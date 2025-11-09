#include "db_query_builder.h"
#include <sstream>

db_query_builder::db_query_builder()
  : limit_(-1)
  , offset_(-1)
  , param_counter_(0)
{
}

db_query_builder& db_query_builder::select(const flx_string& fields)
{
  select_fields_.push_back(fields);
  return *this;
}

db_query_builder& db_query_builder::select(const std::vector<flx_string>& fields)
{
  for (const auto& field : fields) {
    select_fields_.push_back(field);
  }
  return *this;
}

db_query_builder& db_query_builder::from(const flx_string& table)
{
  table_ = table;
  table_alias_ = "";
  return *this;
}

db_query_builder& db_query_builder::from(const flx_string& table, const flx_string& alias)
{
  table_ = table;
  table_alias_ = alias;
  return *this;
}

db_query_builder& db_query_builder::where(const flx_string& field, operator_type op, const flx_variant& value)
{
  conditions_.emplace_back(field, op, value, conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::where(const flx_string& field, const flx_string& op_str, const flx_variant& value)
{
  return where(field, parse_operator(op_str), value);
}

db_query_builder& db_query_builder::and_where(const flx_string& field, operator_type op, const flx_variant& value)
{
  conditions_.emplace_back(field, op, value, "AND");
  return *this;
}

db_query_builder& db_query_builder::and_where(const flx_string& field, const flx_string& op_str, const flx_variant& value)
{
  return and_where(field, parse_operator(op_str), value);
}

db_query_builder& db_query_builder::or_where(const flx_string& field, operator_type op, const flx_variant& value)
{
  conditions_.emplace_back(field, op, value, "OR");
  return *this;
}

db_query_builder& db_query_builder::or_where(const flx_string& field, const flx_string& op_str, const flx_variant& value)
{
  return or_where(field, parse_operator(op_str), value);
}

db_query_builder& db_query_builder::where_null(const flx_string& field)
{
  conditions_.emplace_back(field, operator_type::IS_NULL, flx_variant(), conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::where_not_null(const flx_string& field)
{
  conditions_.emplace_back(field, operator_type::IS_NOT_NULL, flx_variant(), conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::where_in(const flx_string& field, const std::vector<flx_variant>& values)
{
  flxv_vector vec;
  for (const auto& v : values) {
    vec.push_back(v);
  }
  conditions_.emplace_back(field, operator_type::IN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::where_not_in(const flx_string& field, const std::vector<flx_variant>& values)
{
  flxv_vector vec;
  for (const auto& v : values) {
    vec.push_back(v);
  }
  conditions_.emplace_back(field, operator_type::NOT_IN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::where_between(const flx_string& field, const flx_variant& min, const flx_variant& max)
{
  flxv_vector vec;
  vec.push_back(min);
  vec.push_back(max);
  conditions_.emplace_back(field, operator_type::BETWEEN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_query_builder& db_query_builder::join(const flx_string& table, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::INNER, table, "", on_condition);
  return *this;
}

db_query_builder& db_query_builder::join(const flx_string& table, const flx_string& alias, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::INNER, table, alias, on_condition);
  return *this;
}

db_query_builder& db_query_builder::left_join(const flx_string& table, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::LEFT, table, "", on_condition);
  return *this;
}

db_query_builder& db_query_builder::left_join(const flx_string& table, const flx_string& alias, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::LEFT, table, alias, on_condition);
  return *this;
}

db_query_builder& db_query_builder::right_join(const flx_string& table, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::RIGHT, table, "", on_condition);
  return *this;
}

db_query_builder& db_query_builder::inner_join(const flx_string& table, const flx_string& on_condition)
{
  joins_.emplace_back(join_type::INNER, table, "", on_condition);
  return *this;
}

db_query_builder& db_query_builder::order_by(const flx_string& field, bool ascending)
{
  order_by_.emplace_back(field, ascending);
  return *this;
}

db_query_builder& db_query_builder::limit(int count)
{
  limit_ = count;
  return *this;
}

db_query_builder& db_query_builder::offset(int count)
{
  offset_ = count;
  return *this;
}

db_query_builder& db_query_builder::insert_into(const flx_string& table)
{
  table_ = table;
  return *this;
}

db_query_builder& db_query_builder::values(const flxv_map& values)
{
  insert_values_ = values;
  return *this;
}

db_query_builder& db_query_builder::update(const flx_string& table)
{
  table_ = table;
  return *this;
}

db_query_builder& db_query_builder::set(const flx_string& field, const flx_variant& value)
{
  update_values_[field] = value;
  return *this;
}

db_query_builder& db_query_builder::set(const flxv_map& values)
{
  update_values_ = values;
  return *this;
}

db_query_builder& db_query_builder::delete_from(const flx_string& table)
{
  table_ = table;
  return *this;
}

flx_string db_query_builder::build_select()
{
  flx_string sql = "SELECT ";

  if (select_fields_.empty()) {
    sql += "*";
  } else {
    for (size_t i = 0; i < select_fields_.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += select_fields_[i];
    }
  }

  sql += " FROM " + table_;
  if (!table_alias_.empty()) {
    sql += " " + table_alias_;
  }

  sql += build_join_clauses();

  flx_string where = build_where_clause();
  if (!where.empty()) {
    sql += " WHERE " + where;
  }

  if (!order_by_.empty()) {
    sql += " ORDER BY ";
    for (size_t i = 0; i < order_by_.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += order_by_[i].first + (order_by_[i].second ? " ASC" : " DESC");
    }
  }

  if (limit_ > 0) {
    sql += " LIMIT " + flx_string(std::to_string(limit_).c_str());
  }

  if (offset_ > 0) {
    sql += " OFFSET " + flx_string(std::to_string(offset_).c_str());
  }

  return sql;
}

flx_string db_query_builder::build_insert()
{
  if (insert_values_.empty()) {
    return "";
  }

  flx_string sql = "INSERT INTO " + table_ + " (";
  flx_string values_clause = " VALUES (";

  bool first = true;
  for (const auto& pair : insert_values_) {
    if (!first) {
      sql += ", ";
      values_clause += ", ";
    }
    sql += pair.first.c_str();

    flx_string param_name = generate_param_name();
    values_clause += ":" + param_name;
    add_parameter(param_name, pair.second);

    first = false;
  }

  sql += ")" + values_clause + ")";
  return sql;
}

flx_string db_query_builder::build_update()
{
  if (update_values_.empty()) {
    return "";
  }

  flx_string sql = "UPDATE " + table_ + " SET ";

  bool first = true;
  for (const auto& pair : update_values_) {
    if (!first) sql += ", ";

    flx_string param_name = generate_param_name();
    sql += pair.first.c_str() + flx_string(" = :") + param_name;
    add_parameter(param_name, pair.second);

    first = false;
  }

  flx_string where = build_where_clause();
  if (!where.empty()) {
    sql += " WHERE " + where;
  }

  return sql;
}

flx_string db_query_builder::build_delete()
{
  flx_string sql = "DELETE FROM " + table_;

  flx_string where = build_where_clause();
  if (!where.empty()) {
    sql += " WHERE " + where;
  }

  return sql;
}

flx_string db_query_builder::build()
{
  if (!select_fields_.empty() || !joins_.empty()) {
    return build_select();
  }
  if (!insert_values_.empty()) {
    return build_insert();
  }
  if (!update_values_.empty()) {
    return build_update();
  }
  return build_select();
}

const std::map<flx_string, flx_variant>& db_query_builder::get_parameters() const
{
  return parameters_;
}

void db_query_builder::reset()
{
  table_ = "";
  table_alias_ = "";
  select_fields_.clear();
  conditions_.clear();
  joins_.clear();
  order_by_.clear();
  limit_ = -1;
  offset_ = -1;
  insert_values_.clear();
  update_values_.clear();
  parameters_.clear();
  param_counter_ = 0;
}

db_query_builder::operator_type db_query_builder::parse_operator(const flx_string& op_str)
{
  if (op_str == "=" || op_str == "==") return operator_type::EQUAL;
  if (op_str == "!=" || op_str == "<>") return operator_type::NOT_EQUAL;
  if (op_str == ">") return operator_type::GREATER;
  if (op_str == "<") return operator_type::LESS;
  if (op_str == ">=") return operator_type::GREATER_EQUAL;
  if (op_str == "<=") return operator_type::LESS_EQUAL;
  if (op_str.to_lower() == "like") return operator_type::LIKE;
  if (op_str.to_lower() == "not like") return operator_type::NOT_LIKE;
  if (op_str.to_lower() == "in") return operator_type::IN;
  if (op_str.to_lower() == "not in") return operator_type::NOT_IN;
  if (op_str.to_lower() == "is null") return operator_type::IS_NULL;
  if (op_str.to_lower() == "is not null") return operator_type::IS_NOT_NULL;
  if (op_str.to_lower() == "between") return operator_type::BETWEEN;
  if (op_str == "<->") return operator_type::VECTOR_DISTANCE;

  return operator_type::EQUAL;  // Default
}

flx_string db_query_builder::operator_to_sql(operator_type op)
{
  switch (op) {
    case operator_type::EQUAL: return "=";
    case operator_type::NOT_EQUAL: return "!=";
    case operator_type::GREATER: return ">";
    case operator_type::LESS: return "<";
    case operator_type::GREATER_EQUAL: return ">=";
    case operator_type::LESS_EQUAL: return "<=";
    case operator_type::LIKE: return "LIKE";
    case operator_type::NOT_LIKE: return "NOT LIKE";
    case operator_type::IN: return "IN";
    case operator_type::NOT_IN: return "NOT IN";
    case operator_type::IS_NULL: return "IS NULL";
    case operator_type::IS_NOT_NULL: return "IS NOT NULL";
    case operator_type::BETWEEN: return "BETWEEN";
    case operator_type::VECTOR_DISTANCE: return "<->";
  }
  return "=";
}

flx_string db_query_builder::generate_param_name()
{
  return flx_string(("param" + std::to_string(param_counter_++)).c_str());
}

void db_query_builder::add_parameter(const flx_string& name, const flx_variant& value)
{
  parameters_[name] = value;
}

flx_string db_query_builder::build_where_clause()
{
  if (conditions_.empty()) {
    return "";
  }

  flx_string where;

  for (size_t i = 0; i < conditions_.size(); ++i) {
    const auto& cond = conditions_[i];

    if (i > 0 && !cond.conjunction.empty()) {
      where += " " + cond.conjunction + " ";
    }

    if (cond.op == operator_type::IS_NULL || cond.op == operator_type::IS_NOT_NULL) {
      where += cond.field + " " + operator_to_sql(cond.op);
    }
    else if (cond.op == operator_type::IN || cond.op == operator_type::NOT_IN) {
      where += cond.field + " " + operator_to_sql(cond.op) + " (";

      if (cond.value.is_vector()) {
        const auto& vec = cond.value.vector_value();
        for (size_t j = 0; j < vec.size(); ++j) {
          if (j > 0) where += ", ";
          flx_string param_name = generate_param_name();
          where += ":" + param_name;
          add_parameter(param_name, vec[j]);
        }
      }
      where += ")";
    }
    else if (cond.op == operator_type::BETWEEN) {
      if (cond.value.is_vector() && cond.value.vector_value().size() == 2) {
        const auto& vec = cond.value.vector_value();
        flx_string param1 = generate_param_name();
        flx_string param2 = generate_param_name();

        where += cond.field + " BETWEEN :" + param1 + " AND :" + param2;
        add_parameter(param1, vec[0]);
        add_parameter(param2, vec[1]);
      }
    }
    else {
      flx_string param_name = generate_param_name();
      where += cond.field + " " + operator_to_sql(cond.op) + " :" + param_name;
      add_parameter(param_name, cond.value);
    }
  }

  return where;
}

flx_string db_query_builder::build_join_clauses()
{
  if (joins_.empty()) {
    return "";
  }

  flx_string sql;

  for (const auto& join : joins_) {
    switch (join.type) {
      case join_type::INNER:
        sql += " INNER JOIN ";
        break;
      case join_type::LEFT:
        sql += " LEFT JOIN ";
        break;
      case join_type::RIGHT:
        sql += " RIGHT JOIN ";
        break;
      case join_type::FULL:
        sql += " FULL JOIN ";
        break;
    }

    sql += join.table;
    if (!join.alias.empty()) {
      sql += " " + join.alias;
    }
    sql += " ON " + join.on_condition;
  }

  return sql;
}
