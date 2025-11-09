#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include "db_connection.h"
#include <memory>

class pg_connection : public db_connection {
public:
  pg_connection();
  ~pg_connection() override;

  bool connect(const flx_string& connection_string) override;
  bool disconnect() override;
  bool is_connected() const override;

  std::unique_ptr<db_query> create_query() override;

  flx_string get_last_error() const override;

  void* get_native_connection();

private:
  struct impl;
  std::unique_ptr<impl> pimpl_;
  flx_string last_error_;
  flx_string connection_string_;  // Store for auto-reconnect

  bool reconnect();  // Internal reconnection method
};

#endif // PG_CONNECTION_H
