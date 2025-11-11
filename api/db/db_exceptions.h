#ifndef DB_EXCEPTIONS_H
#define DB_EXCEPTIONS_H

#include "../../utils/flx_string.h"
#include "../../utils/flx_variant.h"
#include <exception>
#include <stdexcept>

// ============================================================================
// DATABASE EXCEPTION HIERARCHY
// ============================================================================
//
// Exception hierarchy for db_repository error handling:
//
// db_exception (base)
// ├── db_connection_error
// ├── db_query_error
// │   ├── db_prepare_error
// │   └── db_constraint_violation
// │       ├── db_foreign_key_violation
// │       └── db_unique_violation
// ├── db_model_error
// │   ├── db_null_id_error
// │   ├── db_record_not_found
// │   ├── db_no_fields_error
// │   └── db_no_table_name_error
// ├── db_nested_save_error
// └── db_table_not_found
//
// ============================================================================

// Base exception class for all database errors
class db_exception : public std::exception {
protected:
  flx_string message_;
  flx_string sql_;
  flx_string database_error_;

public:
  explicit db_exception(const flx_string& message)
    : message_(message), sql_(""), database_error_("") {}

  db_exception(const flx_string& message, const flx_string& sql)
    : message_(message), sql_(sql), database_error_("") {}

  db_exception(const flx_string& message, const flx_string& sql, const flx_string& database_error)
    : message_(message), sql_(sql), database_error_(database_error) {}

  virtual ~db_exception() noexcept = default;

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

  flx_string get_sql() const { return sql_; }
  flx_string get_database_error() const { return database_error_; }
};

// ============================================================================
// CONNECTION ERRORS
// ============================================================================

class db_connection_error : public db_exception {
public:
  explicit db_connection_error(const flx_string& message)
    : db_exception(message) {}
};

// ============================================================================
// QUERY ERRORS
// ============================================================================

class db_query_error : public db_exception {
public:
  using db_exception::db_exception;
};

class db_prepare_error : public db_query_error {
public:
  using db_query_error::db_query_error;
};

// Constraint violation base class
class db_constraint_violation : public db_query_error {
public:
  using db_query_error::db_query_error;
};

class db_foreign_key_violation : public db_constraint_violation {
private:
  flx_string table_name_;
  flx_string foreign_key_column_;
  flx_string referenced_table_;

public:
  db_foreign_key_violation(const flx_string& table, const flx_string& fk_col,
                            const flx_string& ref_table, const flx_string& sql,
                            const flx_string& database_error)
    : db_constraint_violation(
        flx_string("Foreign key violation: column '") + fk_col +
        "' references non-existent record in '" + ref_table + "'",
        sql, database_error),
      table_name_(table), foreign_key_column_(fk_col), referenced_table_(ref_table) {}

  flx_string get_foreign_key_column() const { return foreign_key_column_; }
  flx_string get_referenced_table() const { return referenced_table_; }
  flx_string get_table_name() const { return table_name_; }
};

class db_unique_violation : public db_constraint_violation {
private:
  flx_string table_name_;
  flx_string column_name_;
  flx_variant duplicate_value_;

public:
  db_unique_violation(const flx_string& table, const flx_string& column, const flx_variant& value)
    : db_constraint_violation(
        flx_string("Unique constraint violation: duplicate value for column '") +
        column + "' in table '" + table + "'"),
      table_name_(table), column_name_(column), duplicate_value_(value) {}

  flx_string get_table_name() const { return table_name_; }
  flx_string get_column_name() const { return column_name_; }
  flx_variant get_duplicate_value() const { return duplicate_value_; }
};

// ============================================================================
// MODEL ERRORS
// ============================================================================

class db_model_error : public db_exception {
public:
  using db_exception::db_exception;
};

class db_null_id_error : public db_model_error {
private:
  flx_string operation_;
  flx_string table_name_;

public:
  db_null_id_error(const flx_string& operation, const flx_string& table_name)
    : db_model_error(
        flx_string("Cannot ") + operation + " record in '" + table_name +
        "': ID is NULL"),
      operation_(operation), table_name_(table_name) {}

  flx_string get_operation() const { return operation_; }
  flx_string get_table_name() const { return table_name_; }
};

class db_record_not_found : public db_model_error {
private:
  flx_string table_name_;
  long long id_;

public:
  db_record_not_found(const flx_string& table_name, long long id)
    : db_model_error(
        flx_string("Record not found in '") + table_name +
        "': ID = " + flx_string(std::to_string(id).c_str())),
      table_name_(table_name), id_(id) {}

  flx_string get_table_name() const { return table_name_; }
  long long get_id() const { return id_; }
};

class db_no_fields_error : public db_model_error {
public:
  explicit db_no_fields_error(const flx_string& table_name)
    : db_model_error(
        flx_string("Model for table '") + table_name +
        "' has no fields with column metadata") {}
};

class db_no_table_name_error : public db_model_error {
public:
  db_no_table_name_error()
    : db_model_error("Model has no primary_key metadata to determine table name") {}
};

// ============================================================================
// HIERARCHICAL ERRORS
// ============================================================================

class db_nested_save_error : public db_exception {
private:
  flx_string parent_table_;
  flx_string child_table_;

public:
  db_nested_save_error(const flx_string& parent_table, const flx_string& child_table,
                        const flx_string& message)
    : db_exception(
        flx_string("Failed to save nested objects: parent='") + parent_table +
        "', child='" + child_table + "': " + message),
      parent_table_(parent_table), child_table_(child_table) {}

  flx_string get_parent_table() const { return parent_table_; }
  flx_string get_child_table() const { return child_table_; }
};

// ============================================================================
// SCHEMA ERRORS
// ============================================================================

class db_table_not_found : public db_exception {
private:
  flx_string table_name_;

public:
  explicit db_table_not_found(const flx_string& table_name)
    : db_exception(flx_string("Table not found: '") + table_name + "'"),
      table_name_(table_name) {}

  flx_string get_table_name() const { return table_name_; }
};

#endif // DB_EXCEPTIONS_H
