#ifndef DB_REPOSITORY_H
#define DB_REPOSITORY_H

#include "db_connection.h"
#include "db_query.h"
#include "db_query_builder.h"
#include "db_search_criteria.h"
#include "flx_semantic_embedder.h"
#include "../../utils/flx_model.h"
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <iostream>

template<typename T>
class db_repository {
  static_assert(std::is_base_of<flx_model, T>::value, "T must derive from flx_model");

public:
  explicit db_repository(db_connection* conn, const flx_string& table_name);
  virtual ~db_repository() = default;

  // CRUD Operations
  virtual bool create(T& model);
  virtual bool update(T& model);
  virtual bool remove(T& model);
  virtual bool find_by_id(long long id, T& model);
  virtual bool find_all(flx_model_list<T>& results);
  virtual bool find_where(const flx_string& condition, flx_model_list<T>& results);

  // Utility
  virtual bool table_exists();
  virtual bool create_table();
  virtual bool migrate_table();  // Auto-add missing columns from model
  virtual bool drop_table();

  // Configuration
  void set_id_column(const flx_string& column_name);
  void set_schema(const std::vector<flx_string>& columns);
  void set_embedder(flx_semantic_embedder* embedder);
  flx_string get_last_error() const;

  // Metadata-based automation
  virtual bool auto_configure();
  virtual bool search(const db_search_criteria& criteria, flx_model_list<T>& results);

  // Hierarchical search (two-phase: ID query + batch load + tree construction)
  virtual bool search_hierarchical(const db_search_criteria& criteria, flx_model_list<T>& results);

  // Metadata scanning structures
  struct field_metadata {
    flx_string property_name;  // XML fieldname for nested path navigation (legacy)
    flx_string cpp_name;       // C++ property name for map lookup
    flx_string column_name;
    bool is_primary_key;
    bool is_foreign_key;
    flx_string foreign_table;
    flx_variant::state type;
  };

  struct relation_metadata {
    flx_string property_name;
    flx_string related_table;
    flx_string foreign_key_column;
  };

  virtual std::vector<field_metadata> scan_fields();
  virtual std::vector<relation_metadata> scan_relations();

  // Hierarchical operations helpers
  virtual bool save_nested_objects(T& model);
  virtual bool load_nested_objects(T& model);
  virtual flx_string build_join_sql(const std::vector<relation_metadata>& relations);
  virtual void map_joined_results(const flxv_map& row, T& model, const std::vector<relation_metadata>& relations);

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
  flx_string table_name_;
  flx_string id_column_;
  std::vector<flx_string> columns_;
  flx_string last_error_;
  flx_semantic_embedder* embedder_;

  // Helper methods
  virtual flx_string build_insert_sql(const T& model);
  virtual flx_string build_update_sql(const T& model);
  virtual flx_string build_select_sql(const flx_string& where_clause = "");
  virtual void bind_model_values(db_query* query, T& model);
  virtual T map_to_model(const flxv_map& row);
  virtual flx_string get_sql_type(const flx_variant& value);
  virtual flx_string get_sql_type_from_state(flx_variant::state state);
  virtual flx_variant access_nested_value(T& model, const flx_string& property_name);
  virtual std::set<flx_string> get_existing_columns();  // Query database schema
};

// Implementation

template<typename T>
db_repository<T>::db_repository(db_connection* conn, const flx_string& table_name)
  : connection_(conn)
  , table_name_(table_name)
  , id_column_("id")
  , last_error_("")
  , embedder_(nullptr)
{
}

template<typename T>
void db_repository<T>::set_id_column(const flx_string& column_name)
{
  id_column_ = column_name;
}

template<typename T>
void db_repository<T>::set_embedder(flx_semantic_embedder* embedder)
{
  embedder_ = embedder;
}

template<typename T>
void db_repository<T>::set_schema(const std::vector<flx_string>& columns)
{
  columns_ = columns;
}

template<typename T>
flx_string db_repository<T>::get_last_error() const
{
  return last_error_;
}

template<typename T>
bool db_repository<T>::create(T& model)
{
  std::cerr << "[CREATE] START for table=" << table_name_.c_str() << std::endl;

  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  // AUTO-EMBED: Generate semantic embedding if embedder is set
  if (embedder_) {
    std::cerr << "[CREATE] Generating embedding..." << std::endl;
    embedder_->embed_model(model);
    std::cerr << "[CREATE] Embedding done" << std::endl;
  }

  std::cerr << "[CREATE] Creating query..." << std::endl;
  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    std::cerr << "[CREATE] FAILED to create query" << std::endl;
    return false;
  }

  std::cerr << "[CREATE] Building INSERT SQL..." << std::endl;
  flx_string sql = build_insert_sql(model);
  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare insert: ") + query->get_last_error();
    std::cerr << "[CREATE] FAILED to prepare" << std::endl;
    return false;
  }

  std::cerr << "[CREATE] Binding values..." << std::endl;
  bind_model_values(query.get(), model);

  std::cerr << "[CREATE] Executing INSERT..." << std::endl;
  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute insert: ") + query->get_last_error();
    std::cerr << "[CREATE] FAILED to execute" << std::endl;
    return false;
  }

  std::cerr << "[CREATE] Getting lastval..." << std::endl;
  // Get the inserted ID if available
  auto id_query = connection_->create_query();
  id_query->prepare("SELECT lastval()");
  if (id_query->execute() && id_query->next()) {
    auto row = id_query->get_row();
    if (row.find("lastval") != row.end()) {
      model[id_column_] = row["lastval"];
      std::cerr << "[CREATE] Got ID=" << row["lastval"].to_int() << std::endl;
    }
  }

  // AUTO-SAVE: Save nested objects
  std::cerr << "[CREATE] Before save_nested_objects call" << std::endl;
  bool result = save_nested_objects(model);
  std::cerr << "[CREATE] After save_nested_objects call, result=" << result << std::endl;
  if (!result) {
    last_error_ = flx_string("Failed to save nested objects: ") + last_error_;
    return false;
  }

  last_error_ = "";
  return true;
}

template<typename T>
bool db_repository<T>::update(T& model)
{
  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  // AUTO-EMBED: Generate semantic embedding if embedder is set
  if (embedder_) {
    embedder_->embed_model(model);
  }

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    return false;
  }

  flx_string sql = build_update_sql(model);
  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare update: ") + query->get_last_error();
    return false;
  }

  bind_model_values(query.get(), model);
  query->bind("id_value", model[id_column_]);

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute update: ") + query->get_last_error();
    return false;
  }

  // AUTO-UPDATE: Delete old nested objects and insert new ones
  auto relations = scan_relations();
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
    if (!save_nested_objects(model)) {
      last_error_ = flx_string("Failed to update nested objects: ") + last_error_;
      return false;
    }
  }

  last_error_ = "";
  return true;
}

template<typename T>
bool db_repository<T>::remove(T& model)
{
  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    return false;
  }

  flx_string sql = flx_string("DELETE FROM ") + table_name_ + " WHERE " + id_column_ + " = :id_value";

  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare delete: ") + query->get_last_error();
    return false;
  }

  query->bind("id_value", model[id_column_]);

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute delete: ") + query->get_last_error();
    return false;
  }

  last_error_ = "";
  return true;
}

template<typename T>
bool db_repository<T>::find_by_id(long long id, T& model)
{
  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    return false;
  }

  flx_string sql = build_select_sql(id_column_ + " = :id_value");

  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare select: ") + query->get_last_error();
    return false;
  }

  query->bind("id_value", flx_variant(id));

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute select: ") + query->get_last_error();
    return false;
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

    last_error_ = "";
    return true;
  }

  last_error_ = "Record not found";
  return false;
}

template<typename T>
bool db_repository<T>::find_all(flx_model_list<T>& results)
{
  return find_where("", results);
}

template<typename T>
bool db_repository<T>::find_where(const flx_string& condition, flx_model_list<T>& results)
{
  results.clear();

  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    return false;
  }

  flx_string sql = build_select_sql(condition);

  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare select: ") + query->get_last_error();
    return false;
  }

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute select: ") + query->get_last_error();
    return false;
  }

  // Make a copy of rows to avoid issues with query object reuse
  std::vector<flxv_map> rows_copy = query->get_all_rows();

  // Use index-based iteration
  for (size_t i = 0; i < rows_copy.size(); ++i) {
    const auto& row = rows_copy[i];

    T model;
    // First: Copy ALL raw columns (including "id" and other non-property columns)
    for (const auto& pair : row) {
      model[pair.first] = pair.second;
    }
    // Second: Let read_row create nested structures for properties with paths
    model.read_row(row);

    // AUTO-LOAD: Load nested objects for each result
    load_nested_objects(model);

    results.push_back(model);
  }

  last_error_ = "";
  return true;
}

template<typename T>
bool db_repository<T>::table_exists()
{
  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  auto query = connection_->create_query();
  query->prepare("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = :table_name)");
  query->bind("table_name", flx_variant(table_name_));

  if (query->execute() && query->next()) {
    auto row = query->get_row();
    return row["exists"].bool_value();
  }

  return false;
}

template<typename T>
bool db_repository<T>::create_table()
{
  if (columns_.empty()) {
    last_error_ = "No schema defined. Call set_schema() or auto_configure() first.";
    return false;
  }

  // Scan fields to get proper types from metadata
  auto fields = scan_fields();

  flx_string sql = flx_string("CREATE TABLE IF NOT EXISTS ") + table_name_ + " (\n";
  sql += "  " + id_column_ + " SERIAL PRIMARY KEY";

  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;

    flx_string sql_type = get_sql_type_from_state(field.type);
    sql += ",\n  " + field.column_name + " " + sql_type;
  }

  sql += "\n)";

  auto query = connection_->create_query();
  query->prepare(sql);

  if (!query->execute()) {
    last_error_ = flx_string("Failed to create table: ") + query->get_last_error();
    return false;
  }

  last_error_ = "";
  return true;
}

template<typename T>
bool db_repository<T>::drop_table()
{
  auto query = connection_->create_query();
  query->prepare(flx_string("DROP TABLE IF EXISTS ") + table_name_);

  if (!query->execute()) {
    last_error_ = flx_string("Failed to drop table: ") + query->get_last_error();
    return false;
  }

  last_error_ = "";
  return true;
}

template<typename T>
flx_string db_repository<T>::build_insert_sql(const T& model)
{
  auto fields = scan_fields();

  flx_string sql = flx_string("INSERT INTO ") + table_name_ + " (";
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

template<typename T>
flx_string db_repository<T>::build_update_sql(const T& model)
{
  auto fields = scan_fields();

  flx_string sql = flx_string("UPDATE ") + table_name_ + " SET ";

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

template<typename T>
flx_string db_repository<T>::build_select_sql(const flx_string& where_clause)
{
  flx_string sql = flx_string("SELECT * FROM ") + table_name_;

  if (!where_clause.empty()) {
    sql += " WHERE " + where_clause;
  }

  return sql;
}

template<typename T>
void db_repository<T>::bind_model_values(db_query* query, T& model)
{
  auto fields = scan_fields();
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
    std::cout << "[bind] property=" << field.property_name.c_str()
              << " column=" << field.column_name.c_str()
              << " value=" << (value.is_null() ? "NULL" : "HAS_VALUE") << std::endl;

    query->bind(field.column_name, value);  // Bind with column_name (DB column)
  }
}

// Helper function to access nested values (handles "/" separator like flx_property_i::access())
template<typename T>
flx_variant db_repository<T>::access_nested_value(T& model, const flx_string& property_name)
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

template<typename T>
T db_repository<T>::map_to_model(const flxv_map& row)
{
  T model;

  for (const auto& pair : row) {
    model[pair.first] = pair.second;
  }

  return model;
}

template<typename T>
flx_string db_repository<T>::get_sql_type(const flx_variant& value)
{
  if (value.is_string()) return "VARCHAR(255)";
  if (value.is_int()) return "BIGINT";
  if (value.is_double()) return "DOUBLE PRECISION";
  if (value.is_bool()) return "BOOLEAN";
  return "TEXT";
}

template<typename T>
flx_string db_repository<T>::get_sql_type_from_state(flx_variant::state state)
{
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
      return "JSONB";
    default:
      return "TEXT";
  }
}

// Metadata-based automation implementations

template<typename T>
std::vector<typename db_repository<T>::field_metadata> db_repository<T>::scan_fields()
{
  std::vector<field_metadata> fields;
  T sample_model;

  const auto& properties = sample_model.get_properties();

  std::cout << "[scan_fields] Total properties: " << properties.size() << std::endl;
  std::cout << "[scan_fields] columns_ size: " << columns_.size() << std::endl;

  for (const auto& prop_pair : properties) {
    const flx_string& prop_name = prop_pair.first;
    flx_property_i* prop = prop_pair.second;

    std::cout << "[scan_fields] Checking property: " << prop_name.c_str() << std::endl;

    // Get metadata first
    const flxv_map& meta = prop->get_meta();

    // If schema is explicitly set, only include properties in columns_
    if (!columns_.empty()) {
      // Use column metadata if available
      flx_string check_name;
      if (meta.find("column") != meta.end()) {
        flx_variant column_var = meta.at("column");  // Non-const copy
        check_name = column_var.to_string();
        std::cout << "[scan_fields]   Has column metadata: " << check_name.c_str() << std::endl;
      } else {
        // No column metadata - skip this property when columns_ is set
        std::cout << "[scan_fields]   SKIPPED (no column metadata)" << std::endl;
        continue;
      }

      bool found = false;
      for (const auto& col : columns_) {
        if (col == check_name) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::cout << "[scan_fields]   SKIPPED (not in columns_)" << std::endl;
        continue;
      }
      std::cout << "[scan_fields]   FOUND in columns_" << std::endl;
    }

    // Skip relation properties (those with "table" metadata)
    if (meta.find("table") != meta.end()) {
      continue;
    }

    field_metadata field;
    field.property_name = prop_name;  // C++ name for main model properties
    field.cpp_name = prop_name;       // Same as property_name for main models

    // Check for column name override
    if (meta.find("column") != meta.end()) {
      field.column_name = meta.at("column").string_value();
    } else {
      field.column_name = prop_name;
    }

    // Check for primary key
    field.is_primary_key = (meta.find("primary_key") != meta.end());

    // Check for foreign key
    if (meta.find("foreign_key") != meta.end()) {
      field.is_foreign_key = true;
      field.foreign_table = meta.at("foreign_key").string_value();
    } else {
      field.is_foreign_key = false;
    }

    // Get type from property definition
    field.type = prop->get_variant_type();

    fields.push_back(field);
  }

  return fields;
}

template<typename T>
std::vector<typename db_repository<T>::relation_metadata> db_repository<T>::scan_relations()
{
  std::vector<relation_metadata> relations;
  T sample_model;

  const auto& properties = sample_model.get_properties();

  std::cerr << "[scan_relations] Scanning properties for relations..." << std::endl;

  for (const auto& prop_pair : properties) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Check if this is a relation (has "table" metadata)
    if (meta.find("table") != meta.end()) {
      relation_metadata rel;

      // Use fieldname as property_name (fieldname overrides C++ name everywhere)
      if (meta.find("fieldname") != meta.end()) {
        rel.property_name = meta.at("fieldname").string_value();
      } else {
        rel.property_name = prop_pair.first;  // fallback to C++ name
      }

      rel.related_table = meta.at("table").string_value();

      // Get foreign key column from relation metadata
      if (meta.find("foreign_key") != meta.end()) {
        rel.foreign_key_column = meta.at("foreign_key").string_value();
      } else {
        // Default: infer from table name (remove 's' and add '_id')
        flx_string parent_table = table_name_;
        if (parent_table.ends_with("s")) {
          parent_table = parent_table.substr(0, parent_table.length() - 1);
        }
        rel.foreign_key_column = parent_table + "_id";
      }

      std::cerr << "[scan_relations] Found relation: property=" << rel.property_name.c_str()
                << " table=" << rel.related_table.c_str()
                << " fk=" << rel.foreign_key_column.c_str() << std::endl;

      relations.push_back(rel);
    }
  }

  std::cerr << "[scan_relations] Total relations found: " << relations.size() << std::endl;
  return relations;
}

template<typename T>
bool db_repository<T>::auto_configure()
{
  // Scan fields from metadata
  auto fields = scan_fields();

  // Build columns list and find primary key
  columns_.clear();

  for (const auto& field : fields) {
    columns_.push_back(field.column_name);

    if (field.is_primary_key) {
      id_column_ = field.column_name;
    }
  }

  // If no primary key found, default to "id"
  if (id_column_.empty()) {
    id_column_ = "id";
  }

  return true;
}

template<typename T>
bool db_repository<T>::search(const db_search_criteria& criteria, flx_model_list<T>& results)
{
  results.clear();

  if (!connection_ || !connection_->is_connected()) {
    last_error_ = "Not connected to database";
    return false;
  }

  // Build query using query builder
  db_query_builder builder;
  builder.from(table_name_);

  // Apply search criteria
  criteria.apply_to(builder);

  flx_string sql = builder.build_select();

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query";
    return false;
  }

  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare search query: ") + query->get_last_error();
    return false;
  }

  // Bind parameters from query builder
  const auto& params = builder.get_parameters();
  for (const auto& param : params) {
    query->bind(param.first, param.second);
  }

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute search: ") + query->get_last_error();
    return false;
  }

  // Make a copy of rows to avoid issues with query object reuse during nested loading
  std::vector<flxv_map> rows_copy = query->get_all_rows();

  // Use index-based iteration
  for (size_t i = 0; i < rows_copy.size(); ++i) {
    const auto& row = rows_copy[i];

    T model;
    // First: Copy ALL raw columns (including "id" and other non-property columns)
    for (const auto& pair : row) {
      model[pair.first] = pair.second;
    }
    // Second: Let read_row create nested structures for properties with paths
    model.read_row(row);

    // AUTO-LOAD: Load nested objects for each result
    load_nested_objects(model);

    results.push_back(model);
  }

  last_error_ = "";
  return true;
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

template<typename T>
bool db_repository<T>::save_nested_objects(T& model)
{
  std::cerr << "[save_nested_objects] START" << std::endl;
  auto relations = scan_relations();

  std::cerr << "[save_nested_objects] Found " << relations.size() << " relations" << std::endl;

  if (relations.empty()) {
    return true;  // No nested objects to save
  }

  // Get parent ID
  flx_variant parent_id = model[id_column_];
  if (parent_id.in_state() == flx_variant::none) {
    last_error_ = "Cannot save nested objects: parent ID is null";
    return false;
  }

  const auto& properties = model.get_properties();

  for (const auto& rel : relations) {
    std::cerr << "[save_nested_objects] Processing relation: " << rel.property_name.c_str()
              << " (table=" << rel.related_table.c_str() << ")" << std::endl;
    // Find the property with this relation
    // Note: flxp_model_list registers the property with the base name, not name##_vec
    auto prop_it = properties.find(rel.property_name);
    if (prop_it == properties.end()) {
      std::cerr << "[save_nested_objects] Property not found in properties, skipping" << std::endl;
      continue;  // Relation property not found
    }

    std::cerr << "[save_nested_objects] Property found, getting data..." << std::endl;
    // Get the property data (can be vector for model_lists or map for single models)
    flx_variant& data = model[rel.property_name];

    std::cerr << "[save_nested_objects] data.is_vector()=" << data.is_vector()
              << " data.is_map()=" << data.is_map()
              << " data.in_state()=" << data.in_state() << std::endl;

    // Convert to vector for unified processing
    flxv_vector items;
    if (data.is_vector()) {
      items = data.vector_value();
      std::cerr << "[save_nested_objects] Got vector with " << items.size() << " items" << std::endl;
    } else if (data.is_map()) {
      // Single model - wrap in vector with one element
      items.push_back(data);
      std::cerr << "[save_nested_objects] Got map, wrapped as single item" << std::endl;
    } else {
      std::cerr << "[save_nested_objects] Data is neither vector nor map, skipping" << std::endl;
      continue;  // Not a model or model_list
    }

    // Skip empty lists
    if (items.empty()) {
      std::cerr << "[save_nested_objects] Items empty, skipping" << std::endl;
      continue;
    }

    // Get the typed child model - try children first (single models), then model_lists
    flx_model* typed_child_model = nullptr;

    const auto& children = model.get_children();
    auto child_it = children.find(rel.property_name);
    if (child_it != children.end() && child_it->second != nullptr) {
      // Single nested model (flxp_model)
      typed_child_model = child_it->second;
    } else {
      // Try model_lists (flxp_model_list)
      const auto& lists = model.get_model_lists();
      auto list_it = lists.find(rel.property_name);
      if (list_it != lists.end() && list_it->second != nullptr && list_it->second->list_size() > 0) {
        // Get first item from list for schema reflection
        typed_child_model = list_it->second->get_model_at(0);
      } else {
        // Can't determine schema - skip this relation
        continue;
      }
    }

    // Scan fields from the typed child model to get column metadata
    std::vector<field_metadata> child_fields;
    const auto& child_props = typed_child_model->get_properties();
    for (const auto& prop_pair : child_props) {
      flx_property_i* prop = prop_pair.second;
      const flxv_map& meta = prop->get_meta();

      // Only include properties with "column" metadata
      if (meta.find("column") == meta.end()) {
        continue;
      }

      // Skip relation properties (those with "table" metadata)
      if (meta.find("table") != meta.end()) {
        continue;
      }

      field_metadata field;
      // CRITICAL: Store C++ name for map lookup (properties are stored with C++ names)
      field.cpp_name = prop_pair.first;

      // Use fieldname for XML path navigation (legacy)
      if (meta.find("fieldname") != meta.end()) {
        field.property_name = meta.at("fieldname").string_value();
      } else {
        // Fallback to C++ name if no fieldname
        field.property_name = prop_pair.first;
      }
      field.column_name = meta.at("column").string_value();
      field.type = prop->get_variant_type();
      child_fields.push_back(field);
    }

    // Check if this is a single child model or a model_list
    const auto& children_for_save = model.get_children();
    const auto& lists_for_save = model.get_model_lists();

    flx_model* single_child = nullptr;
    flx_list* model_list = nullptr;

    auto child_save_it = children_for_save.find(rel.property_name);
    if (child_save_it != children_for_save.end()) {
      single_child = child_save_it->second;
      std::cout << "[DEBUG] Found single child: " << rel.property_name.c_str() << std::endl;
    } else {
      auto list_save_it = lists_for_save.find(rel.property_name);
      if (list_save_it != lists_for_save.end()) {
        model_list = list_save_it->second;
        std::cout << "[DEBUG] Found model_list: " << rel.property_name.c_str()
                  << " size=" << model_list->list_size() << std::endl;
      } else {
        std::cout << "[DEBUG] Neither child nor list found: " << rel.property_name.c_str() << std::endl;
      }
    }

    // Determine how many items to insert
    size_t item_count = 0;
    if (single_child != nullptr) {
      item_count = 1;  // Single nested model
    } else if (model_list != nullptr) {
      item_count = model_list->list_size();
    } else {
      item_count = items.size();  // Fallback to variant items
    }

    // Now insert each item using the proper column mapping
    for (size_t item_idx = 0; item_idx < item_count; ++item_idx) {
      // Get the typed model directly from single_child or model_list
      flx_model* item_model = nullptr;

      if (single_child != nullptr) {
        item_model = single_child;  // Single nested model
      } else if (model_list != nullptr && item_idx < model_list->list_size()) {
        item_model = model_list->get_model_at(item_idx);  // List item
      }

      if (item_model == nullptr) {
        continue;
      }

      // Build insert SQL for child object using column metadata
      flx_string child_sql = "INSERT INTO " + rel.related_table + " (";
      flx_string values_sql = " VALUES (";

      bool first = true;

      // Add foreign key column first
      child_sql += rel.foreign_key_column;
      values_sql += flx_string(":") + rel.foreign_key_column;
      first = false;

      // Add fields using column metadata
      for (const auto& field : child_fields) {
        // Skip ID column (let database auto-generate via DEFAULT)
        if (field.column_name == id_column_) {
          continue;
        }

        // Skip foreign key column (already added above)
        if (field.column_name == rel.foreign_key_column) {
          continue;
        }

        if (!first) {
          child_sql += ", ";
          values_sql += ", ";
        }
        child_sql += field.column_name;
        values_sql += flx_string(":") + field.column_name;
        first = false;
      }

      child_sql += ")" + values_sql + ")";

      std::cout << "[SQL] " << child_sql.c_str() << std::endl;

      auto child_query = connection_->create_query();
      if (!child_query->prepare(child_sql)) {
        last_error_ = "Failed to prepare child insert: " + child_query->get_last_error();
        return false;
      }

      // Bind foreign key first
      child_query->bind(rel.foreign_key_column, parent_id);
      std::cout << "[SQL BIND] " << rel.foreign_key_column.c_str() << " = " << parent_id.to_string().c_str() << std::endl;

      // Bind field values by accessing properties directly from the model
      const auto& item_properties = item_model->get_properties();
      for (const auto& field : child_fields) {
        // Skip ID column (already skipped in SQL)
        if (field.column_name == id_column_) {
          continue;
        }

        // Skip foreign key column (already bound above)
        if (field.column_name == rel.foreign_key_column) {
          continue;
        }

        // CRITICAL: Use cpp_name to find property in the model (properties keyed by C++ names)
        auto prop_it = item_properties.find(field.cpp_name);

        flx_variant value;
        if (prop_it != item_properties.end()) {
          // Access property value directly
          value = prop_it->second->access();
          std::cout << "[SQL BIND] " << field.column_name.c_str() << " = "
                    << (value.is_null() ? "NULL" : value.to_string().c_str()) << std::endl;
        } else {
          // Property not found - bind NULL
          std::cout << "[SQL BIND] " << field.column_name.c_str() << " = NULL (property not found)" << std::endl;
        }

        child_query->bind(field.column_name, value);
      }

      if (!child_query->execute()) {
        last_error_ = "Failed to execute child insert: " + child_query->get_last_error();
        return false;
      }
    }
  }

  return true;
}

template<typename T>
bool db_repository<T>::load_nested_objects(T& model)
{
  auto relations = scan_relations();

  std::cout << "[LOAD] Loading nested objects, found " << relations.size() << " relations" << std::endl;

  if (relations.empty()) {
    return true;  // No nested objects to load
  }

  // Get parent ID
  flx_variant parent_id = model[id_column_];
  if (parent_id.in_state() == flx_variant::none) {
    std::cout << "[LOAD] Parent ID is null, skipping" << std::endl;
    return true;  // No ID, nothing to load
  }

  std::cout << "[LOAD] Parent ID: " << parent_id.to_string().c_str() << std::endl;

  for (const auto& rel : relations) {
    // Load child objects
    flx_string child_sql = "SELECT * FROM " + rel.related_table +
                           " WHERE " + rel.foreign_key_column + " = :parent_id";

    std::cout << "[LOAD SQL] " << child_sql.c_str() << std::endl;
    std::cout << "[LOAD BIND] parent_id = " << parent_id.to_string().c_str() << std::endl;

    auto child_query = connection_->create_query();
    if (!child_query->prepare(child_sql)) {
      std::cout << "[LOAD] Failed to prepare: " << child_query->get_last_error().c_str() << std::endl;
      continue;
    }

    child_query->bind("parent_id", parent_id);

    if (!child_query->execute()) {
      std::cout << "[LOAD] Failed to execute: " << child_query->get_last_error().c_str() << std::endl;
      continue;
    }

    auto child_rows = child_query->get_all_rows();

    std::cout << "[LOAD] Found " << child_rows.size() << " child rows for " << rel.property_name.c_str() << std::endl;

    // Check if this is a single model (children) or model_list (model_lists)
    const auto& children = model.get_children();
    const auto& lists = model.get_model_lists();

    bool is_single_model = (children.find(rel.property_name) != children.end());
    bool is_model_list = (lists.find(rel.property_name) != lists.end());

    if (is_single_model && !child_rows.empty()) {
      // Single nested model (flxp_model) - set the map directly
      model[rel.property_name] = flx_variant(child_rows[0]);
      std::cout << "[LOAD] Set single model[" << rel.property_name.c_str() << "] from first row" << std::endl;
    } else if (is_model_list || !is_single_model) {
      // Model list (flxp_model_list) - create vector
      flxv_vector children_vec;
      for (const auto& child_row : child_rows) {
        children_vec.push_back(flx_variant(child_row));
      }
      model[rel.property_name] = flx_variant(children_vec);
      std::cout << "[LOAD] Set model_list[" << rel.property_name.c_str() << "] with " << children_vec.size() << " items" << std::endl;
    }

  }

  // Resync all typed child models to point to the loaded data
  std::cout << "[LOAD] Calling resync() to update typed child models" << std::endl;
  model.resync();

  return true;
}

template<typename T>
flx_string db_repository<T>::build_join_sql(const std::vector<relation_metadata>& relations)
{
  flx_string joins;

  for (const auto& rel : relations) {
    // LEFT JOIN related_table AS relationname ON table.id = relationname.foreign_key
    joins += " LEFT JOIN " + rel.related_table + " AS " + rel.property_name;
    joins += " ON " + table_name_ + "." + id_column_ + " = ";
    joins += rel.property_name + "." + rel.foreign_key_column;
  }

  return joins;
}

template<typename T>
void db_repository<T>::map_joined_results(const flxv_map& row, T& model, const std::vector<relation_metadata>& relations)
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

template<typename T>
flx_string db_repository<T>::find_primary_key_column(flx_model& model)
{
  // Scan properties for primary_key metadata
  const auto& properties = model.get_properties();

  for (const auto& prop_pair : properties) {
    flx_property_i* prop = prop_pair.second;
    const flxv_map& meta = prop->get_meta();

    // Check if this property has primary_key metadata
    if (meta.find("primary_key") != meta.end()) {
      if (meta.at("primary_key").bool_value()) {
        // Found primary key - return its column name
        if (meta.find("column") != meta.end()) {
          return meta.at("column").string_value();
        }
      }
    }
  }

  // Fallback to default "id" column
  return "id";
}

template<typename T>
std::set<flx_string> db_repository<T>::collect_all_table_names(flx_model& model, const flx_string& root_table)
{
  std::set<flx_string> tables;
  tables.insert(root_table);

  // Get all properties to access metadata
  const auto& properties = model.get_properties();

  // Iterate over all child models
  const auto& children = model.get_children();
  for (const auto& child_pair : children) {
    const flx_string& child_fieldname = child_pair.first;
    flx_model* child = child_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(child_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Get the table name from metadata
      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();

        // Recursively collect tables from this child
        auto child_tables = collect_all_table_names(*child, child_table);
        tables.insert(child_tables.begin(), child_tables.end());
      }
    }
  }

  // Iterate over all model_lists
  const auto& model_lists = model.get_model_lists();
  for (const auto& list_pair : model_lists) {
    const flx_string& list_fieldname = list_pair.first;
    flx_list* list = list_pair.second;

    // Find the corresponding property to get metadata
    auto prop_it = properties.find(list_fieldname);
    if (prop_it != properties.end()) {
      const flxv_map& meta = prop_it->second->get_meta();

      // Get the table name from metadata
      if (meta.find("table") != meta.end()) {
        flx_string child_table = meta.at("table").string_value();

        // Always add the table for the list
        tables.insert(child_table);

        // If the list has elements, recursively collect from first element
        // (all elements have the same structure)
        if (list->list_size() > 0) {
          flx_model* first_elem = list->get_model_at(0);
          if (first_elem) {
            auto child_tables = collect_all_table_names(*first_elem, child_table);
            // Merge child tables (excluding the child_table itself which is already added)
            tables.insert(child_tables.begin(), child_tables.end());
          }
        }
      }
    }
  }

  return tables;
}

template<typename T>
flx_string db_repository<T>::build_joins_recursive(flx_model& model, const flx_string& parent_table)
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

template<typename T>
flx_string db_repository<T>::build_id_selects_recursive(flx_model& model, const flx_string& table_name)
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

template<typename T>
flx_string db_repository<T>::build_hierarchy_query(flx_model& model, const flx_string& root_table, const db_search_criteria& criteria, db_query_builder& builder)
{
  // Build SELECT clause with all IDs
  flx_string id_selects = build_id_selects_recursive(model, root_table);

  // If semantic search is active, prepend distance expression to SELECT
  if (criteria.has_vector_search()) {
    const auto& vec_search = criteria.get_vector_search();

    // Build distance expression: embedding_field <-> '[...]'::vector AS distance
    flx_string vector_literal = "[";
    for (size_t i = 0; i < vec_search.query_embedding.size(); ++i) {
      if (i > 0) vector_literal += ", ";
      vector_literal += flx_string(std::to_string(vec_search.query_embedding[i]).c_str());
    }
    vector_literal += "]";

    flx_string distance_expr = root_table + "." + vec_search.embedding_field +
                                " <-> '" + vector_literal + "'::vector AS distance";

    // Prepend distance to SELECT (needed for ORDER BY)
    id_selects = distance_expr + ", " + id_selects;
  }

  // Build JOIN clauses
  flx_string joins = build_joins_recursive(model, root_table);

  // Build WHERE/ORDER BY/LIMIT using query builder (builder now passed by reference)
  builder.select(id_selects);
  builder.from(root_table);

  // Apply criteria (WHERE, ORDER BY, LIMIT)
  criteria.apply_to(builder);

  // Debug: check if semantic search is active
  if (criteria.has_vector_search()) {
    std::cout << "[DEBUG] Semantic search active, distance field: "
              << criteria.get_vector_search().embedding_field.c_str() << std::endl;
  }

  // Debug: print ORDER BY info before building
  std::cout << "[DEBUG] Builder state before build_select()" << std::endl;

  // Build the query
  flx_string sql = builder.build_select();

  // Debug: print query BEFORE JOIN insertion
  std::cout << "[SQL BEFORE JOIN] Last 400 chars: ..."
            << (sql.length() > 400 ? sql.substr(sql.length() - 400, 400) : sql).c_str() << std::endl;

  // Debug: show last 300 chars of SQL (should include ORDER BY and LIMIT)
  if (sql.length() > 300) {
    std::cout << "[SQL TAIL] ..." << sql.substr(sql.length() - 300, 300).c_str() << std::endl;
  }

  // Insert JOIN clauses after FROM clause
  // Find position after "FROM <table>"
  flx_string from_marker = "FROM " + root_table;
  size_t from_pos = sql.find(from_marker.c_str());

  if (from_pos != flx_string::npos) {
    size_t insert_pos = from_pos + from_marker.length();

    // Check if there's a WHERE clause to insert before
    size_t where_pos = sql.find(" WHERE");
    if (where_pos != flx_string::npos && where_pos > insert_pos) {
      // Insert JOINs before WHERE
      sql = sql.substr(0, insert_pos) + joins + sql.substr(insert_pos);
    } else {
      // No WHERE clause, check for ORDER BY
      size_t order_pos = sql.find(" ORDER BY");
      if (order_pos != flx_string::npos && order_pos > insert_pos) {
        // Insert JOINs before ORDER BY
        sql = sql.substr(0, insert_pos) + joins + sql.substr(insert_pos);
      } else {
        // No WHERE or ORDER BY, check for LIMIT
        size_t limit_pos = sql.find(" LIMIT");
        if (limit_pos != flx_string::npos && limit_pos > insert_pos) {
          // Insert JOINs before LIMIT
          sql = sql.substr(0, insert_pos) + joins + sql.substr(insert_pos);
        } else {
          // No WHERE, ORDER BY, or LIMIT - append at end
          sql = sql.substr(0, insert_pos) + joins + sql.substr(insert_pos);
        }
      }
    }
  }

  return sql;
}

template<typename T>
std::map<flx_string, std::set<long long>> db_repository<T>::parse_hierarchy_results(const std::vector<flxv_map>& rows, flx_model& model, const flx_string& root_table)
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

template<typename T>
std::map<flx_string, std::map<long long, flxv_map>> db_repository<T>::batch_load_rows(const std::map<flx_string, std::set<long long>>& id_sets)
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
      std::cerr << "[BATCH LOAD] Failed to prepare query for " << table.c_str() << ": "
                << query->get_last_error().c_str() << std::endl;
      continue;
    }

    // Bind ID parameters
    for (size_t i = 0; i < id_vector.size(); ++i) {
      flx_string param_name = "id" + flx_string(std::to_string(i).c_str());
      query->bind(param_name, flx_variant(id_vector[i]));
    }

    if (!query->execute()) {
      std::cerr << "[BATCH LOAD] Failed to execute query for " << table.c_str() << ": "
                << query->get_last_error().c_str() << std::endl;
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

template<typename T>
void db_repository<T>::construct_tree_recursive(flx_model& model, const flx_string& table_name, long long id, const std::map<flx_string, std::map<long long, flxv_map>>& all_rows)
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

template<typename T>
bool db_repository<T>::search_hierarchical(const db_search_criteria& criteria, flx_model_list<T>& results)
{
  // Create a temporary model for introspection
  T model;

  // Phase 1: Build and execute ID hierarchy query
  db_query_builder builder;
  flx_string sql = build_hierarchy_query(model, table_name_, criteria, builder);

  auto query = connection_->create_query();
  if (!query) {
    last_error_ = "Failed to create query object";
    return false;
  }

  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare hierarchy query: ") + query->get_last_error();
    return false;
  }

  // Bind parameters from query builder
  const auto& params = builder.get_parameters();
  for (const auto& param : params) {
    query->bind(param.first, param.second);
  }

  if (!query->execute()) {
    last_error_ = flx_string("Failed to execute hierarchy query: ") + query->get_last_error();
    return false;
  }

  // Collect all result rows
  std::vector<flxv_map> rows;
  while (query->next()) {
    rows.push_back(query->get_row());
  }

  if (rows.empty()) {
    return true;  // No results - success but empty
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
          flx_string qualified_id = flx_string(table_name_.c_str()) + flx_string(".id");
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
  auto id_sets = parse_hierarchy_results(rows, model, table_name_);

  // Phase 3: Batch load full rows for each table
  auto all_rows = batch_load_rows(id_sets);

  // Phase 4: Tree construction - for each root ID
  auto root_ids = id_sets[table_name_];
  for (long long root_id : root_ids) {
    T root_model;
    construct_tree_recursive(root_model, table_name_, root_id, all_rows);

    // Attach distance value if available
    if (distance_map.find(root_id) != distance_map.end()) {
      (*root_model)["distance"] = distance_map[root_id];
    }

    results.push_back(root_model);
  }

  return true;
}

// Migration implementations
template<typename T>
bool db_repository<T>::migrate_table()
{
  if (!table_exists()) {
    std::cerr << "[Migration] Table " << table_name_.c_str() << " does not exist, creating..." << std::endl;
    return create_table();
  }

  // Get existing columns from database
  auto existing_cols = get_existing_columns();
  if (existing_cols.empty()) {
    last_error_ = "Failed to query existing columns";
    return false;
  }

  // Get expected columns from model metadata
  auto fields = scan_fields();

  // Find missing columns
  std::vector<field_metadata> missing_cols;
  for (const auto& field : fields) {
    if (field.column_name == id_column_) continue;  // Skip primary key
    if (existing_cols.find(field.column_name) == existing_cols.end()) {
      missing_cols.push_back(field);
    }
  }

  if (missing_cols.empty()) {
    std::cerr << "[Migration] Table " << table_name_.c_str() << " is up to date" << std::endl;
    return true;
  }

  // Add missing columns
  std::cerr << "[Migration] Adding " << missing_cols.size() << " missing columns to " << table_name_.c_str() << std::endl;

  for (const auto& field : missing_cols) {
    flx_string sql_type = get_sql_type_from_state(field.type);
    flx_string alter_sql = "ALTER TABLE " + table_name_ + " ADD COLUMN " + field.column_name + " " + sql_type;

    auto query = connection_->create_query();
    if (!query->prepare(alter_sql)) {
      last_error_ = flx_string("Failed to prepare ALTER TABLE: ") + query->get_last_error();
      std::cerr << "[Migration] ERROR: " << last_error_.c_str() << std::endl;
      return false;
    }

    if (!query->execute()) {
      last_error_ = flx_string("Failed to add column ") + field.column_name + ": " + query->get_last_error();
      std::cerr << "[Migration] ERROR: " << last_error_.c_str() << std::endl;
      return false;
    }

    std::cerr << "[Migration] Added column: " << field.column_name.c_str() << " (" << sql_type.c_str() << ")" << std::endl;
  }

  std::cerr << "[Migration] Table " << table_name_.c_str() << " successfully migrated" << std::endl;
  return true;
}

template<typename T>
std::set<flx_string> db_repository<T>::get_existing_columns()
{
  std::set<flx_string> columns;

  // PostgreSQL-specific query to get column names from information_schema
  flx_string sql = "SELECT column_name FROM information_schema.columns WHERE table_name = :table_name";

  auto query = connection_->create_query();
  if (!query->prepare(sql)) {
    last_error_ = flx_string("Failed to prepare schema query: ") + query->get_last_error();
    return columns;
  }

  query->bind("table_name", flx_variant(table_name_));

  if (!query->execute()) {
    last_error_ = flx_string("Failed to query schema: ") + query->get_last_error();
    return columns;
  }

  while (query->next()) {
    auto row = query->get_row();
    if (row.count("column_name") > 0) {
      columns.insert(row["column_name"].to_string());
    }
  }

  return columns;
}

#endif // DB_REPOSITORY_H
