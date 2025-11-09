#ifndef DB_QUERY_BUILDER_H
#define DB_QUERY_BUILDER_H

#include "../../utils/flx_string.h"
#include "../../utils/flx_variant.h"
#include <vector>
#include <map>

class db_query_builder {
public:
  enum class operator_type {
    EQUAL,
    NOT_EQUAL,
    GREATER,
    LESS,
    GREATER_EQUAL,
    LESS_EQUAL,
    LIKE,
    NOT_LIKE,
    IN,
    NOT_IN,
    IS_NULL,
    IS_NOT_NULL,
    BETWEEN,
    VECTOR_DISTANCE  // pgvector <-> operator for semantic search
  };

  enum class join_type {
    INNER,
    LEFT,
    RIGHT,
    FULL
  };

  struct condition {
    flx_string field;
    operator_type op;
    flx_variant value;
    flx_string conjunction;  // "AND" or "OR"

    condition(const flx_string& f, operator_type o, const flx_variant& v, const flx_string& c = "AND")
      : field(f), op(o), value(v), conjunction(c) {}
  };

  struct join_clause {
    join_type type;
    flx_string table;
    flx_string alias;
    flx_string on_condition;

    join_clause(join_type t, const flx_string& tbl, const flx_string& a, const flx_string& on)
      : type(t), table(tbl), alias(a), on_condition(on) {}
  };

  db_query_builder();

  // SELECT building
  db_query_builder& select(const flx_string& fields);
  db_query_builder& select(const std::vector<flx_string>& fields);
  db_query_builder& from(const flx_string& table);
  db_query_builder& from(const flx_string& table, const flx_string& alias);

  // WHERE conditions
  db_query_builder& where(const flx_string& field, operator_type op, const flx_variant& value);
  db_query_builder& where(const flx_string& field, const flx_string& op_str, const flx_variant& value);
  db_query_builder& and_where(const flx_string& field, operator_type op, const flx_variant& value);
  db_query_builder& and_where(const flx_string& field, const flx_string& op_str, const flx_variant& value);
  db_query_builder& or_where(const flx_string& field, operator_type op, const flx_variant& value);
  db_query_builder& or_where(const flx_string& field, const flx_string& op_str, const flx_variant& value);

  db_query_builder& where_null(const flx_string& field);
  db_query_builder& where_not_null(const flx_string& field);
  db_query_builder& where_in(const flx_string& field, const std::vector<flx_variant>& values);
  db_query_builder& where_not_in(const flx_string& field, const std::vector<flx_variant>& values);
  db_query_builder& where_between(const flx_string& field, const flx_variant& min, const flx_variant& max);

  // JOINs
  db_query_builder& join(const flx_string& table, const flx_string& on_condition);
  db_query_builder& join(const flx_string& table, const flx_string& alias, const flx_string& on_condition);
  db_query_builder& left_join(const flx_string& table, const flx_string& on_condition);
  db_query_builder& left_join(const flx_string& table, const flx_string& alias, const flx_string& on_condition);
  db_query_builder& right_join(const flx_string& table, const flx_string& on_condition);
  db_query_builder& inner_join(const flx_string& table, const flx_string& on_condition);

  // ORDER BY / LIMIT / OFFSET
  db_query_builder& order_by(const flx_string& field, bool ascending = true);
  db_query_builder& limit(int count);
  db_query_builder& offset(int count);

  // INSERT building
  db_query_builder& insert_into(const flx_string& table);
  db_query_builder& values(const flxv_map& values);

  // UPDATE building
  db_query_builder& update(const flx_string& table);
  db_query_builder& set(const flx_string& field, const flx_variant& value);
  db_query_builder& set(const flxv_map& values);

  // DELETE building
  db_query_builder& delete_from(const flx_string& table);

  // Build final SQL
  flx_string build_select();
  flx_string build_insert();
  flx_string build_update();
  flx_string build_delete();
  flx_string build();

  // Get bound parameters
  const std::map<flx_string, flx_variant>& get_parameters() const;

  // Reset builder
  void reset();

  // Static helper: operator string to enum
  static operator_type parse_operator(const flx_string& op_str);
  static flx_string operator_to_sql(operator_type op);

private:
  flx_string table_;
  flx_string table_alias_;
  std::vector<flx_string> select_fields_;
  std::vector<condition> conditions_;
  std::vector<join_clause> joins_;
  std::vector<std::pair<flx_string, bool>> order_by_;  // field, ascending
  int limit_;
  int offset_;

  flxv_map insert_values_;
  flxv_map update_values_;

  std::map<flx_string, flx_variant> parameters_;
  int param_counter_;

  flx_string generate_param_name();
  void add_parameter(const flx_string& name, const flx_variant& value);
  flx_string build_where_clause();
  flx_string build_join_clauses();
};

#endif // DB_QUERY_BUILDER_H
