#ifndef DB_QUERY_H
#define DB_QUERY_H

#include "../../utils/flx_string.h"
#include "../../utils/flx_variant.h"
#include <vector>

class db_query {
public:
  virtual ~db_query() = default;

  virtual bool prepare(const flx_string& sql) = 0;
  virtual bool execute() = 0;

  virtual void bind(int index, const flx_variant& value) = 0;
  virtual void bind(const flx_string& name, const flx_variant& value) = 0;

  virtual bool next() = 0;
  virtual flxv_map get_row() = 0;
  virtual std::vector<flxv_map> get_all_rows() = 0;

  virtual int rows_affected() const = 0;
  virtual flx_string get_last_error() const = 0;
  virtual flx_string get_sql() const = 0;
};

#endif // DB_QUERY_H
