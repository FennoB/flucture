#include "pg_connection.h"
#include "pg_query.h"
#include "db_exceptions.h"
#include <pqxx/pqxx>
#include <iostream>

struct pg_connection::impl {
  std::unique_ptr<pqxx::connection> conn;
};

pg_connection::pg_connection()
  : pimpl_(std::make_unique<impl>())
  , last_error_("")
  , verbose_sql_(false)
  , reconnect_helper_(std::make_unique<reconnect_helper>(this))
{
}

pg_connection::~pg_connection()
{
  disconnect();
}

bool pg_connection::connect(const flx_string& connection_string)
{
  try {
    connection_string_ = connection_string;  // Store for reconnection
    pimpl_->conn = std::make_unique<pqxx::connection>(connection_string.c_str());
    last_error_ = "";
    return is_connected();
  } catch (const std::exception& e) {
    last_error_ = flx_string("Connection failed: ") + e.what();
    return false;
  }
}

bool pg_connection::disconnect()
{
  if (pimpl_->conn) {
    try {
      pimpl_->conn.reset();
      last_error_ = "";
      return true;
    } catch (const std::exception& e) {
      last_error_ = flx_string("Disconnect failed: ") + e.what();
      return false;
    }
  }
  return true;
}

bool pg_connection::is_connected() const
{
  return pimpl_->conn && pimpl_->conn->is_open();
}

std::unique_ptr<db_query> pg_connection::create_query()
{
  // Check connection health - throw immediately if not connected
  if (!is_connected()) {
    // Start background reconnect thread if not already running
    if (!reconnect_helper_->is_attempting_reconnect()) {
      reconnect_helper_->start_reconnect_loop();
    }

    // Throw immediately - don't wait! API stays responsive
    throw db_not_reachable(
      "Database not reachable. Reconnect in progress.",
      reconnect_helper_->get_retry_after_ms(),
      reconnect_helper_->get_attempt_count()
    );
  }

  return std::make_unique<pg_query>(static_cast<void*>(pimpl_->conn.get()), verbose_sql_);
}

flx_string pg_connection::get_last_error() const
{
  return last_error_;
}

void* pg_connection::get_native_connection()
{
  return static_cast<void*>(pimpl_->conn.get());
}

bool pg_connection::reconnect()
{
  if (connection_string_.empty()) {
    last_error_ = "Cannot reconnect: no connection string stored";
    return false;
  }

  try {
    // Close old connection
    if (pimpl_->conn) {
      pimpl_->conn.reset();
    }

    // Create new connection
    pimpl_->conn = std::make_unique<pqxx::connection>(connection_string_.c_str());

    if (is_connected()) {
      last_error_ = "";
      return true;
    } else {
      last_error_ = "Reconnection failed: connection not open";
      return false;
    }
  } catch (const std::exception& e) {
    last_error_ = flx_string("Reconnection failed: ") + e.what();
    return false;
  }
}

void pg_connection::set_verbose_sql(bool verbose)
{
  verbose_sql_ = verbose;
}

bool pg_connection::get_verbose_sql() const
{
  return verbose_sql_;
}
