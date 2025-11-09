#ifndef DB_SEARCH_CRITERIA_H
#define DB_SEARCH_CRITERIA_H

#include "db_query_builder.h"
#include "../../utils/flx_string.h"
#include "../../utils/flx_variant.h"
#include "../../utils/flx_model.h"
#include <vector>
#include <map>
#include <set>

class db_search_criteria {
public:
  // Semantic search configuration
  struct vector_search_config {
    flx_string embedding_field;        // Field containing vector embedding
    std::vector<double> query_embedding; // Query vector
    int top_k;                          // Number of results
    bool active;                         // Is semantic search active?

    vector_search_config() : top_k(10), active(false) {}
  };

  // Default constructor (backwards compatible)
  db_search_criteria();

  // Hierarchy-aware constructor
  db_search_criteria(flx_model* model, const flx_string& root_table);

  // Fluent where methods
  db_search_criteria& where(const flx_string& field, const flx_string& op, const flx_variant& value);
  db_search_criteria& and_where(const flx_string& field, const flx_string& op, const flx_variant& value);
  db_search_criteria& or_where(const flx_string& field, const flx_string& op, const flx_variant& value);

  // Convenience methods
  db_search_criteria& equals(const flx_string& field, const flx_variant& value);
  db_search_criteria& not_equals(const flx_string& field, const flx_variant& value);
  db_search_criteria& greater_than(const flx_string& field, const flx_variant& value);
  db_search_criteria& less_than(const flx_string& field, const flx_variant& value);
  db_search_criteria& greater_equal(const flx_string& field, const flx_variant& value);
  db_search_criteria& less_equal(const flx_string& field, const flx_variant& value);

  // String operations
  db_search_criteria& like(const flx_string& field, const flx_string& pattern);
  db_search_criteria& not_like(const flx_string& field, const flx_string& pattern);

  // NULL checks
  db_search_criteria& is_null(const flx_string& field);
  db_search_criteria& is_not_null(const flx_string& field);

  // List operations
  db_search_criteria& in(const flx_string& field, const std::vector<flx_variant>& values);
  db_search_criteria& not_in(const flx_string& field, const std::vector<flx_variant>& values);

  // Range operations
  db_search_criteria& between(const flx_string& field, const flx_variant& min, const flx_variant& max);

  // Sorting
  db_search_criteria& order_by(const flx_string& field, bool ascending = true);
  db_search_criteria& order_by_desc(const flx_string& field);

  // Pagination
  db_search_criteria& limit(int count);
  db_search_criteria& offset(int count);

  // Semantic search (pgvector)
  db_search_criteria& semantic_search(
      const flx_string& embedding_field,
      const std::vector<double>& query_embedding,
      int top_k = 10
  );

  // Get vector search config
  const vector_search_config& get_vector_search() const;
  bool has_vector_search() const;

  // Convert to raw SQL WHERE clause (without "WHERE" keyword)
  flx_string to_where_clause() const;

  // Apply criteria to query builder
  void apply_to(db_query_builder& builder) const;

  // Get all conditions
  const std::vector<db_query_builder::condition>& get_conditions() const;

  // Reset criteria
  void reset();

  // Check if empty
  bool is_empty() const;

private:
  // Build column → tables mapping recursively
  void build_column_mapping(flx_model& model, const flx_string& table_name);

  // Qualify a column name based on mapping
  // Returns: table.column or (table1.column OR table2.column OR ...) for multi-table columns
  flx_string qualify_column(const flx_string& column) const;

  // Validate column exists in model (returns true if valid)
  bool is_valid_column(const flx_string& column) const;

  // Validate table.column combination (returns true if valid)
  bool is_valid_qualified_column(const flx_string& table, const flx_string& column) const;

  std::vector<db_query_builder::condition> conditions_;
  std::vector<std::pair<flx_string, bool>> order_by_;
  int limit_;
  int offset_;
  vector_search_config vector_search_;

  // Hierarchy-aware mapping: column_name → vector of table names
  std::map<flx_string, std::vector<flx_string>> column_to_tables_;

  // Set of all valid table names
  std::set<flx_string> valid_tables_;

  // Flag indicating if hierarchy-aware mode is active
  bool hierarchy_mode_;
};

#endif // DB_SEARCH_CRITERIA_H
