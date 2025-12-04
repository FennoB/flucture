#include "pg_query.h"
#include <pqxx/pqxx>
#include <sstream>
#include <algorithm>
#include <iostream>

// Helper function to truncate long vector arrays in SQL for logging
static flx_string truncate_vectors_in_sql(const flx_string& sql) {
  std::string result = sql.c_str();

  size_t pos = 0;
  while ((pos = result.find("'[", pos)) != std::string::npos) {
    size_t start = pos + 2;  // Skip "'["
    size_t end = result.find("]'", start);

    if (end == std::string::npos) {
      pos = start;
      continue;
    }

    size_t length = end - start;

    // If vector array is long (> 100 chars), truncate it
    if (length > 100) {
      std::string vector_content = result.substr(start, length);

      // Count commas to estimate number of values
      size_t comma_count = 0;
      for (char c : vector_content) {
        if (c == ',') comma_count++;
      }
      size_t total_values = comma_count + 1;

      // Keep first 2 values
      size_t first_comma = vector_content.find(',');
      if (first_comma != std::string::npos) {
        size_t second_comma = vector_content.find(',', first_comma + 1);

        if (second_comma != std::string::npos) {
          std::string truncated = vector_content.substr(0, second_comma);
          std::string replacement = "'[" + truncated + ", ... (" +
                                   std::to_string(total_values - 2) + " more)]'";

          // Replace the full vector with truncated version
          result = result.substr(0, pos) + replacement + result.substr(end + 2);
          pos += replacement.length();
          continue;
        }
      }
    }

    pos = end + 2;
  }

  return flx_string(result.c_str());
}

static flx_variant::state oid_to_variant_state(int oid) {
  switch (oid) {
    case 16:   return flx_variant::bool_state;    // BOOLOID
    case 20:   return flx_variant::int_state;     // INT8OID (bigint)
    case 21:   return flx_variant::int_state;     // INT2OID (smallint)
    case 23:   return flx_variant::int_state;     // INT4OID (integer)
    case 700:  return flx_variant::double_state;  // FLOAT4OID
    case 701:  return flx_variant::double_state;  // FLOAT8OID
    case 1700: return flx_variant::double_state;  // NUMERICOID
    case 25:   return flx_variant::string_state;  // TEXTOID
    case 1043: return flx_variant::string_state;  // VARCHAROID
    default:   return flx_variant::string_state;  // Unknown types → string (safe fallback)
  }
}

struct pg_query::impl {
  pqxx::connection* conn;
  std::unique_ptr<pqxx::work> work;
  std::unique_ptr<pqxx::result> result;
};

pg_query::pg_query(void* conn, bool verbose_sql)
  : pimpl_(std::make_unique<impl>())
  , current_row_(0)
  , rows_affected_(0)
  , last_error_("")
  , verbose_sql_(verbose_sql)
{
  pimpl_->conn = static_cast<pqxx::connection*>(conn);
}

pg_query::~pg_query()
{
  if (pimpl_->work) {
    try {
      pimpl_->work->abort();
    } catch (...) {
    }
  }
}

bool pg_query::prepare(const flx_string& sql)
{
  try {
    sql_ = sql;
    indexed_params_.clear();
    named_params_.clear();
    current_row_ = 0;
    rows_affected_ = 0;
    last_error_ = "";
    return true;
  } catch (const std::exception& e) {
    last_error_ = flx_string("Prepare failed: ") + e.what();
    return false;
  }
}

bool pg_query::execute()
{
  try {
    if (!pimpl_->conn || !pimpl_->conn->is_open()) {
      last_error_ = "Connection not open";
      return false;
    }

    pimpl_->work = std::make_unique<pqxx::work>(*pimpl_->conn);

    flx_string final_sql = substitute_params(sql_);

    // Log SQL query if verbose mode enabled (with vector truncation)
    if (verbose_sql_) {
      flx_string truncated_sql = truncate_vectors_in_sql(final_sql);
      std::cout << "[SQL] " << truncated_sql.c_str() << std::endl;
    }

    auto res = pimpl_->work->exec(final_sql.c_str());
    pimpl_->result = std::make_unique<pqxx::result>(res);

    rows_affected_ = static_cast<int>(pimpl_->result->affected_rows());
    current_row_ = 0;

    pimpl_->work->commit();

    last_error_ = "";
    return true;
  } catch (const std::exception& e) {
    last_error_ = flx_string("Execute failed: ") + e.what();
    if (pimpl_->work) {
      try {
        pimpl_->work->abort();
      } catch (...) {
      }
    }
    return false;
  }
}

void pg_query::bind(int index, const flx_variant& value)
{
  indexed_params_[index] = value;
}

void pg_query::bind(const flx_string& name, const flx_variant& value)
{
  named_params_[name] = value;
}

bool pg_query::next()
{
  if (!pimpl_->result) {
    return false;
  }

  if (current_row_ < pimpl_->result->size()) {
    current_row_++;
    return true;
  }

  return false;
}

flxv_map pg_query::get_row()
{
  if (!pimpl_->result || current_row_ == 0 || current_row_ > pimpl_->result->size()) {
    return flxv_map();
  }

  return row_to_variant_map(current_row_ - 1);
}

std::vector<flxv_map> pg_query::get_all_rows()
{
  std::vector<flxv_map> rows;

  if (!pimpl_->result) {
    return rows;
  }

  rows.reserve(pimpl_->result->size());
  for (size_t i = 0; i < pimpl_->result->size(); ++i) {
    rows.push_back(row_to_variant_map(i));
  }

  return rows;
}

int pg_query::rows_affected() const
{
  return rows_affected_;
}

flx_string pg_query::get_last_error() const
{
  return last_error_;
}

flx_string pg_query::substitute_params(const flx_string& sql)
{
  flx_string result = sql;

  // Sort named params by key length (longest first) to avoid partial matches
  // e.g., :contract_duration shouldn't match inside :contract_duration_unit
  std::vector<std::pair<flx_string, flx_variant>> sorted_named_params(named_params_.begin(), named_params_.end());
  std::sort(sorted_named_params.begin(), sorted_named_params.end(),
    [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });

  for (const auto& pair : sorted_named_params) {
    flx_string placeholder = flx_string(":") + pair.first;
    flx_string value;

    // CRITICAL: Check is_null() FIRST before type checks
    if (pair.second.is_null()) {
      value = "NULL";
    } else if (pair.second.is_string()) {
      // CRITICAL: Use pqxx::quote to prevent SQL injection
      value = flx_string(pimpl_->conn->quote(pair.second.string_value().c_str()).c_str());
    } else if (pair.second.is_int()) {
      value = flx_string(pair.second.int_value());
    } else if (pair.second.is_double()) {
      value = flx_string(pair.second.double_value());
    } else if (pair.second.is_bool()) {
      value = pair.second.bool_value() ? "true" : "false";
    } else if (pair.second.in_state() == flx_variant::vector_state) {
      // PostgreSQL halfvec/vector format: [0.1,0.2,0.3,...]
      const flxv_vector& vec = pair.second.vector_value();
      flx_string vec_str = "[";
      for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) vec_str += ",";
        vec_str += flx_string(vec[i].double_value());
      }
      vec_str += "]";
      value = flx_string(pimpl_->conn->quote(vec_str.c_str()).c_str());
    } else {
      // CRITICAL: Use pqxx::quote to prevent SQL injection
      value = flx_string(pimpl_->conn->quote(pair.second.convert(flx_variant::string_state).string_value().c_str()).c_str());
    }

    size_t pos = 0;
    while ((pos = result.to_std().find(placeholder.c_str(), pos)) != flx_string::npos) {
      result = flx_string(result.to_std().replace(pos, placeholder.length(), value.c_str()));
      pos += value.length();
    }
  }

  for (const auto& pair : indexed_params_) {
    flx_string placeholder = flx_string("$") + flx_string(static_cast<long long>(pair.first));
    flx_string value;

    // CRITICAL: Check is_null() FIRST before type checks
    if (pair.second.is_null()) {
      value = "NULL";
    } else if (pair.second.is_string()) {
      // CRITICAL: Use pqxx::quote to prevent SQL injection
      value = flx_string(pimpl_->conn->quote(pair.second.string_value().c_str()).c_str());
    } else if (pair.second.is_int()) {
      value = flx_string(pair.second.int_value());
    } else if (pair.second.is_double()) {
      value = flx_string(pair.second.double_value());
    } else if (pair.second.is_bool()) {
      value = pair.second.bool_value() ? "true" : "false";
    } else if (pair.second.in_state() == flx_variant::vector_state) {
      // PostgreSQL halfvec/vector format: [0.1,0.2,0.3,...]
      const flxv_vector& vec = pair.second.vector_value();
      flx_string vec_str = "[";
      for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) vec_str += ",";
        vec_str += flx_string(vec[i].double_value());
      }
      vec_str += "]";
      value = flx_string(pimpl_->conn->quote(vec_str.c_str()).c_str());
    } else {
      // CRITICAL: Use pqxx::quote to prevent SQL injection
      value = flx_string(pimpl_->conn->quote(pair.second.convert(flx_variant::string_state).string_value().c_str()).c_str());
    }

    size_t pos = 0;
    while ((pos = result.to_std().find(placeholder.c_str(), pos)) != flx_string::npos) {
      result = flx_string(result.to_std().replace(pos, placeholder.length(), value.c_str()));
      pos += value.length();
    }
  }

  return result;
}

flxv_map pg_query::row_to_variant_map(size_t row_index)
{
  flxv_map row_map;

  if (!pimpl_->result || row_index >= pimpl_->result->size()) {
    return row_map;
  }

  const auto& row = (*pimpl_->result)[row_index];

  for (size_t col = 0; col < row.size(); ++col) {
    const auto& field = row[static_cast<int>(col)];
    flx_string column_name = field.name();

    if (field.is_null()) {
      row_map[column_name] = flx_variant();
    } else {
      flx_string value_str = field.c_str();

      // Parse vectors: PostgreSQL array format [1.2,3.4,5.6]
      if (value_str.starts_with("[") && value_str.ends_with("]")) {
        flx_string vec_content = value_str.substr(1, value_str.length() - 2);
        auto parts = vec_content.split(',');

        flxv_vector vec;
        vec.reserve(parts.size());
        for (const auto& part : parts) {
          vec.push_back(flx_variant(part.trim().to_double(0.0)));
        }
        row_map[column_name] = flx_variant(vec);
      }
      else {
        int oid = field.type();
        flx_variant::state target_state = oid_to_variant_state(oid);
        flx_variant temp(value_str);
        row_map[column_name] = temp.convert(target_state);
      }
    }
  }

  return row_map;
}

flx_string pg_query::get_sql() const
{
  return sql_;
}
