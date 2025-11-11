#ifndef DB_REPOSITORY_H
#define DB_REPOSITORY_H

#include "db_connection.h"
#include "db_query.h"
#include "db_query_builder.h"
#include "db_search_criteria.h"
#include "db_exceptions.h"
#include "flx_semantic_embedder.h"
#include "../../utils/flx_model.h"
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <iostream>

class db_repository {
public:
  explicit db_repository(db_connection* conn);
  virtual ~db_repository() = default;

  // CRUD Operations (throw exceptions on error)
  virtual void create(flx_model& model);
  virtual void update(flx_model& model);
  virtual void remove(flx_model& model);
  virtual void find_by_id(long long id, flx_model& model);
  virtual void find_all(flx_list& results);
  virtual void find_where(const flx_string& condition, flx_list& results);

  // Utility
  virtual bool table_exists(flx_model& model);  // Returns bool with semantic meaning
  virtual void create_table(flx_model& model);
  virtual void migrate_table(flx_model& model);  // Auto-add missing columns from model
  virtual void drop_table(flx_model& model);
  virtual void ensure_structures(flx_model& model);  // Ensure all tables exist (idempotent)

  // Configuration
  void set_id_column(const flx_string& column_name);
  void set_embedder(flx_semantic_embedder* embedder);
  flx_string get_last_error() const;

  // Metadata-based automation
  virtual void auto_configure(flx_model& model);
  virtual void search(const db_search_criteria& criteria, flx_list& results);

  // Extract table name from model's primary_key metadata
  static flx_string extract_table_name(const flx_model& model);

  // Hierarchical search (two-phase: ID query + batch load + tree construction)
  virtual void search_hierarchical(const db_search_criteria& criteria, flx_list& results);

  // Metadata scanning structures
  struct field_metadata {
    flx_string property_name;  // XML fieldname for nested path navigation (legacy)
    flx_string cpp_name;       // C++ property name for map lookup
    flx_string column_name;
    bool is_primary_key = false;
    bool is_foreign_key = false;
    flx_string foreign_table;
    flx_variant::state type;
    bool is_unique = false;            // UNIQUE constraint
    bool is_not_null = false;          // NOT NULL constraint
  };

  struct relation_metadata {
    flx_string property_name;
    flx_string related_table;
    flx_string foreign_key_column;
  };

  virtual std::vector<field_metadata> scan_fields(const flx_model& model);
  virtual std::vector<relation_metadata> scan_relations(const flx_model& model);

  // Hierarchical operations helpers
  virtual void save_nested_objects(flx_model& model);
  virtual void load_nested_objects(flx_model& model);
  virtual flx_string build_join_sql(const flx_model& model, const std::vector<relation_metadata>& relations);
  virtual void map_joined_results(const flxv_map& row, flx_model& model, const std::vector<relation_metadata>& relations);

  // Hierarchical search helpers (public for testing)
  virtual flx_string find_primary_key_column(flx_model& model);
  virtual std::set<flx_string> collect_all_table_names(flx_model& model, const flx_string& root_table);
  virtual flx_string build_joins_recursive(flx_model& model, const flx_string& parent_table);
  virtual flx_string build_id_selects_recursive(flx_model& model, const flx_string& table_name);
  virtual flx_string build_hierarchy_query(flx_model& model, const flx_string& root_table, const db_search_criteria& criteria, db_query_builder& builder);
  virtual std::map<flx_string, std::set<long long>> parse_hierarchy_results(const std::vector<flxv_map>& rows, flx_model& model, const flx_string& root_table);
  virtual std::map<flx_string, std::map<long long, flxv_map>> batch_load_rows(const std::map<flx_string, std::set<long long>>& id_sets);
  virtual void construct_tree_recursive(flx_model& model, const flx_string& table_name, long long id, const std::map<flx_string, std::map<long long, flxv_map>>& all_rows);

protected:
  db_connection* connection_;
  flx_string id_column_;
  flx_string last_error_;
  flx_semantic_embedder* embedder_;

  // Helper methods
  virtual flx_string build_insert_sql(const flx_model& model);
  virtual flx_string build_update_sql(const flx_model& model);
  virtual flx_string build_select_sql(flx_model& model, const flx_string& where_clause = "");
  virtual void bind_model_values(db_query* query, flx_model& model);
  virtual void map_to_model(const flxv_map& row, flx_model& model);
  virtual flx_string get_sql_type(const flx_variant& value);
  virtual flx_string get_sql_type_from_state(flx_variant::state state, const flx_string& column_name = "");
  virtual flx_variant access_nested_value(flx_model& model, const flx_string& property_name);
  virtual std::set<flx_string> get_existing_columns(flx_model& model);  // Query database schema
  virtual void ensure_child_table_from_model(flx_model* child_model);  // Create child table dynamically
  virtual void save_nested_objects_impl(flx_model& model);  // Generic recursive save helper
  virtual bool has_semantic_properties(const flx_model& model) const;  // Check for semantic metadata
  virtual void ensure_pgvector_extension();  // Ensure pgvector extension exists
  virtual void create_semantic_index(const flx_string& table_name);  // Create HNSW index for semantic_embedding

private:
  // Helper structs and methods for save_nested_objects_impl refactoring (SRP)
  struct child_source_info {
    flx_model* single_child;
    flx_list* model_list;
    size_t item_count;
  };

  flx_variant validate_parent_id(flx_model& model);  // Throws db_null_id_error
  flx_model* find_typed_child_model(flx_model& parent, const relation_metadata& rel);
  std::vector<field_metadata> scan_child_field_metadata(flx_model* typed_child_model);
  child_source_info determine_child_source(flx_model& model, const relation_metadata& rel);
  flx_string build_child_insert_sql(const relation_metadata& rel, const std::vector<field_metadata>& fields);
  void bind_and_execute_child_insert(db_query* query, const relation_metadata& rel, flx_model* item_model,
                                       const flx_variant& parent_id, const std::vector<field_metadata>& fields);  // Throws db_query_error
  long long retrieve_inserted_id();
  void update_child_foreign_key(flx_model* item_model, const relation_metadata& rel,
                                 const flx_variant& parent_id, const std::vector<field_metadata>& fields);

  // Helper methods for build_hierarchy_query refactoring
  flx_string build_vector_distance_expression(const db_search_criteria& criteria, const flx_string& root_table);
  flx_string insert_joins_into_sql(const flx_string& sql, const flx_string& joins, const flx_string& root_table);

  // Helper methods for scan_relations refactoring
  relation_metadata scan_relation_from_model(flx_model* child_model, const flx_string& property_name, const flx_string& parent_table);

  // Helper methods for search refactoring
  void validate_search_prerequisites(flx_list& results);  // Throws db_connection_error
  void execute_search_query(const db_search_criteria& criteria, flx_model& model, std::vector<flxv_map>& rows);  // Throws db_query_error
  void process_search_results(const std::vector<flxv_map>& rows, flx_list& results);

  // Helper methods for load_nested_objects refactoring
  void load_child_relation(flx_model& model, const relation_metadata& rel, const flx_variant& parent_id);
  void recursively_load_grandchildren(flx_model& model);

  // Helper methods for parsing PostgreSQL error messages
  struct fk_violation_info {
    flx_string foreign_key_column;
    flx_string referenced_table;
  };
  fk_violation_info parse_fk_violation(const flx_string& error_msg);

  struct unique_violation_info {
    flx_string column_name;
    flx_string value_str;
  };
  unique_violation_info parse_unique_violation(const flx_string& error_msg);
};

// Implementation


inline db_repository::db_repository(db_connection* conn)
  : connection_(conn)
  , id_column_("id")
  , last_error_("")
  , embedder_(nullptr)
{
}


inline void db_repository::set_id_column(const flx_string& column_name)
{
  id_column_ = column_name;
}


inline void db_repository::set_embedder(flx_semantic_embedder* embedder)
{
  embedder_ = embedder;
}




inline flx_string db_repository::get_last_error() const
{
  return last_error_;
}


inline void db_repository::create(flx_model& model)
{
  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }

  // AUTO-EMBED: Generate semantic embedding if embedder is set
  // If model has semantic properties but no embedder, semantic_embedding will be NULL
  if (embedder_) {
    embedder_->embed_model(model);
  }

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  flx_string sql = build_insert_sql(model);
  flx_string table_name = extract_table_name(model);

  if (!query->prepare(sql)) {
    flx_string error_msg = query->get_last_error();
    // Check if table doesn't exist
    if (error_msg.contains("does not exist") || error_msg.contains("relation") && error_msg.contains("does not exist")) {
      throw db_table_not_found(table_name);
    }
    throw db_prepare_error("Failed to prepare insert", sql, error_msg);
  }

  bind_model_values(query.get(), model);

  if (!query->execute()) {
    flx_string error_msg = query->get_last_error();

    // Check for table not found during execute
    if (error_msg.contains("does not exist") || (error_msg.contains("relation") && error_msg.contains("does not exist"))) {
      throw db_table_not_found(table_name);
    }

    // Check for constraint violations
    if (error_msg.contains("foreign key") || error_msg.contains("violates foreign key constraint")) {
      auto fk_info = parse_fk_violation(error_msg);
      throw db_foreign_key_violation(table_name, fk_info.foreign_key_column,
                                       fk_info.referenced_table, sql, error_msg);
    }

    if (error_msg.contains("unique") || error_msg.contains("duplicate key")) {
      auto unique_info = parse_unique_violation(error_msg);
      // Create variant from parsed value string
      flx_variant dup_value(unique_info.value_str);
      throw db_unique_violation(table_name, unique_info.column_name, dup_value);
    }

    throw db_query_error("Failed to execute insert", sql, error_msg);
  }

  // Get the inserted ID if available
  auto id_query = connection_->create_query();
  id_query->prepare("SELECT lastval()");
  if (id_query->execute() && id_query->next()) {
    auto row = id_query->get_row();
    if (row.find("lastval") != row.end()) {
      model[id_column_] = row["lastval"];
    }
  }

  // AUTO-SAVE: Save nested objects
  save_nested_objects(model);
}


inline void db_repository::update(flx_model& model)
{
  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }

  flx_string table_name = extract_table_name(model);

  // Check if ID is null
  flx_variant id = model[id_column_];
  if (id.in_state() == flx_variant::none) {
    throw db_null_id_error("update", table_name);
  }

  // AUTO-EMBED: Generate semantic embedding if embedder is set
  // If model has semantic properties but no embedder, semantic_embedding will be NULL
  if (embedder_) {
    embedder_->embed_model(model);
  }

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  flx_string sql = build_update_sql(model);
  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare update", sql, query->get_last_error());
  }

  bind_model_values(query.get(), model);
  query->bind("id_value", model[id_column_]);

  if (!query->execute()) {
    throw db_query_error("Failed to execute update", sql, query->get_last_error());
  }

  // AUTO-UPDATE: Delete old nested objects and insert new ones
  auto relations = scan_relations(model);
  if (!relations.empty()) {
    flx_variant parent_id = model[id_column_];

    for (const auto& rel : relations) {
      // Delete old child objects
      flx_string delete_sql = "DELETE FROM " + rel.related_table +
                              " WHERE " + rel.foreign_key_column + " = :parent_id";

      auto del_query = connection_->create_query();
      if (del_query->prepare(delete_sql)) {
        del_query->bind("parent_id", parent_id);
        del_query->execute();
      }
    }

    // Insert new child objects
    save_nested_objects(model);
  }
}


inline void db_repository::remove(flx_model& model)
{
  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }

  flx_string table_name = extract_table_name(model);

  // Check if ID is null
  flx_variant id = model[id_column_];
  if (id.in_state() == flx_variant::none) {
    throw db_null_id_error("delete", table_name);
  }

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  flx_string sql = flx_string("DELETE FROM ") + table_name + " WHERE " + id_column_ + " = :id_value";

  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare delete", sql, query->get_last_error());
  }

  query->bind("id_value", model[id_column_]);

  if (!query->execute()) {
    throw db_query_error("Failed to execute delete", sql, query->get_last_error());
  }
}


inline void db_repository::find_by_id(long long id, flx_model& model)
{
  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }

  flx_string table_name = extract_table_name(model);

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  flx_string sql = build_select_sql(model, id_column_ + " = :id_value");

  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare select", sql, query->get_last_error());
  }

  query->bind("id_value", flx_variant(id));

  if (!query->execute()) {
    throw db_query_error("Failed to execute select", sql, query->get_last_error());
  }

  if (query->next()) {
    auto row = query->get_row();

    // First: Copy ALL raw columns (including "id" and other non-property columns)
    for (const auto& pair : row) {
      model[pair.first] = pair.second;
    }
    // Second: Let read_row create nested structures for properties with paths
    model.read_row(row);

    // AUTO-LOAD: Load nested objects
    load_nested_objects(model);

    return;
  }

  // Record not found - throw exception
  throw db_record_not_found(table_name, id);
}


inline void db_repository::find_all(flx_list& results)
{
  find_where("", results);
}


inline void db_repository::find_where(const flx_string& condition, flx_list& results)
{
  results.clear();

  // Get sample model from list factory
  auto sample = results.factory();
  if (sample.is_null()) {
    throw db_query_error("Failed to create sample model from list factory");
  }
  flx_model& model = *sample;

  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  flx_string sql = build_select_sql(model, condition);

  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare select", sql, query->get_last_error());
  }

  if (!query->execute()) {
    throw db_query_error("Failed to execute select", sql, query->get_last_error());
  }

  // Make a copy of rows to avoid issues with query object reuse
  std::vector<flxv_map> rows_copy = query->get_all_rows();

  // Use index-based iteration
  for (size_t i = 0; i < rows_copy.size(); ++i) {
    const auto& row = rows_copy[i];

    flx_model model;
    // First: Copy ALL raw columns (including "id" and other non-property columns)
    for (const auto& pair : row) {
      model[pair.first] = pair.second;
    }
    // Second: Let read_row create nested structures for properties with paths
    model.read_row(row);

    // AUTO-LOAD: Load nested objects for each result
    load_nested_objects(model);

    // Add new element to results and copy data
    results.add_element();
    *results.back() = *model;  // Copy map data
    results.back().resync();   // Resync child pointers
  }
}


inline bool db_repository::table_exists(flx_model& model)
{
  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Database not connected";
    return false;
  }

  auto query = connection_->create_query();
  query->prepare("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = :table_name)");
  query->bind("table_name", flx_variant(extract_table_name(model)));

  if (query->execute() && query->next()) {
    auto row = query->get_row();
    return row["exists"].bool_value();
  }

  return false;
}


inline void db_repository::create_table(flx_model& model)
{
  // Scan fields to get proper types from metadata
  auto fields = scan_fields(model);

  if (fields.empty()) {
    flx_string table_name = extract_table_name(model);
    throw db_no_fields_error(table_name);
  }

  flx_string table_name = extract_table_name(model);
  flx_string sql = flx_string("CREATE TABLE IF NOT EXISTS ") + table_name + " (\n";
  sql += "  " + id_column_ + " SERIAL PRIMARY KEY";

  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;

    flx_string sql_type = get_sql_type_from_state(field.type, field.column_name);
    sql += ",\n  " + field.column_name + " " + sql_type;

    // Add NOT NULL constraint if specified
    if (field.is_not_null) {
      sql += " NOT NULL";
    }

    // Add UNIQUE constraint if specified
    if (field.is_unique) {
      sql += " UNIQUE";
    }
  }

  sql += "\n)";

  auto query = connection_->create_query();
  query->prepare(sql);

  if (!query->execute()) {
    throw db_query_error("Failed to create table", sql, query->get_last_error());
  }

  // Add foreign key constraints with CASCADE DELETE
  for (const auto& field : fields) {
    if (field.is_foreign_key && !field.foreign_table.empty()) {
      flx_string constraint_name = table_name + "_" + field.column_name + "_fkey";
      flx_string fk_sql = "ALTER TABLE " + table_name +
                          " ADD CONSTRAINT " + constraint_name +
                          " FOREIGN KEY (" + field.column_name + ")" +
                          " REFERENCES " + field.foreign_table + "(id)" +
                          " ON DELETE CASCADE";

      auto fk_query = connection_->create_query();
      if (!fk_query->prepare(fk_sql)) {
        throw db_prepare_error("Failed to prepare FK constraint", fk_sql, fk_query->get_last_error());
      }

      if (!fk_query->execute()) {
        // Ignore if constraint already exists
      }
    }
  }

  // Ensure pgvector extension exists if model has semantic properties
  if (has_semantic_properties(model)) {
    ensure_pgvector_extension();
    create_semantic_index(table_name);
  }
}


inline void db_repository::drop_table(flx_model& model)
{
  auto query = connection_->create_query();
  flx_string sql = flx_string("DROP TABLE IF EXISTS ") + extract_table_name(model);
  query->prepare(sql);

  if (!query->execute()) {
    throw db_query_error("Failed to drop table", sql, query->get_last_error());
  }
}


inline flx_string db_repository::build_insert_sql(const flx_model& model)
{
  auto fields = scan_fields(model);

  flx_string sql = flx_string("INSERT INTO ") + extract_table_name(model) + " (";
  flx_string values = " VALUES (";

  bool first = true;
  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;

    if (!first) {
      sql += ", ";
      values += ", ";
    }
    sql += field.column_name;
    values += flx_string(":") + field.column_name;  // Use column name for binding
    first = false;
  }

  sql += ")" + values + ")";
  return sql;
}


inline flx_string db_repository::build_update_sql(const flx_model& model)
{
  auto fields = scan_fields(model);

  flx_string sql = flx_string("UPDATE ") + extract_table_name(model) + " SET ";

  bool first = true;
  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;

    if (!first) sql += ", ";
    sql += field.column_name + " = :" + field.column_name;  // Use column name for binding (consistent with INSERT)
    first = false;
  }

  sql += " WHERE " + id_column_ + " = :id_value";
  return sql;
}


inline flx_string db_repository::build_select_sql(flx_model& model, const flx_string& where_clause)
{
  // Build explicit column list from model properties (performance: avoid loading unused columns like semantic_embedding)
  auto fields = scan_fields(model);
  flx_string column_list = id_column_;  // Always include primary key

  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;  // Already added
    column_list += ", " + field.column_name;
  }

  flx_string sql = flx_string("SELECT ") + column_list + " FROM " + extract_table_name(model);

  if (!where_clause.empty()) {
    sql += " WHERE " + where_clause;
  }

  return sql;
}


inline void db_repository::bind_model_values(db_query* query, flx_model& model)
{
  auto fields = scan_fields(model);
  const auto& properties = model.get_properties();

  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;

    // Access value directly from property using access() method
    flx_variant value;

    auto prop_it = properties.find(field.property_name);
    if (prop_it != properties.end()) {
      // Property exists - get its value via access()
      value = prop_it->second->access();
    } else {
      // Property not found - use NULL
      value = flx_variant();
    }

    // DEBUG
    // std::cout << "[bind] property=" << field.property_name.c_str()
    //           << " column=" << field.column_name.c_str()
    //           << " value=" << (value.is_null() ? "NULL" : "HAS_VALUE") << std::endl;

    query->bind(field.column_name, value);  // Bind with column_name (DB column)
  }
}

// Helper function to access nested values (handles "/" separator like flx_property_i::access())

inline flx_variant db_repository::access_nested_value(flx_model& model, const flx_string& property_name)
{
  // Check if name contains "/" for nested access
  if (property_name.contains("/")) {
    std::vector<flx_string> parts = property_name.split("/");

    // Navigate through nested maps
    flx_variant* current_var = &model[parts[0]];

    for (size_t i = 1; i < parts.size(); ++i) {
      // If not a map, return null variant
      if (current_var->in_state() != flx_variant::map_state) {
        return flx_variant();
      }

      flxv_map& current_map = current_var->to_map();

      // Check if key exists
      if (current_map.find(parts[i]) == current_map.end()) {
        return flx_variant();
      }

      current_var = &current_map[parts[i]];
    }

    return *current_var;
  }

  return model[property_name];
}


inline void db_repository::map_to_model(const flxv_map& row, flx_model& model)
{
  for (const auto& pair : row) {
    model[pair.first] = pair.second;
  }
}


inline flx_string db_repository::get_sql_type(const flx_variant& value)
{
  if (value.is_string()) return "VARCHAR(255)";
  if (value.is_int()) return "BIGINT";
  if (value.is_double()) return "DOUBLE PRECISION";
  if (value.is_bool()) return "BOOLEAN";
  return "TEXT";
}


inline flx_string db_repository::get_sql_type_from_state(flx_variant::state state, const flx_string& column_name)
{
  // Special handling for semantic_embedding column (pgvector halfvec type)
  if (column_name == "semantic_embedding" && state == flx_variant::vector_state) {
    return "halfvec(3072)";  // OpenAI text-embedding-3-large dimensions
  }

  switch (state) {
    case flx_variant::string_state:
      return "VARCHAR(255)";
    case flx_variant::int_state:
      return "BIGINT";
    case flx_variant::double_state:
      return "DOUBLE PRECISION";
    case flx_variant::bool_state:
      return "BOOLEAN";
    case flx_variant::map_state:
      return "JSONB";
    case flx_variant::vector_state:
      return "JSONB";  // Regular vectors use JSONB
    default:
      return "TEXT";
  }
}

// Metadata-based automation implementations


inline std::vector<db_repository::field_metadata> db_repository::scan_fields(const flx_model& model)
{
  std::vector<field_metadata> fields;

  const auto& properties = model.get_properties();

  for (const auto& prop_pair : properties) {
    const flx_string& prop_name = prop_pair.first;
    flx_property_i* prop = prop_pair.second;

    // Get metadata first
    const flxv_map& meta = prop->get_meta();

    // REQUIRED: column metadata must exist (no fallback!)
    if (meta.find("column") == meta.end()) {
      // std::cout << "[scan_fields]   SKIPPED (no column metadata)" << std::endl;
      continue;
    }

    field_metadata field;
    field.property_name = prop_name;  // C++ name for main model properties
    field.cpp_name = prop_name;       // Same as property_name for main models
    field.column_name = meta.at("column").string_value();

    // Check for primary key (value contains table name)
    field.is_primary_key = (meta.find("primary_key") != meta.end());

    // Check for foreign key
    if (meta.find("foreign_key") != meta.end()) {
      field.is_foreign_key = true;
      field.foreign_table = meta.at("foreign_key").string_value();
    } else {
      field.is_foreign_key = false;
    }

    // Check for UNIQUE constraint
    field.is_unique = (meta.find("unique") != meta.end() &&
                       meta.at("unique").string_value() == "true");

    // Check for NOT NULL constraint
    field.is_not_null = (meta.find("not_null") != meta.end() &&
                         meta.at("not_null").string_value() == "true");

    // Get type from property definition
    field.type = prop->get_variant_type();

    fields.push_back(field);
  }

  return fields;
}


inline std::vector<db_repository::relation_metadata> db_repository::scan_relations(const flx_model& model)
{
  std::vector<relation_metadata> relations;
  flx_string parent_table = extract_table_name(model);

  // Process flxp_model (1:1 relations)
  const auto& children = model.get_children();
  for (const auto& [child_name, child_ptr] : children) {
    auto rel = scan_relation_from_model(child_ptr, child_name, parent_table);
    if (!rel.related_table.empty() && !rel.foreign_key_column.empty()) {
      relations.push_back(rel);
    }
  }

  // Process flxp_model_list (1:N relations)
  const auto& model_lists = model.get_model_lists();
  for (const auto& [list_name, list_ptr] : model_lists) {
    if (list_ptr == nullptr) continue;
    auto sample_elem = list_ptr->factory();  // Get sample element for structure
    if (sample_elem.is_null()) continue;

    auto rel = scan_relation_from_model(&(*sample_elem), list_name, parent_table);
    if (!rel.related_table.empty() && !rel.foreign_key_column.empty()) {
      relations.push_back(rel);
    }
  }

  return relations;
}


inline void db_repository::auto_configure(flx_model& model)
{
  // Scan fields from metadata
  auto fields = scan_fields(model);

  // Find primary key
  for (const auto& field : fields) {
    if (field.is_primary_key) {
      id_column_ = field.column_name;
    }
  }

  // If no primary key found, default to "id"
  if (id_column_.empty()) {
    id_column_ = "id";
  }
}


inline flx_string db_repository::extract_table_name(const flx_model& model)
{
  const auto& properties = model.get_properties();

  // Find property with primary_key metadata
  for (const auto& prop_pair : properties) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    if (meta.find("primary_key") != meta.end()) {
      // Value of primary_key is the table name
      return meta.at("primary_key").string_value();
    }
  }

  // Fallback: empty string (will cause error)
  return "";
}


inline void db_repository::search(const db_search_criteria& criteria, flx_list& results)
{
  validate_search_prerequisites(results);

  auto sample = results.factory();  // Keep sample alive for entire function
  if (sample.is_null()) {
    throw db_query_error("Failed to create sample model from list factory");
  }
  flx_model& model = *sample;

  std::vector<flxv_map> rows;
  execute_search_query(criteria, model, rows);

  process_search_results(rows, results);
}

// Hierarchical operations implementations

// Helper function to navigate nested map using slash-separated path
inline flx_variant* navigate_nested_map(flxv_map& root, const flx_string& path) {
  if (!path.contains("/")) {
    // Simple key, direct lookup
    if (root.find(path) != root.end()) {
      return &root[path];
    }
    return nullptr;
  }

  // Navigate through nested structure
  std::vector<flx_string> parts = path.split("/");
  flx_variant* current = &root[parts[0]];

  for (size_t i = 1; i < parts.size(); ++i) {
    if (current->in_state() != flx_variant::map_state) {
      return nullptr;  // Can't navigate further
    }

    flxv_map& current_map = current->to_map();
    if (current_map.find(parts[i]) == current_map.end()) {
      return nullptr;  // Path doesn't exist
    }
    current = &current_map[parts[i]];
  }

  return current;
}


inline void db_repository::save_nested_objects(flx_model& model)
{
  save_nested_objects_impl(model);
}


inline void db_repository::save_nested_objects_impl(flx_model& model)
{
  auto relations = scan_relations(model);
  if (relations.empty()) return;

  flx_variant parent_id = validate_parent_id(model);
  flx_string parent_table = extract_table_name(model);

  for (const auto& rel : relations) {
    flx_model* typed_child = find_typed_child_model(model, rel);
    if (typed_child == nullptr) continue;

    auto child_fields = scan_child_field_metadata(typed_child);
    auto source_info = determine_child_source(model, rel);
    if (source_info.item_count == 0) continue;

    flx_string insert_sql = build_child_insert_sql(rel, child_fields);

    for (size_t i = 0; i < source_info.item_count; ++i) {
      flx_model* item = (source_info.single_child != nullptr) ? source_info.single_child : source_info.model_list->get_model_at(i);
      if (item == nullptr) continue;

      try {
        auto query = connection_->create_query();
        if (!query->prepare(insert_sql)) {
          throw db_prepare_error("Failed to prepare child insert", insert_sql, query->get_last_error());
        }

        bind_and_execute_child_insert(query.get(), rel, item, parent_id, child_fields);
        update_child_foreign_key(item, rel, parent_id, child_fields);

        save_nested_objects_impl(*item);  // Recursive save grandchildren
      }
      catch (const db_exception& e) {
        // Wrap any db_exception in db_nested_save_error to show hierarchy context
        throw db_nested_save_error(parent_table, rel.related_table, e.what());
      }
    }
  }
}

// ============================================================================
// Helper Methods for save_nested_objects_impl (SRP Refactoring)
// ============================================================================

inline flx_variant db_repository::validate_parent_id(flx_model& model)
{
  flx_variant parent_id = model[id_column_];
  if (parent_id.in_state() == flx_variant::none) {
    flx_string table_name = extract_table_name(model);
    throw db_null_id_error("save nested objects", table_name);
  }
  return parent_id;
}

inline flx_model* db_repository::find_typed_child_model(flx_model& parent, const relation_metadata& rel)
{
  // Try single nested model first (flxp_model)
  const auto& children = parent.get_children();
  auto child_it = children.find(rel.property_name);
  if (child_it != children.end() && child_it->second != nullptr) {
    return child_it->second;
  }

  // Try model_lists (flxp_model_list)
  const auto& lists = parent.get_model_lists();
  auto list_it = lists.find(rel.property_name);
  if (list_it != lists.end() && list_it->second != nullptr && list_it->second->list_size() > 0) {
    return list_it->second->get_model_at(0);  // Get first item for schema reflection
  }

  return nullptr;  // Can't determine schema
}

inline std::vector<db_repository::field_metadata> db_repository::scan_child_field_metadata(flx_model* typed_child_model)
{
  std::vector<field_metadata> child_fields;

  const auto& child_props = typed_child_model->get_properties();
  for (const auto& prop_pair : child_props) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Only include properties with "column" metadata
    if (meta.find("column") == meta.end()) {
      continue;
    }

    field_metadata field;
    field.cpp_name = prop_pair.first;  // C++ name for map lookup

    // Use fieldname for XML path navigation (legacy)
    if (meta.find("fieldname") != meta.end()) {
      field.property_name = meta.at("fieldname").string_value();
    } else {
      field.property_name = prop_pair.first;  // Fallback to C++ name
    }

    field.column_name = meta.at("column").string_value();
    field.type = prop->get_variant_type();

    child_fields.push_back(field);
  }

  return child_fields;
}

inline db_repository::child_source_info db_repository::determine_child_source(flx_model& model, const relation_metadata& rel)
{
  child_source_info info{nullptr, nullptr, 0};

  const auto& children = model.get_children();
  auto child_it = children.find(rel.property_name);
  if (child_it != children.end()) {
    info.single_child = child_it->second;
    info.item_count = 1;
    return info;
  }

  const auto& lists = model.get_model_lists();
  auto list_it = lists.find(rel.property_name);
  if (list_it != lists.end()) {
    info.model_list = list_it->second;
    info.item_count = list_it->second->list_size();
  }

  return info;
}

inline flx_string db_repository::build_child_insert_sql(const relation_metadata& rel, const std::vector<field_metadata>& fields)
{
  flx_string sql = "INSERT INTO " + rel.related_table + " (";
  flx_string values = " VALUES (";
  bool first = true;

  // Add foreign key column first
  sql += rel.foreign_key_column;
  values += flx_string(":") + rel.foreign_key_column;
  first = false;

  // Add fields using column metadata
  for (const auto& field : fields) {
    // Skip ID and FK columns (ID auto-generated, FK already added)
    if (field.column_name == id_column_ || field.column_name == rel.foreign_key_column) {
      continue;
    }

    if (!first) {
      sql += ", ";
      values += ", ";
    }
    sql += field.column_name;
    values += flx_string(":") + field.column_name;
    first = false;
  }

  return sql + ")" + values + ")";
}

inline void db_repository::bind_and_execute_child_insert(db_query* query, const relation_metadata& rel,
                                                           flx_model* item_model, const flx_variant& parent_id,
                                                           const std::vector<field_metadata>& fields)
{
  // Bind foreign key first
  query->bind(rel.foreign_key_column, parent_id);

  // Bind field values
  const auto& item_properties = item_model->get_properties();
  for (const auto& field : fields) {
    // Skip ID and FK columns
    if (field.column_name == id_column_ || field.column_name == rel.foreign_key_column) {
      continue;
    }

    // Use cpp_name to find property in model
    auto prop_it = item_properties.find(field.cpp_name);
    flx_variant value;
    if (prop_it != item_properties.end()) {
      value = prop_it->second->access();
    }
    query->bind(field.column_name, value);
  }

  // Execute insert
  if (!query->execute()) {
    throw db_query_error("Failed to execute child insert", "", query->get_last_error());
  }
}

inline long long db_repository::retrieve_inserted_id()
{
  auto id_query = connection_->create_query();
  id_query->prepare("SELECT lastval()");
  if (id_query->execute() && id_query->next()) {
    auto row = id_query->get_row();
    if (row.find("lastval") != row.end()) {
      return row["lastval"].int_value();
    }
  }
  return -1;  // Failed to retrieve ID
}

inline void db_repository::update_child_foreign_key(flx_model* item_model, const relation_metadata& rel,
                                                      const flx_variant& parent_id, const std::vector<field_metadata>& fields)
{
  // Set inserted ID in model
  long long inserted_id = retrieve_inserted_id();
  if (inserted_id != -1) {
    (*item_model)[id_column_] = flx_variant(inserted_id);
  }

  // Set foreign key value in model
  for (const auto& field : fields) {
    if (field.column_name == rel.foreign_key_column) {
      (*item_model)[field.cpp_name] = parent_id;
      break;
    }
  }
}

// ============================================================================
// Helper Methods for build_hierarchy_query (SRP Refactoring)
// ============================================================================

inline flx_string db_repository::build_vector_distance_expression(const db_search_criteria& criteria, const flx_string& root_table)
{
  if (!criteria.has_vector_search()) return "";

  const auto& vec_search = criteria.get_vector_search();

  // Build vector literal: [v1, v2, v3, ...]
  flx_string vector_literal = "[";
  for (size_t i = 0; i < vec_search.query_embedding.size(); ++i) {
    if (i > 0) vector_literal += ", ";
    vector_literal += flx_string(std::to_string(vec_search.query_embedding[i]).c_str());
  }
  vector_literal += "]";

  // Return distance expression: embedding_field <-> '[...]'::vector AS distance
  return root_table + "." + vec_search.embedding_field + " <-> '" + vector_literal + "'::vector AS distance";
}

inline flx_string db_repository::insert_joins_into_sql(const flx_string& sql, const flx_string& joins, const flx_string& root_table)
{
  flx_string from_marker = "FROM " + root_table;
  size_t from_pos = sql.find(from_marker.c_str());

  if (from_pos == flx_string::npos) return sql;  // FROM not found, return unchanged

  size_t insert_pos = from_pos + from_marker.length();

  // Find first clause after FROM (WHERE, ORDER BY, or LIMIT)
  size_t where_pos = sql.find(" WHERE");
  size_t order_pos = sql.find(" ORDER BY");
  size_t limit_pos = sql.find(" LIMIT");

  // Determine minimum valid position after insert_pos
  size_t next_clause = flx_string::npos;
  if (where_pos != flx_string::npos && where_pos > insert_pos) next_clause = where_pos;
  if (order_pos != flx_string::npos && order_pos > insert_pos && (next_clause == flx_string::npos || order_pos < next_clause)) next_clause = order_pos;
  if (limit_pos != flx_string::npos && limit_pos > insert_pos && (next_clause == flx_string::npos || limit_pos < next_clause)) next_clause = limit_pos;

  // Insert JOINs at appropriate position
  return sql.substr(0, insert_pos) + joins + sql.substr(insert_pos);
}

// ============================================================================
// Helper Methods for scan_relations (SRP Refactoring)
// ============================================================================

inline db_repository::relation_metadata db_repository::scan_relation_from_model(flx_model* child_model, const flx_string& property_name, const flx_string& parent_table)
{
  relation_metadata rel;
  rel.property_name = property_name;

  if (child_model == nullptr) return rel;

  // Scan child's properties for metadata
  const auto& child_properties = child_model->get_properties();
  for (const auto& [prop_name, prop] : child_properties) {
    const flxv_map& meta = prop->get_meta();

    // Find primary_key for table name
    if (meta.find("primary_key") != meta.end()) {
      rel.related_table = meta.at("primary_key").string_value();
    }

    // Find foreign_key pointing back to parent
    if (meta.find("foreign_key") != meta.end()) {
      flx_string fk_table = meta.at("foreign_key").string_value();
      if (fk_table == parent_table && meta.find("column") != meta.end()) {
        rel.foreign_key_column = meta.at("column").string_value();
      }
    }
  }

  return rel;
}

// ============================================================================
// Helper Methods for search (SRP Refactoring)
// ============================================================================

inline void db_repository::validate_search_prerequisites(flx_list& results)
{
  results.clear();

  if (!connection_ || !connection_->is_connected()) {
    throw db_connection_error("Database not connected");
  }
}

inline void db_repository::execute_search_query(const db_search_criteria& criteria, flx_model& model, std::vector<flxv_map>& rows)
{
  db_query_builder builder;
  builder.from(extract_table_name(model));
  criteria.apply_to(builder);
  flx_string sql = builder.build_select();

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query");
  }

  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare search query", sql, query->get_last_error());
  }

  const auto& params = builder.get_parameters();
  for (const auto& param : params) {
    query->bind(param.first, param.second);
  }

  if (!query->execute()) {
    throw db_query_error("Failed to execute search", sql, query->get_last_error());
  }

  rows = query->get_all_rows();
}

inline void db_repository::process_search_results(const std::vector<flxv_map>& rows, flx_list& results)
{
  for (const auto& row : rows) {
    flx_model model;
    for (const auto& pair : row) {
      model[pair.first] = pair.second;
    }
    model.read_row(row);
    load_nested_objects(model);

    results.add_element();
    *results.back() = *model;
    results.back().resync();
  }
}

// ============================================================================
// Helper Methods for load_nested_objects (SRP Refactoring)
// ============================================================================

inline void db_repository::load_child_relation(flx_model& model, const relation_metadata& rel, const flx_variant& parent_id)
{
  flx_string child_sql = "SELECT * FROM " + rel.related_table + " WHERE " + rel.foreign_key_column + " = :parent_id";

  auto child_query = connection_->create_query();
  if (!child_query->prepare(child_sql)) return;

  child_query->bind("parent_id", parent_id);
  if (!child_query->execute()) return;

  auto child_rows = child_query->get_all_rows();

  const auto& children = model.get_children();
  const auto& lists = model.get_model_lists();
  bool is_single_model = (children.find(rel.property_name) != children.end());

  if (is_single_model && !child_rows.empty()) {
    model[rel.property_name] = flx_variant(child_rows[0]);
  } else {
    flxv_vector children_vec;
    for (const auto& child_row : child_rows) {
      children_vec.push_back(flx_variant(child_row));
    }
    model[rel.property_name] = flx_variant(children_vec);
  }
}

inline void db_repository::recursively_load_grandchildren(flx_model& model)
{
  const auto& children = model.get_children();
  for (const auto& [child_name, child_ptr] : children) {
    if (child_ptr != nullptr) {
      load_nested_objects(*child_ptr);
    }
  }

  const auto& lists = model.get_model_lists();
  for (const auto& [list_name, list_ptr] : lists) {
    if (list_ptr != nullptr) {
      for (size_t i = 0; i < list_ptr->list_size(); ++i) {
        flx_model* child_model = list_ptr->get_model_at(i);
        if (child_model != nullptr) {
          load_nested_objects(*child_model);
        }
      }
    }
  }
}


inline void db_repository::load_nested_objects(flx_model& model)
{
  auto relations = scan_relations(model);
  if (relations.empty()) return;

  flx_variant parent_id = model[id_column_];
  if (parent_id.in_state() == flx_variant::none) return;  // No ID, nothing to load

  for (const auto& rel : relations) {
    load_child_relation(model, rel, parent_id);
  }

  model.resync();  // Sync typed child models to loaded data
  recursively_load_grandchildren(model);
}


inline flx_string db_repository::build_join_sql(const flx_model& model, const std::vector<relation_metadata>& relations)
{
  flx_string joins;

  for (const auto& rel : relations) {
    // LEFT JOIN related_table AS relationname ON table.id = relationname.foreign_key
    joins += " LEFT JOIN " + rel.related_table + " AS " + rel.property_name;
    joins += " ON " + extract_table_name(model) + "." + id_column_ + " = ";
    joins += rel.property_name + "." + rel.foreign_key_column;
  }

  return joins;
}


inline void db_repository::map_joined_results(const flxv_map& row, flx_model& model, const std::vector<relation_metadata>& relations)
{
  // Map parent fields
  for (const auto& pair : row) {
    flx_string key = pair.first.c_str();

    // Check if this is an aliased field (contains _)
    bool is_aliased = false;
    for (const auto& rel : relations) {
      flx_string prefix = rel.property_name.c_str() + flx_string("_");
      if (key.starts_with(prefix)) {
        // This is a child field: relationname_fieldname
        is_aliased = true;

        // Extract field name
        flx_string field_name = key.substr(prefix.length());

        // TODO: Map to nested object in model
        // For now we skip this as we need the full child aggregation logic

        break;
      }
    }

    if (!is_aliased) {
      // Parent field - map directly
      model[pair.first] = pair.second;
    }
  }
}

// Hierarchical search helper implementations


inline flx_string db_repository::find_primary_key_column(flx_model& model)
{
  // Scan properties for primary_key metadata
  const auto& properties = model.get_properties();

  for (const auto& prop_pair : properties) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Check if this property has primary_key metadata (value = table name)
    if (meta.find("primary_key") != meta.end()) {
      // Found primary key - return its column name
      if (meta.find("column") != meta.end()) {
        return meta.at("column").string_value();
      }
    }
  }

  // Fallback to default "id" column
  return "id";
}


inline std::set<flx_string> db_repository::collect_all_table_names(flx_model& model, const flx_string& root_table)
{
  std::set<flx_string> tables;
  tables.insert(root_table);

  // Iterate over all child models (flxp_model - 1:1)
  const auto& children = model.get_children();
  for (const auto& child_pair : children) {
    flx_model* child = child_pair.second;
    if (child == nullptr) continue;

    // Extract table name from child's primary_key metadata
    flx_string child_table;
    const auto& child_properties = child->get_properties();
    for (const auto& [prop_name, prop] : child_properties) {
      const flxv_map& meta = prop->get_meta();
      if (meta.find("primary_key") != meta.end()) {
        child_table = meta.at("primary_key").string_value();
        break;
      }
    }

    if (!child_table.empty()) {
      // Recursively collect tables from this child
      auto child_tables = collect_all_table_names(*child, child_table);
      tables.insert(child_tables.begin(), child_tables.end());
    }
  }

  // Iterate over all model_lists (flxp_model_list - 1:N)
  const auto& model_lists = model.get_model_lists();
  for (const auto& list_pair : model_lists) {
    flx_list* list = list_pair.second;
    if (list == nullptr || list->list_size() == 0) continue;

    // Get first element to inspect structure
    flx_model* first_elem = list->get_model_at(0);
    if (first_elem == nullptr) continue;

    // Extract table name from element's primary_key metadata
    flx_string child_table;
    const auto& elem_properties = first_elem->get_properties();
    for (const auto& [prop_name, prop] : elem_properties) {
      const flxv_map& meta = prop->get_meta();
      if (meta.find("primary_key") != meta.end()) {
        child_table = meta.at("primary_key").string_value();
        break;
      }
    }

    if (!child_table.empty()) {
      // Always add the table for the list
      tables.insert(child_table);

      // Recursively collect nested tables
      auto child_tables = collect_all_table_names(*first_elem, child_table);
      tables.insert(child_tables.begin(), child_tables.end());
    }
  }

  return tables;
}


inline flx_string db_repository::build_joins_recursive(flx_model& model, const flx_string& parent_table)
{
  flx_string joins = "";

  // Get parent's primary key column
  flx_string parent_pk = find_primary_key_column(model);

  // Get all properties to access metadata
  const auto& properties = model.get_properties();

  // Process child models (flxp_model)
  const auto& children = model.get_children();
  for (const auto& child_pair : children) {
    const flx_string& child_fieldname = child_pair.first;
    flx_model* child = child_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(child_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Check if this child has table and foreign_key metadata
      if (meta.find("table") != meta.end() && meta.find("foreign_key") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();
        flx_string foreign_key = meta.at("foreign_key").string_value();

        // Build LEFT JOIN clause
        joins += " LEFT JOIN " + child_table +
                 " ON " + parent_table + "." + parent_pk +
                 " = " + child_table + "." + foreign_key;

        // Recursively build joins for this child
        joins += build_joins_recursive(*child, child_table);
      }
    }
  }

  // Process model_lists (flxp_model_list)
  const auto& model_lists = model.get_model_lists();
  for (const auto& list_pair : model_lists) {
    const flx_string& list_fieldname = list_pair.first;
    flx_list* list = list_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(list_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Check if this list has table and foreign_key metadata
      if (meta.find("table") != meta.end() && meta.find("foreign_key") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();
        flx_string foreign_key = meta.at("foreign_key").string_value();

        // Build LEFT JOIN clause
        joins += " LEFT JOIN " + child_table +
                 " ON " + parent_table + "." + parent_pk +
                 " = " + child_table + "." + foreign_key;

        // Recursively build joins for list elements (if list has elements)
        if (list->list_size() > 0) {
          flx_model* first_elem = list->get_model_at(0);
          if (first_elem) {
            joins += build_joins_recursive(*first_elem, child_table);
          }
        }
      }
    }
  }

  return joins;
}


inline flx_string db_repository::build_id_selects_recursive(flx_model& model, const flx_string& table_name)
{
  flx_string selects = "";

  // Get this table's primary key column
  flx_string pk_column = find_primary_key_column(model);

  // Add this table's ID select: table.pk_column AS "table.pk_column"
  selects += table_name + "." + pk_column + " AS \"" + table_name + "." + pk_column + "\"";

  // Get all properties to access metadata
  const auto& properties = model.get_properties();

  // Process child models (flxp_model)
  const auto& children = model.get_children();
  for (const auto& child_pair : children) {
    const flx_string& child_fieldname = child_pair.first;
    flx_model* child = child_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(child_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Check if this child has table metadata
      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();

        // Add comma before child selects
        selects += ", ";

        // Recursively build ID selects for this child
        selects += build_id_selects_recursive(*child, child_table);
      }
    }
  }

  // Process model_lists (flxp_model_list)
  const auto& model_lists = model.get_model_lists();
  for (const auto& list_pair : model_lists) {
    const flx_string& list_fieldname = list_pair.first;
    flx_list* list = list_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(list_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Check if this list has table metadata
      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();

        // Add comma before child selects
        selects += ", ";

        // Recursively build ID selects for list elements (if list has elements)
        if (list->list_size() > 0) {
          flx_model* first_elem = list->get_model_at(0);
          if (first_elem) {
            selects += build_id_selects_recursive(*first_elem, child_table);
          }
        } else {
          // List is empty - still add ID select for this table
          // Use "id" as default primary key (most common case)
          selects += child_table + ".id AS \"" + child_table + ".id\"";
        }
      }
    }
  }

  return selects;
}


inline flx_string db_repository::build_hierarchy_query(flx_model& model, const flx_string& root_table, const db_search_criteria& criteria, db_query_builder& builder)
{
  // Build SELECT clause with all IDs
  flx_string id_selects = build_id_selects_recursive(model, root_table);

  // Prepend vector distance expression if semantic search is active
  flx_string distance_expr = build_vector_distance_expression(criteria, root_table);
  if (!distance_expr.empty()) {
    id_selects = distance_expr + ", " + id_selects;
  }

  // Build JOIN clauses
  flx_string joins = build_joins_recursive(model, root_table);

  // Configure query builder and apply criteria
  builder.select(id_selects);
  builder.from(root_table);
  criteria.apply_to(builder);

  // Build SQL and insert JOINs at correct position
  flx_string sql = builder.build_select();
  return insert_joins_into_sql(sql, joins, root_table);
}


inline std::map<flx_string, std::set<long long>> db_repository::parse_hierarchy_results(const std::vector<flxv_map>& rows, flx_model& model, const flx_string& root_table)
{
  std::map<flx_string, std::set<long long>> id_sets;

  // Collect all table names
  auto tables = collect_all_table_names(model, root_table);

  // Initialize empty sets for all tables
  for (const auto& table : tables) {
    id_sets[table] = std::set<long long>();
  }

  // Process each result row
  for (const auto& row : rows) {
    // For each table, extract its ID from the row
    for (const auto& table : tables) {
      // Construct the qualified column name: "table.pk_column"
      flx_string pk_column = find_primary_key_column(model);  // Get PK from model

      // Note: We need to find the correct model for each table
      // For now, assume all models use "id" as primary key
      flx_string qualified_column = table + ".id";

      // Check if this column exists in the row
      if (row.find(qualified_column) != row.end()) {
        const flx_variant& id_variant = row.at(qualified_column);

        // Skip NULL values (LEFT JOIN result with no matching child)
        if (id_variant.in_state() != flx_variant::none && !id_variant.is_null()) {
          // Convert to long long and add to set
          long long id = id_variant.int_value();
          id_sets[table].insert(id);
        }
      }
    }
  }

  return id_sets;
}


inline std::map<flx_string, std::map<long long, flxv_map>> db_repository::batch_load_rows(const std::map<flx_string, std::set<long long>>& id_sets)
{
  std::map<flx_string, std::map<long long, flxv_map>> all_rows;

  // For each table with IDs
  for (const auto& [table, ids] : id_sets) {
    if (ids.empty()) {
      continue;  // Skip tables with no IDs
    }

    // Build SELECT * FROM table WHERE id IN (:id0, :id1, :id2, ...)
    flx_string sql = "SELECT * FROM " + table + " WHERE id IN (";

    std::vector<long long> id_vector(ids.begin(), ids.end());
    for (size_t i = 0; i < id_vector.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += ":id" + flx_string(std::to_string(i).c_str());
    }
    sql += ")";

    // Execute query
    auto query = connection_->create_query();
    if (!query->prepare(sql)) {
      continue;
    }

    // Bind ID parameters
    for (size_t i = 0; i < id_vector.size(); ++i) {
      flx_string param_name = "id" + flx_string(std::to_string(i).c_str());
      query->bind(param_name, flx_variant(id_vector[i]));
    }

    if (!query->execute()) {
      continue;
    }

    // Store rows: map<id, row>
    while (query->next()) {
      flxv_map row = query->get_row();

      // Extract ID from row
      if (row.find("id") != row.end()) {
        long long id = row["id"].int_value();
        all_rows[table][id] = row;
      }
    }
  }

  return all_rows;
}


inline void db_repository::construct_tree_recursive(flx_model& model, const flx_string& table_name, long long id, const std::map<flx_string, std::map<long long, flxv_map>>& all_rows)
{
  // Get the row data for this model
  auto table_it = all_rows.find(table_name);
  if (table_it == all_rows.end()) {
    return;  // No data for this table
  }

  auto row_it = table_it->second.find(id);
  if (row_it == table_it->second.end()) {
    return;  // No row for this ID
  }

  const flxv_map& row = row_it->second;

  // Step 1: Copy all raw columns to model
  for (const auto& [key, value] : row) {
    model[key] = value;
  }

  // Step 2: Use read_row to create nested structures
  model.read_row(row);

  // Step 3: Get properties for metadata
  const auto& properties = model.get_properties();

  // Step 4: Process child models (flxp_model)
  const auto& children = model.get_children();
  for (const auto& [child_fieldname, child] : children) {
    auto prop_it = properties.find(child_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      if (meta.find("table") != meta.end() && meta.find("foreign_key") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();
        flx_string foreign_key = meta.at("foreign_key").string_value();

        // Find child ID by matching foreign key
        auto child_table_it = all_rows.find(child_table);
        if (child_table_it != all_rows.end()) {
          for (const auto& [child_id, child_row] : child_table_it->second) {
            // Check if foreign key matches parent ID
            if (child_row.find(foreign_key) != child_row.end()) {
              long long fk_value = child_row.at(foreign_key).int_value();
              if (fk_value == id) {
                // Recursively construct child
                construct_tree_recursive(*child, child_table, child_id, all_rows);
                break;  // Only one child model (not a list)
              }
            }
          }
        }
      }
    }
  }

  // Step 5: Process model_lists (flxp_model_list)
  // Work with raw variant data to populate lists
  for (const auto& [list_fieldname, list] : model.get_model_lists()) {
    auto prop_it = properties.find(list_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      if (meta.find("table") != meta.end() && meta.find("foreign_key") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();
        flx_string foreign_key = meta.at("foreign_key").string_value();

        // Build vector of child maps
        flxv_vector child_maps;

        // Find all children with matching foreign key
        auto child_table_it = all_rows.find(child_table);
        if (child_table_it != all_rows.end()) {
          for (const auto& [child_id, child_row] : child_table_it->second) {
            // Check if foreign key matches parent ID
            if (child_row.find(foreign_key) != child_row.end()) {
              long long fk_value = child_row.at(foreign_key).int_value();
              if (fk_value == id) {
                // Create a temporary child model and recursively construct it
                // We'll do this after we have the list properly set up
                flxv_map child_map = child_row;  // Copy the row data
                child_maps.push_back(child_map);
              }
            }
          }
        }

        // Set the list in the model's raw data
        if (!child_maps.empty()) {
          model[list_fieldname] = flx_variant(child_maps);
        }
      }
    }
  }

  // Step 6: Call resync() to update typed properties
  model.resync();
}


inline void db_repository::search_hierarchical(const db_search_criteria& criteria, flx_list& results)
{
  // Get sample model from list factory
  auto sample = results.factory();
  if (sample.is_null()) {
    throw db_query_error("Failed to create sample model from list factory");
  }
  flx_model& model = *sample;

  // Phase 1: Build and execute ID hierarchy query
  db_query_builder builder;
  flx_string sql = build_hierarchy_query(model, extract_table_name(model), criteria, builder);

  auto query = connection_->create_query();
  if (!query) {
    throw db_query_error("Failed to create query object");
  }

  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare hierarchy query", sql, query->get_last_error());
  }

  // Bind parameters from query builder
  const auto& params = builder.get_parameters();
  for (const auto& param : params) {
    query->bind(param.first, param.second);
  }

  if (!query->execute()) {
    throw db_query_error("Failed to execute hierarchy query", sql, query->get_last_error());
  }

  // Collect all result rows
  std::vector<flxv_map> rows;
  while (query->next()) {
    rows.push_back(query->get_row());
  }

  if (rows.empty()) {
    return;  // No results - success but empty
  }

  // Extract distance values if semantic search was used
  std::map<long long, double> distance_map;
  if (criteria.has_vector_search()) {
    std::cout << "[DISTANCE DEBUG] Extracting distances from " << rows.size() << " rows" << std::endl;
    for (const auto& row : rows) {
      // Debug: print all fields in this row
      std::cout << "[DISTANCE DEBUG] Row fields: ";
      for (const auto& field : row) {
        std::cout << field.first.c_str() << " ";
      }
      std::cout << std::endl;

      // Check if distance field exists
      if (row.find("distance") != row.end()) {
        std::cout << "[DISTANCE DEBUG] Found distance field in row!" << std::endl;
        try {
          // Get the root table ID
          std::cout << "[DISTANCE DEBUG] Creating qualified_id..." << std::endl;
          flx_string qualified_id = flx_string(extract_table_name(model).c_str()) + flx_string(".id");
          std::cout << "[DISTANCE DEBUG] Looking for qualified_id: " << qualified_id.c_str() << std::endl;

          if (row.find(qualified_id) != row.end()) {
            std::cout << "[DISTANCE DEBUG] Found qualified_id in row!" << std::endl;
            // Use convert() to get type-converted copies
            flx_variant id_variant = row.at(qualified_id).convert(flx_variant::int_state);
            flx_variant dist_variant = row.at("distance").convert(flx_variant::double_state);
            long long id = id_variant.to_int();
            double distance = dist_variant.to_double();
            distance_map[id] = distance;
            std::cout << "[DISTANCE DEBUG] Stored distance " << distance << " for ID " << id << std::endl;
          } else {
            std::cout << "[DISTANCE DEBUG] qualified_id NOT found in row!" << std::endl;
          }
        } catch (const std::exception& e) {
          std::cerr << "[DISTANCE DEBUG] Exception: " << e.what() << std::endl;
        } catch (...) {
          std::cerr << "[DISTANCE DEBUG] Unknown exception!" << std::endl;
        }
      } else {
        std::cout << "[DISTANCE DEBUG] Distance field NOT in row" << std::endl;
      }
    }
    std::cout << "[DISTANCE DEBUG] Total distances extracted: " << distance_map.size() << std::endl;
  }

  // Phase 2: Parse results to get ID sets per table
  auto id_sets = parse_hierarchy_results(rows, model, extract_table_name(model));

  // Phase 3: Batch load full rows for each table
  auto all_rows = batch_load_rows(id_sets);

  // Phase 4: Tree construction - for each root ID
  auto root_ids = id_sets[extract_table_name(model)];
  for (long long root_id : root_ids) {
    flx_model root_model;
    construct_tree_recursive(root_model, extract_table_name(model), root_id, all_rows);

    // Attach distance value if available
    if (distance_map.find(root_id) != distance_map.end()) {
      (*root_model)["distance"] = distance_map[root_id];
    }

    // Add new element to results and copy data
    results.add_element();
    *results.back() = *root_model;
    results.back().resync();
  }
}

// Migration implementations

inline void db_repository::migrate_table(flx_model& model)
{
  if (!table_exists(model)) {
    // Auto-configure schema from metadata before creating
    auto_configure(model);
    create_table(model);
    return;
  }

  // Get existing columns from database
  auto existing_cols = get_existing_columns(model);
  if (existing_cols.empty()) {
    throw db_query_error("Failed to query existing columns");
  }

  // Get expected columns from model metadata
  auto fields = scan_fields(model);

  // Find missing columns
  std::vector<field_metadata> missing_cols;
  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;  // Skip primary key
    if (existing_cols.find(field.column_name) == existing_cols.end()) {
      missing_cols.push_back(field);
    }
  }

  if (missing_cols.empty()) {
    return;
  }

  // Add missing columns
  for (const auto& field : missing_cols) {
    flx_string sql_type = get_sql_type_from_state(field.type, field.column_name);
    flx_string alter_sql = "ALTER TABLE " + extract_table_name(model) + " ADD COLUMN " + field.column_name + " " + sql_type;

    auto query = connection_->create_query();
    if (!query->prepare(alter_sql)) {
      throw db_prepare_error("Failed to prepare ALTER TABLE", alter_sql, query->get_last_error());
    }

    if (!query->execute()) {
      throw db_query_error("Failed to add column " + field.column_name, alter_sql, query->get_last_error());
    }
  }
}


inline std::set<flx_string> db_repository::get_existing_columns(flx_model& model)
{
  std::set<flx_string> columns;

  // PostgreSQL-specific query to get column names from information_schema
  flx_string sql = "SELECT column_name FROM information_schema.columns WHERE table_name = :table_name";

  auto query = connection_->create_query();
  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare schema query", sql, query->get_last_error());
  }

  query->bind("table_name", flx_variant(extract_table_name(model)));

  if (!query->execute()) {
    throw db_query_error("Failed to query schema", sql, query->get_last_error());
  }

  while (query->next()) {
    auto row = query->get_row();
    if (row.count("column_name") > 0) {
      columns.insert(row["column_name"].to_string());
    }
  }

  return columns;
}


inline void db_repository::ensure_child_table_from_model(flx_model* child_model)
{
  if (child_model == nullptr) return;

  // Extract table name and fields from child model
  const auto& child_properties = child_model->get_properties();
  flx_string child_table_name;
  flx_string child_id_column = "id";
  std::vector<field_metadata> child_fields;

  // Scan for primary key and table name
  for (const auto& [prop_name, prop] : child_properties) {
    const flxv_map& meta = prop->get_meta();

    if (meta.find("primary_key") != meta.end()) {
      child_table_name = meta.at("primary_key").string_value();
    }

    // Collect field metadata
    if (meta.find("column") != meta.end()) {
      field_metadata field;
      field.cpp_name = prop_name;
      field.column_name = meta.at("column").string_value();
      field.is_primary_key = (meta.find("primary_key") != meta.end());
      field.is_foreign_key = (meta.find("foreign_key") != meta.end());
      if (field.is_foreign_key) {
        field.foreign_table = meta.at("foreign_key").string_value();
      }

      // Check for UNIQUE constraint
      field.is_unique = (meta.find("unique") != meta.end() &&
                         meta.at("unique").string_value() == "true");

      // Check for NOT NULL constraint
      field.is_not_null = (meta.find("not_null") != meta.end() &&
                           meta.at("not_null").string_value() == "true");

      field.type = prop->get_variant_type();
      child_fields.push_back(field);
    }
  }

  if (child_table_name.empty()) {
    // std::cerr << "[ensure_child_table] No table name found in child model" << std::endl;
    return;
  }

  // Check if table exists (use same method as table_exists())
  flx_string check_sql = "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = :table_name)";
  auto check_query = connection_->create_query();
  if (!check_query->prepare(check_sql)) {
    throw db_prepare_error("Failed to prepare table check", check_sql, check_query->get_last_error());
  }

  check_query->bind("table_name", flx_variant(child_table_name));
  if (check_query->execute() && check_query->next()) {
    auto row = check_query->get_row();
    if (row["exists"].bool_value()) {
      // Table exists - don't recursively process to avoid infinite loops
      // (child tables will be handled by their own ensure_structures() calls)
      return;
    }
  }

  // Table doesn't exist - create it
  // std::cerr << "[ensure_child_table] Creating table " << child_table_name.c_str() << std::endl;

  flx_string create_sql = "CREATE TABLE IF NOT EXISTS " + child_table_name + " (";
  bool first = true;

  for (const auto& field : child_fields) {
    if (!first) create_sql += ", ";
    first = false;

    create_sql += field.column_name + " ";
    create_sql += get_sql_type_from_state(field.type, field.column_name);

    if (field.is_primary_key) {
      create_sql += " PRIMARY KEY GENERATED ALWAYS AS IDENTITY";
    }

    // Add NOT NULL constraint if specified (skip for primary key)
    if (!field.is_primary_key && field.is_not_null) {
      create_sql += " NOT NULL";
    }

    // Add UNIQUE constraint if specified (skip for primary key)
    if (!field.is_primary_key && field.is_unique) {
      create_sql += " UNIQUE";
    }
  }

  create_sql += ")";

  auto create_query = connection_->create_query();
  if (!create_query->prepare(create_sql)) {
    throw db_prepare_error("Failed to prepare CREATE TABLE", create_sql, create_query->get_last_error());
  }

  if (!create_query->execute()) {
    throw db_query_error("Failed to execute CREATE TABLE", create_sql, create_query->get_last_error());
  }

  // std::cerr << "[ensure_child_table] Created table " << child_table_name.c_str() << std::endl;

  // Add foreign key constraints with CASCADE DELETE
  for (const auto& field : child_fields) {
    if (field.is_foreign_key && !field.foreign_table.empty()) {
      flx_string constraint_name = child_table_name + "_" + field.column_name + "_fkey";
      flx_string fk_sql = "ALTER TABLE " + child_table_name +
                          " ADD CONSTRAINT " + constraint_name +
                          " FOREIGN KEY (" + field.column_name + ")" +
                          " REFERENCES " + field.foreign_table + "(id)" +
                          " ON DELETE CASCADE";

      auto fk_query = connection_->create_query();
      if (!fk_query->prepare(fk_sql)) {
        throw db_prepare_error("Failed to prepare FK constraint", fk_sql, fk_query->get_last_error());
      }

      if (!fk_query->execute()) {
        // Ignore if constraint already exists
        // std::cerr << "[ensure_child_table] FK constraint might exist: " << fk_query->get_last_error().c_str() << std::endl;
      }
    }
  }

  // Don't recursively handle children to avoid infinite loops
  // Each model type will have its own ensure_structures() call
}


inline void db_repository::ensure_structures(flx_model& model)
{
  // Migrate own table first (creates if not exists, adds columns if needed)
  migrate_table(model);

  // Get all child tables from nested models and lists
  const auto& children = model.get_children();
  const auto& model_lists = model.get_model_lists();

  // Process nested models (flxp_model)
  for (const auto& [child_name, child_ptr] : children) {
    if (child_ptr == nullptr) continue;
    ensure_child_table_from_model(child_ptr);
  }

  // Process model_lists (flxp_model_list)
  for (const auto& [list_name, list_ptr] : model_lists) {
    if (list_ptr == nullptr) continue;

    // Use factory() to get a sample element without data
    auto sample_elem = list_ptr->factory();
    if (sample_elem.is_null()) {
      continue;
    }

    ensure_child_table_from_model(&(*sample_elem));
  }
}


// ============================================================================
// PostgreSQL Error Message Parsing
// ============================================================================

inline db_repository::fk_violation_info db_repository::parse_fk_violation(const flx_string& error_msg)
{
  fk_violation_info info;

  // PostgreSQL FK error format:
  // "ERROR: insert or update on table "table_name" violates foreign key constraint "constraint_name"
  //  DETAIL: Key (column_name)=(value) is not present in table "referenced_table"."

  // Extract column name from "Key (column_name)=(...)"
  size_t key_pos = error_msg.find("Key (");
  if (key_pos != std::string::npos) {
    size_t start_pos = key_pos + 5;  // After "Key ("
    size_t end_pos = error_msg.find(")", start_pos);
    if (end_pos != std::string::npos) {
      info.foreign_key_column = error_msg.substr(start_pos, end_pos - start_pos);
    }
  }

  // Extract referenced table from "not present in table \"table_name\""
  size_t table_pos = error_msg.find("not present in table ");
  if (table_pos != std::string::npos) {
    size_t start_pos = error_msg.find("\"", table_pos);
    if (start_pos != std::string::npos) {
      start_pos++;  // Skip opening quote
      size_t end_pos = error_msg.find("\"", start_pos);
      if (end_pos != std::string::npos) {
        info.referenced_table = error_msg.substr(start_pos, end_pos - start_pos);
      }
    }
  }

  return info;
}


inline db_repository::unique_violation_info db_repository::parse_unique_violation(const flx_string& error_msg)
{
  unique_violation_info info;

  // PostgreSQL unique constraint error format:
  // "ERROR: duplicate key value violates unique constraint "constraint_name"
  //  DETAIL: Key (column_name)=(value) already exists."

  // Extract column name from "Key (column_name)=(...)"
  size_t key_pos = error_msg.find("Key (");
  if (key_pos != std::string::npos) {
    size_t start_pos = key_pos + 5;  // After "Key ("
    size_t end_pos = error_msg.find(")", start_pos);
    if (end_pos != std::string::npos) {
      info.column_name = error_msg.substr(start_pos, end_pos - start_pos);

      // Extract value from ")=(value)"
      size_t value_start = error_msg.find("=(", end_pos);
      if (value_start != std::string::npos) {
        value_start += 2;  // After "=("
        size_t value_end = error_msg.find(")", value_start);
        if (value_end != std::string::npos) {
          info.value_str = error_msg.substr(value_start, value_end - value_start);
        }
      }
    }
  }

  return info;
}


// ============================================================================
// Semantic Embedding Support
// ============================================================================

inline bool db_repository::has_semantic_properties(const flx_model& model) const
{
  const auto& properties = model.get_properties();

  for (const auto& prop_pair : properties) {
    const flxv_map& meta = prop_pair.second->get_meta();
    auto it = meta.find("semantic");
    if (it != meta.end()) {
      flx_variant semantic_flag = it->second;  // Non-const copy
      if (semantic_flag.in_state() == flx_variant::bool_state && semantic_flag.to_bool()) {
        return true;
      }
    }
  }

  return false;
}


inline void db_repository::ensure_pgvector_extension()
{
  flx_string sql = "CREATE EXTENSION IF NOT EXISTS vector";

  auto query = connection_->create_query();
  if (!query->prepare(sql)) {
    throw db_prepare_error("Failed to prepare CREATE EXTENSION", sql, query->get_last_error());
  }

  if (!query->execute()) {
    throw db_query_error("Failed to create pgvector extension", sql, query->get_last_error());
  }
}


inline void db_repository::create_semantic_index(const flx_string& table_name)
{
  // Check if semantic_embedding column exists
  flx_string check_sql = "SELECT column_name FROM information_schema.columns WHERE table_name = :table_name AND column_name = 'semantic_embedding'";

  auto check_query = connection_->create_query();
  if (!check_query->prepare(check_sql)) {
    return;  // Silent fail - column doesn't exist
  }

  check_query->bind("table_name", flx_variant(table_name));
  if (!check_query->execute()) {
    return;  // Silent fail
  }

  auto rows = check_query->get_all_rows();
  if (rows.empty()) {
    return;  // No semantic_embedding column
  }

  // Create HNSW index for vector similarity search
  flx_string index_name = "idx_" + table_name + "_semantic_embedding";
  flx_string sql = "CREATE INDEX IF NOT EXISTS " + index_name + " ON " + table_name +
                   " USING hnsw (semantic_embedding halfvec_cosine_ops) WITH (m = 16, ef_construction = 64)";

  auto query = connection_->create_query();
  if (!query->prepare(sql)) {
    // Index creation can fail if already exists or column type wrong - not critical
    return;
  }

  query->execute();  // Ignore errors - index creation is best-effort
}


#endif // DB_REPOSITORY_H
