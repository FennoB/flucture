#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/pg_query.h>

static bool setup_test_database(pg_connection& conn)
{
  auto query = conn.create_query();
  if (!query) {
    return false;
  }

  query->prepare("DROP TABLE IF EXISTS test_users");
  query->execute();

  query->prepare(
    "CREATE TABLE test_users ("
    "  id SERIAL PRIMARY KEY,"
    "  name VARCHAR(100) NOT NULL,"
    "  age INTEGER,"
    "  score DOUBLE PRECISION,"
    "  active BOOLEAN"
    ")"
  );

  return query->execute();
}

SCENARIO("pg_query basic operations") {
  GIVEN("A connected PostgreSQL connection") {
    pg_connection conn;
    flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";

    if (!conn.connect(conn_str)) {
      WARN("Skipping test - PostgreSQL server not available");
      return;
    }

    if (!setup_test_database(conn)) {
      WARN("Skipping test - Could not setup test database");
      return;
    }

    WHEN("Inserting data with named parameters") {
      auto query = conn.create_query();
      query->prepare("INSERT INTO test_users (name, age, score, active) VALUES (:name, :age, :score, :active)");
      query->bind("name", flx_variant("Alice"));
      query->bind("age", flx_variant(30));
      query->bind("score", flx_variant(95.5));
      query->bind("active", flx_variant(true));

      bool result = query->execute();

      THEN("Insert should succeed") {
        REQUIRE(result == true);
        REQUIRE(query->rows_affected() == 1);
      }
    }

    WHEN("Inserting data with indexed parameters") {
      auto query = conn.create_query();
      query->prepare("INSERT INTO test_users (name, age, score, active) VALUES ($1, $2, $3, $4)");
      query->bind(1, flx_variant("Bob"));
      query->bind(2, flx_variant(25));
      query->bind(3, flx_variant(88.0));
      query->bind(4, flx_variant(false));

      bool result = query->execute();

      THEN("Insert should succeed") {
        REQUIRE(result == true);
        REQUIRE(query->rows_affected() == 1);
      }
    }

    WHEN("Querying data") {
      auto insert_query = conn.create_query();
      insert_query->prepare("INSERT INTO test_users (name, age, score, active) VALUES ('Charlie', 35, 92.3, true)");
      insert_query->execute();

      auto query = conn.create_query();
      query->prepare("SELECT * FROM test_users WHERE name = 'Charlie'");
      bool result = query->execute();

      THEN("Query should succeed") {
        REQUIRE(result == true);

        AND_WHEN("Iterating through results") {
          bool has_row = query->next();

          THEN("Should have at least one row") {
            REQUIRE(has_row == true);

            auto row = query->get_row();
            REQUIRE(row["name"].string_value() == "Charlie");
            REQUIRE(row["age"].int_value() == 35);
            REQUIRE(row["score"].double_value() >= 92.2);
            REQUIRE(row["score"].double_value() <= 92.4);
            REQUIRE(row["active"].bool_value() == true);
          }
        }
      }
    }

    WHEN("Getting all rows at once") {
      auto insert_query = conn.create_query();
      insert_query->prepare("INSERT INTO test_users (name, age, score, active) VALUES ('David', 40, 87.5, true)");
      insert_query->execute();
      insert_query->prepare("INSERT INTO test_users (name, age, score, active) VALUES ('Eve', 28, 94.2, false)");
      insert_query->execute();

      auto query = conn.create_query();
      query->prepare("SELECT * FROM test_users WHERE name IN ('David', 'Eve') ORDER BY name");
      query->execute();

      auto rows = query->get_all_rows();

      THEN("Should return all matching rows") {
        REQUIRE(rows.size() == 2);
        REQUIRE(rows[0]["name"].string_value() == "David");
        REQUIRE(rows[1]["name"].string_value() == "Eve");
      }
    }
  }
}

SCENARIO("pg_query error handling") {
  GIVEN("A connected PostgreSQL connection") {
    pg_connection conn;
    flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";

    if (!conn.connect(conn_str)) {
      WARN("Skipping test - PostgreSQL server not available");
      return;
    }

    WHEN("Executing invalid SQL") {
      auto query = conn.create_query();
      query->prepare("SELECT * FROM non_existent_table");
      bool result = query->execute();

      THEN("Execution should fail") {
        REQUIRE(result == false);
        REQUIRE(query->get_last_error().empty() == false);
      }
    }
  }
}

SCENARIO("pg_query null value handling") {
  GIVEN("A connected PostgreSQL connection with test data") {
    pg_connection conn;
    flx_string conn_str = "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";

    if (!conn.connect(conn_str)) {
      WARN("Skipping test - PostgreSQL server not available");
      return;
    }

    if (!setup_test_database(conn)) {
      WARN("Skipping test - Could not setup test database");
      return;
    }

    WHEN("Inserting null values") {
      auto query = conn.create_query();
      query->prepare("INSERT INTO test_users (name, age, score, active) VALUES (:name, :age, :score, :active)");
      query->bind("name", flx_variant("NullTest"));
      query->bind("age", flx_variant());
      query->bind("score", flx_variant());
      query->bind("active", flx_variant(true));

      bool result = query->execute();

      THEN("Insert should succeed") {
        REQUIRE(result == true);

        auto select_query = conn.create_query();
        select_query->prepare("SELECT * FROM test_users WHERE name = 'NullTest'");
        select_query->execute();
        select_query->next();
        auto row = select_query->get_row();

        REQUIRE(row["name"].string_value() == "NullTest");
        REQUIRE(row["age"].is_null() == true);
        REQUIRE(row["score"].is_null() == true);
        REQUIRE(row["active"].bool_value() == true);
      }
    }
  }
}
