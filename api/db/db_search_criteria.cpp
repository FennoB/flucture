#include "db_search_criteria.h"

db_search_criteria::db_search_criteria()
  : limit_(-1)
  , offset_(-1)
  , hierarchy_mode_(false)
{
}

db_search_criteria::db_search_criteria(flx_model* model, const flx_string& root_table)
  : limit_(-1)
  , offset_(-1)
  , hierarchy_mode_(true)
{
  if (model) {
    build_column_mapping(*model, root_table);
  }
}

db_search_criteria& db_search_criteria::where(const flx_string& field, const flx_string& op, const flx_variant& value)
{
  // Validate column in hierarchy mode
  if (hierarchy_mode_ && !is_valid_column(field)) {
    // Invalid column - silently skip (security: don't allow arbitrary SQL)
    return *this;
  }

  auto op_type = db_query_builder::parse_operator(op);
  conditions_.emplace_back(field, op_type, value, conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::and_where(const flx_string& field, const flx_string& op, const flx_variant& value)
{
  // Validate column in hierarchy mode
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;  // Silently skip invalid columns
  }

  auto op_type = db_query_builder::parse_operator(op);
  conditions_.emplace_back(field, op_type, value, "AND");
  return *this;
}

db_search_criteria& db_search_criteria::or_where(const flx_string& field, const flx_string& op, const flx_variant& value)
{
  // Validate column in hierarchy mode
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;  // Silently skip invalid columns
  }

  auto op_type = db_query_builder::parse_operator(op);
  conditions_.emplace_back(field, op_type, value, "OR");
  return *this;
}

db_search_criteria& db_search_criteria::equals(const flx_string& field, const flx_variant& value)
{
  return where(field, "=", value);
}

db_search_criteria& db_search_criteria::not_equals(const flx_string& field, const flx_variant& value)
{
  return where(field, "!=", value);
}

db_search_criteria& db_search_criteria::greater_than(const flx_string& field, const flx_variant& value)
{
  return where(field, ">", value);
}

db_search_criteria& db_search_criteria::less_than(const flx_string& field, const flx_variant& value)
{
  return where(field, "<", value);
}

db_search_criteria& db_search_criteria::greater_equal(const flx_string& field, const flx_variant& value)
{
  return where(field, ">=", value);
}

db_search_criteria& db_search_criteria::less_equal(const flx_string& field, const flx_variant& value)
{
  return where(field, "<=", value);
}

db_search_criteria& db_search_criteria::like(const flx_string& field, const flx_string& pattern)
{
  return where(field, "LIKE", flx_variant(pattern));
}

db_search_criteria& db_search_criteria::not_like(const flx_string& field, const flx_string& pattern)
{
  return where(field, "NOT LIKE", flx_variant(pattern));
}

db_search_criteria& db_search_criteria::is_null(const flx_string& field)
{
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;
  }
  conditions_.emplace_back(field, db_query_builder::operator_type::IS_NULL, flx_variant(), conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::is_not_null(const flx_string& field)
{
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;
  }
  conditions_.emplace_back(field, db_query_builder::operator_type::IS_NOT_NULL, flx_variant(), conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::in(const flx_string& field, const std::vector<flx_variant>& values)
{
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;
  }
  flxv_vector vec;
  for (const auto& v : values) {
    vec.push_back(v);
  }
  conditions_.emplace_back(field, db_query_builder::operator_type::IN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::not_in(const flx_string& field, const std::vector<flx_variant>& values)
{
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;
  }
  flxv_vector vec;
  for (const auto& v : values) {
    vec.push_back(v);
  }
  conditions_.emplace_back(field, db_query_builder::operator_type::NOT_IN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::between(const flx_string& field, const flx_variant& min, const flx_variant& max)
{
  if (hierarchy_mode_ && !is_valid_column(field)) {
    return *this;
  }
  flxv_vector vec;
  vec.push_back(min);
  vec.push_back(max);
  conditions_.emplace_back(field, db_query_builder::operator_type::BETWEEN, flx_variant(vec), conditions_.empty() ? "" : "AND");
  return *this;
}

db_search_criteria& db_search_criteria::order_by(const flx_string& field, bool ascending)
{
  order_by_.emplace_back(field, ascending);
  return *this;
}

db_search_criteria& db_search_criteria::order_by_desc(const flx_string& field)
{
  return order_by(field, false);
}

db_search_criteria& db_search_criteria::limit(int count)
{
  limit_ = count;
  return *this;
}

db_search_criteria& db_search_criteria::offset(int count)
{
  offset_ = count;
  return *this;
}

flx_string db_search_criteria::to_where_clause() const
{
  if (conditions_.empty()) {
    return "";
  }

  db_query_builder builder;
  apply_to(builder);
  return builder.build_select();  // Will include WHERE clause
}

void db_search_criteria::apply_to(db_query_builder& builder) const
{
  for (const auto& cond : conditions_) {
    // Get qualified field name
    flx_string qualified_field = hierarchy_mode_ ? qualify_column(cond.field) : cond.field;

    // Skip if qualification returned empty (invalid column)
    if (qualified_field.empty()) {
      continue;
    }

    // Check if field appears in multiple tables (unqualified multi-table column)
    if (hierarchy_mode_ && !qualified_field.contains(".")) {
      // Multi-table column - need to OR across all tables
      auto it = column_to_tables_.find(cond.field);
      if (it != column_to_tables_.end() && it->second.size() > 1) {
        // Build OR condition for each table
        const std::vector<flx_string>& tables = it->second;

        // Add opening parenthesis
        // Note: db_query_builder doesn't support grouping yet
        // For now, just apply to first table
        // TODO: Enhance db_query_builder to support grouped OR conditions
        qualified_field = tables[0] + "." + cond.field;
      }
    }

    // Apply condition with qualified field
    if (cond.conjunction == "OR") {
      builder.or_where(qualified_field, cond.op, cond.value);
    } else {
      if (conditions_[0].field == cond.field && &cond == &conditions_[0]) {
        builder.where(qualified_field, cond.op, cond.value);
      } else {
        builder.and_where(qualified_field, cond.op, cond.value);
      }
    }
  }

  for (const auto& order : order_by_) {
    // Qualify ORDER BY fields too
    // Skip qualification for complex expressions (e.g., distance expressions with <->)
    flx_string qualified_order = order.first;

    // Only qualify if it's a simple column name (no operators like <->, no spaces after trim)
    bool is_simple_column = !order.first.contains("<->") &&
                           !order.first.contains("(") &&
                           !order.first.contains(" ");

    if (is_simple_column && hierarchy_mode_) {
      qualified_order = qualify_column(order.first);
      if (!qualified_order.empty()) {
        // Handle multi-table columns - use first table
        if (!qualified_order.contains(".")) {
          auto it = column_to_tables_.find(order.first);
          if (it != column_to_tables_.end() && !it->second.empty()) {
            qualified_order = it->second[0] + "." + order.first;
          }
        }
      } else {
        qualified_order = order.first;  // Fallback to original
      }
    }

    builder.order_by(qualified_order, order.second);
  }

  if (limit_ > 0) {
    builder.limit(limit_);
  }

  if (offset_ > 0) {
    builder.offset(offset_);
  }
}

const std::vector<db_query_builder::condition>& db_search_criteria::get_conditions() const
{
  return conditions_;
}

void db_search_criteria::reset()
{
  conditions_.clear();
  order_by_.clear();
  limit_ = -1;
  offset_ = -1;
  vector_search_.active = false;
  vector_search_.query_embedding.clear();
}

bool db_search_criteria::is_empty() const
{
  return conditions_.empty();
}

db_search_criteria& db_search_criteria::semantic_search(
    const flx_string& embedding_field,
    const std::vector<double>& query_embedding,
    int top_k
)
{
  vector_search_.embedding_field = embedding_field;
  vector_search_.query_embedding = query_embedding;
  vector_search_.top_k = top_k;
  vector_search_.active = true;

  // Build pgvector format: '[1.0, 2.0, ...]'::vector
  flx_string vector_literal = "[";
  for (size_t i = 0; i < query_embedding.size(); ++i) {
    if (i > 0) vector_literal += ", ";
    vector_literal += flx_string(std::to_string(query_embedding[i]).c_str());
  }
  vector_literal += "]";

  // Add ORDER BY for pgvector distance: embedding_field <-> '[...]'::vector
  flx_string distance_expr = embedding_field + " <-> '" + vector_literal + "'::vector";
  order_by(distance_expr, true);  // Ascending = closest match first
  limit(top_k);

  return *this;
}

const db_search_criteria::vector_search_config& db_search_criteria::get_vector_search() const
{
  return vector_search_;
}

bool db_search_criteria::has_vector_search() const
{
  return vector_search_.active;
}

void db_search_criteria::build_column_mapping(flx_model& model, const flx_string& table_name)
{
  // Add this table to valid tables
  valid_tables_.insert(table_name);

  // Get all properties
  const auto& properties = model.get_properties();

  // Map all columns from this table
  for (const auto& prop_pair : properties) {
    const flx_string& fieldname = prop_pair.first;
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Only process properties with {"column", "..."} metadata
    if (meta.find("column") != meta.end()) {
      flx_string column_name = meta.at("column").string_value();

      // Add mapping: column → table
      column_to_tables_[column_name].push_back(table_name);
    }
  }

  // Recursively process child models
  const auto& children = model.get_children();
  for (const auto& child_pair : children) {
    const flx_string& child_fieldname = child_pair.first;
    flx_model* child = child_pair.second;

    // Find the corresponding property to get table metadata
    auto prop_it = properties.find(child_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();
        build_column_mapping(*child, child_table);
      }
    }
  }

  // Recursively process model_lists
  const auto& model_lists = model.get_model_lists();
  for (const auto& list_pair : model_lists) {
    const flx_string& list_fieldname = list_pair.first;
    flx_list* list = list_pair.second;

    // Find the corresponding property to get table metadata
    auto prop_it = properties.find(list_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();

        // Add this table
        valid_tables_.insert(child_table);

        // Always process list structure (even if empty)
        if (list->list_size() > 0) {
          flx_model* first_elem = list->get_model_at(0);
          if (first_elem) {
            build_column_mapping(*first_elem, child_table);
          }
        } else {
          // List is empty - add a temporary element to introspect structure
          // This is safe because we're just introspecting, not persisting
          // Cast to flx_model_list to call add_element()
          // Note: This assumes the list is actually a flx_model_list<T>
          // Since we can't easily create a typed instance without templates,
          // we'll just map the basic columns we know all child models have

          // For now, add common columns that all models should have
          // These will be refined when the list has actual data
          column_to_tables_["id"].push_back(child_table);
        }
      }
    }
  }
}

flx_string db_search_criteria::qualify_column(const flx_string& column) const
{
  // If not in hierarchy mode, return column as-is
  if (!hierarchy_mode_) {
    return column;
  }

  // Check if column is already qualified (contains ".")
  if (column.contains(".")) {
    // Extract table and column parts
    auto parts = column.split(".");
    if (parts.size() == 2) {
      const flx_string& table = parts[0];
      const flx_string& col = parts[1];

      // Validate table.column combination
      if (is_valid_qualified_column(table, col)) {
        return column;  // Already qualified and valid
      } else {
        // Invalid qualification - return empty (will be filtered out)
        return "";
      }
    } else {
      // Malformed table.column - return empty
      return "";
    }
  }

  // Unqualified column - look up in mapping
  auto it = column_to_tables_.find(column);
  if (it == column_to_tables_.end()) {
    // Column not found in any table - return empty (invalid column)
    return "";
  }

  const std::vector<flx_string>& tables = it->second;

  if (tables.size() == 1) {
    // Column exists in exactly one table - qualify it
    return tables[0] + "." + column;
  } else if (tables.size() > 1) {
    // Column exists in multiple tables - return as-is
    // Caller will need to handle OR logic
    return column;
  }

  // No tables (shouldn't happen) - return empty
  return "";
}

bool db_search_criteria::is_valid_column(const flx_string& column) const
{
  // In non-hierarchy mode, all columns are considered valid
  if (!hierarchy_mode_) {
    return true;
  }

  // Check if column contains "."
  if (column.contains(".")) {
    auto parts = column.split(".");
    if (parts.size() == 2) {
      return is_valid_qualified_column(parts[0], parts[1]);
    }
    return false;  // Malformed
  }

  // Unqualified column - check if it exists in mapping
  return column_to_tables_.find(column) != column_to_tables_.end();
}

bool db_search_criteria::is_valid_qualified_column(const flx_string& table, const flx_string& column) const
{
  // Check table is valid
  if (valid_tables_.find(table) == valid_tables_.end()) {
    return false;
  }

  // Check column exists in that table
  auto it = column_to_tables_.find(column);
  if (it == column_to_tables_.end()) {
    return false;
  }

  // Check this specific table has this column
  const std::vector<flx_string>& tables = it->second;
  for (const auto& t : tables) {
    if (t == table) {
      return true;
    }
  }

  return false;
}
