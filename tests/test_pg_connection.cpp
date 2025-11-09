#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/pg_query.h>

SCENARIO("pg_connection basic operations") {
  GIVEN("A PostgreSQL connection") {
    pg_connection conn;

    WHEN("Attempting to connect with invalid connection string") {
      bool result = conn.connect("invalid_connection_string");

      THEN("Connection should fail") {
        REQUIRE(result == false);
        REQUIRE(conn.is_connected() == false);
        REQUIRE(conn.get_last_error().empty() == false);
      }
    }

    WHEN("Attempting to connect with valid connection string") {
      flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
      bool result = conn.connect(conn_str);

      if (result) {
        THEN("Connection should succeed") {
          REQUIRE(conn.is_connected() == true);
          REQUIRE(conn.get_last_error().empty() == true);

          AND_WHEN("Disconnecting") {
            bool disconnect_result = conn.disconnect();

            THEN("Disconnection should succeed") {
              REQUIRE(disconnect_result == true);
              REQUIRE(conn.is_connected() == false);
            }
          }
        }
      } else {
        WARN("Skipping connection test - PostgreSQL server not available");
        WARN("Error: " << conn.get_last_error().c_str());
      }
    }
  }
}

SCENARIO("pg_connection query creation") {
  GIVEN("A connected PostgreSQL connection") {
    pg_connection conn;
    flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";

    if (conn.connect(conn_str)) {
      WHEN("Creating a query") {
        auto query = conn.create_query();

        THEN("Query should be created") {
          REQUIRE(query != nullptr);
        }
      }
    } else {
      WARN("Skipping test - PostgreSQL server not available");
    }
  }

  GIVEN("A disconnected PostgreSQL connection") {
    pg_connection conn;

    WHEN("Attempting to create a query") {
      auto query = conn.create_query();

      THEN("Query creation should fail") {
        REQUIRE(query == nullptr);
      }
    }
  }
}

SCENARIO("pg_connection reconnection") {
  GIVEN("A PostgreSQL connection") {
    pg_connection conn;
    flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";

    if (conn.connect(conn_str)) {
      WHEN("Disconnecting and reconnecting") {
        conn.disconnect();
        bool reconnect_result = conn.connect(conn_str);

        THEN("Reconnection should succeed") {
          REQUIRE(reconnect_result == true);
          REQUIRE(conn.is_connected() == true);
        }
      }
    } else {
      WARN("Skipping test - PostgreSQL server not available");
    }
  }
}
