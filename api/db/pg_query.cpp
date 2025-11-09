#include "pg_query.h"
#include <pqxx/pqxx>
#include <sstream>
#include <algorithm>
#include <iostream>

struct pg_query::impl {
  pqxx::connection* conn;
  std::unique_ptr<pqxx::work> work;
  std::unique_ptr<pqxx::result> result;
};

pg_query::pg_query(void* conn)
  : pimpl_(std::make_unique<impl>())
  , current_row_(0)
  , rows_affected_(0)
  , last_error_("")
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

    // DEBUG
    std::cout << "[SQL BIND DEBUG] " << pair.first.c_str()
              << " is_null=" << pair.second.is_null()
              << " is_string=" << pair.second.is_string()
              << " is_int=" << pair.second.is_int()
              << " is_double=" << pair.second.is_double()
              << " is_bool=" << pair.second.is_bool()
              << " state=" << pair.second.in_state()
              << std::endl;

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
      // Try to get the value with proper type conversion
      flx_string value_str = field.c_str();

      // Try to detect numeric types by attempting conversion
      // Check for boolean first
      if (value_str == "t" || value_str == "f" ||
          value_str == "true" || value_str == "false") {
        row_map[column_name] = flx_variant(value_str == "t" || value_str == "true");
      }
      // Check if it contains a decimal point - if so, it's a double
      else if (value_str.contains(".") && value_str.is_double()) {
        row_map[column_name] = flx_variant(value_str.to_double(0.0));
      }
      // Check if it's an integer
      else if (value_str.is_integer()) {
        row_map[column_name] = flx_variant(static_cast<long long>(value_str.to_int(0)));
      }
      // Otherwise it's a string
      else {
        row_map[column_name] = flx_variant(value_str);
      }
    }
  }

  return row_map;
}
