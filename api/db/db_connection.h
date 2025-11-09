#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include "../../utils/flx_string.h"
#include <memory>

class db_query;

class db_connection {
public:
  virtual ~db_connection() = default;

  virtual bool connect(const flx_string& connection_string) = 0;
  virtual bool disconnect() = 0;
  virtual bool is_connected() const = 0;

  virtual std::unique_ptr<db_query> create_query() = 0;

  virtual flx_string get_last_error() const = 0;
};

#endif // DB_CONNECTION_H
