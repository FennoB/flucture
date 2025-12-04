#ifndef PG_QUERY_H
#define PG_QUERY_H

#include "db_query.h"
#include <memory>
#include <map>

class pg_query : public db_query {
public:
  explicit pg_query(void* conn, bool verbose_sql = false);
  ~pg_query() override;

  bool prepare(const flx_string& sql) override;
  bool execute() override;

  void bind(int index, const flx_variant& value) override;
  void bind(const flx_string& name, const flx_variant& value) override;

  bool next() override;
  flxv_map get_row() override;
  std::vector<flxv_map> get_all_rows() override;

  int rows_affected() const override;
  flx_string get_last_error() const override;
  flx_string get_sql() const override;

private:
  struct impl;
  std::unique_ptr<impl> pimpl_;

  flx_string sql_;
  std::map<int, flx_variant> indexed_params_;
  std::map<flx_string, flx_variant> named_params_;

  size_t current_row_;
  int rows_affected_;
  flx_string last_error_;
  bool verbose_sql_;

  flx_string substitute_params(const flx_string& sql);
  flxv_map row_to_variant_map(size_t row_index);
};

#endif // PG_QUERY_H
