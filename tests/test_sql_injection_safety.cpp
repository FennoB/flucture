#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/db_repository.h>
#include <api/db/db_search_criteria.h>

class TestUser : public flx_model {
public:
  flxp_int(id, {
    {flx_string("primary_key"), flx_variant()}
  });
  flxp_string(username);
  flxp_string(email);
};

static flx_string get_connection_string()
{
  return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

SCENARIO("SQL Injection Protection") {
  GIVEN("A database with user data") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<TestUser> repo(&conn, "sql_injection_test");
    repo.auto_configure();
    repo.drop_table();
    repo.create_table();

    // Insert normal user
    TestUser user1;
    user1.username = "alice";
    user1.email = "alice@test.com";
    repo.create(user1);

    // Insert user with SQL injection attempt in data
    TestUser user2;
    user2.username = "'; DROP TABLE sql_injection_test; --";
    user2.email = "hacker@test.com";
    bool created = repo.create(user2);

    WHEN("Inserting malicious SQL in username") {
      THEN("Insertion should succeed without executing SQL") {
        REQUIRE(created == true);
      }

      THEN("Table should still exist") {
        REQUIRE(repo.table_exists() == true);
      }

      THEN("Malicious user should be searchable") {
        db_search_criteria criteria;
        criteria.equals("email", flx_variant(flx_string("hacker@test.com")));

        flx_model_list<TestUser> results;
        bool found = repo.search(criteria, results);

        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }
    }

    WHEN("Searching with SQL injection attempt") {
      db_search_criteria criteria;
      // Attempt injection via LIKE pattern
      criteria.like("username", "' OR '1'='1");

      flx_model_list<TestUser> results;
      bool found = repo.search(criteria, results);

      THEN("Search should not return all users") {
        // If injection works, it would return all users
        // If safe, it returns 0 (no match for the literal string)
        REQUIRE(found == true);
        REQUIRE(results.size() == 0);  // No actual match
      }
    }

    WHEN("Searching with dangerous characters") {
      TestUser user3;
      user3.username = "user'; DELETE FROM sql_injection_test WHERE '1'='1";
      user3.email = "dangerous@test.com";
      repo.create(user3);

      db_search_criteria criteria;
      criteria.equals("email", flx_variant(flx_string("dangerous@test.com")));

      flx_model_list<TestUser> results;
      bool found = repo.search(criteria, results);

      THEN("User with dangerous characters should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }

      THEN("Table should still have all records") {
        flx_model_list<TestUser> all_users;
        repo.find_all(all_users);
        REQUIRE(all_users.size() >= 3);  // At least our 3 test users
      }
    }

    repo.drop_table();
  }
}

SCENARIO("Query Builder SQL Injection Protection") {
  GIVEN("A database connection") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<TestUser> repo(&conn, "qb_injection_test");
    repo.auto_configure();
    repo.drop_table();
    repo.create_table();

    TestUser user;
    user.username = "testuser";
    user.email = "test@example.com";
    repo.create(user);

    WHEN("Using search criteria with injection attempts") {
      db_search_criteria criteria;
      criteria.equals("username", flx_variant(flx_string("testuser' OR '1'='1")));

      flx_model_list<TestUser> results;
      bool found = repo.search(criteria, results);

      THEN("Should not match (injection prevented)") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 0);
      }
    }

    WHEN("Using IN clause with injection") {
      db_search_criteria criteria;
      std::vector<flx_variant> values = {
        flx_variant(flx_string("testuser")),
        flx_variant(flx_string("'; DROP TABLE qb_injection_test; --"))
      };
      criteria.in("username", values);

      flx_model_list<TestUser> results;
      bool found = repo.search(criteria, results);

      THEN("Should only match legitimate value") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }

      THEN("Table should still exist") {
        REQUIRE(repo.table_exists() == true);
      }
    }

    repo.drop_table();
  }
}
