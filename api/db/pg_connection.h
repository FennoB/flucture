#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include "db_connection.h"
#include "reconnect_helper.h"
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

  // Reconnect using stored connection string
  bool reconnect() override;

  void* get_native_connection();

  // SQL query logging
  void set_verbose_sql(bool verbose);
  bool get_verbose_sql() const;

private:
  struct impl;
  std::unique_ptr<impl> pimpl_;
  flx_string last_error_;
  flx_string connection_string_;  // Store for auto-reconnect
  bool verbose_sql_;               // Enable SQL query logging
  std::unique_ptr<reconnect_helper> reconnect_helper_;  // Background reconnect thread
};

#endif // PG_CONNECTION_H
