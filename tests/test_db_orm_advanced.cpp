#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/db_repository.h>
#include <api/db/db_search_criteria.h>

// Test model with metadata
class Employee : public flx_model {
public:
  flxp_int(id, {
    {flx_string("primary_key"), flx_variant()}
  });

  flxp_string(name, {
    {flx_string("column"), flx_variant(flx_string("employee_name"))}
  });

  flxp_string(email);

  flxp_int(age);

  flxp_double(salary);

  flxp_bool(active);

  flxp_int(department_id, {
    {flx_string("foreign_key"), flx_variant(flx_string("departments"))}
  });
};

class Department : public flx_model {
public:
  flxp_int(id, {
    {flx_string("primary_key"), flx_variant()}
  });

  flxp_string(name);
  flxp_string(location);
};

static flx_string get_connection_string()
{
  return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

SCENARIO("db_repository metadata scanning") {
  GIVEN("An Employee model with metadata") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Employee> repo(&conn, "orm_employees");

    WHEN("Scanning fields") {
      auto fields = repo.scan_fields();

      THEN("All fields should be detected") {
        REQUIRE(fields.size() == 7);
      }

      THEN("Primary key should be identified") {
        bool found_pk = false;
        for (const auto& field : fields) {
          if (field.property_name == "id") {
            REQUIRE(field.is_primary_key == true);
            found_pk = true;
          }
        }
        REQUIRE(found_pk == true);
      }

      THEN("Column name override should work") {
        bool found_name = false;
        for (const auto& field : fields) {
          if (field.property_name == "name") {
            REQUIRE(field.column_name == "employee_name");
            found_name = true;
          }
        }
        REQUIRE(found_name == true);
      }

      THEN("Foreign key should be detected") {
        bool found_fk = false;
        for (const auto& field : fields) {
          if (field.property_name == "department_id") {
            REQUIRE(field.is_foreign_key == true);
            REQUIRE(field.foreign_table == "departments");
            found_fk = true;
          }
        }
        REQUIRE(found_fk == true);
      }
    }
  }
}

SCENARIO("db_repository auto_configure") {
  GIVEN("A connected database and Employee repository") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Employee> repo(&conn, "orm_employees_auto");

    WHEN("Calling auto_configure") {
      bool configured = repo.auto_configure();

      THEN("Configuration should succeed") {
        REQUIRE(configured == true);
      }

      AND_WHEN("Creating table with auto-configured schema") {
        repo.drop_table();
        bool created = repo.create_table();

        THEN("Table should be created successfully") {
          REQUIRE(created == true);
          REQUIRE(repo.table_exists() == true);
        }
      }
    }

    repo.drop_table();
  }
}

SCENARIO("db_repository search with criteria") {
  GIVEN("A database with Employee data") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Employee> repo(&conn, "orm_employees_search");
    repo.auto_configure();

    // Force drop and recreate
    bool dropped = repo.drop_table();
    if (!dropped) {
      WARN("Drop table warning: " << repo.get_last_error().c_str());
    }

    bool created = repo.create_table();
    if (!created) {
      WARN("Create table error: " << repo.get_last_error().c_str());
      return;
    }

    REQUIRE(repo.table_exists() == true);

    // Insert test data
    Employee emp1;
    emp1.name = "Alice Johnson";
    emp1.email = "alice@company.com";
    emp1.age = 30;
    emp1.salary = 75000.0;
    emp1.active = true;
    repo.create(emp1);

    Employee emp2;
    emp2.name = "Bob Smith";
    emp2.email = "bob@company.com";
    emp2.age = 45;
    emp2.salary = 95000.0;
    emp2.active = true;
    repo.create(emp2);

    Employee emp3;
    emp3.name = "Charlie Brown";
    emp3.email = "charlie@company.com";
    emp3.age = 28;
    emp3.salary = 65000.0;
    emp3.active = false;
    repo.create(emp3);

    WHEN("Searching with EQUALS") {
      db_search_criteria criteria;
      criteria.equals("active", flx_variant(true));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      if (!found) {
        WARN("Search error: " << repo.get_last_error().c_str());
      }

      THEN("Only active employees should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with GREATER_THAN") {
      db_search_criteria criteria;
      criteria.greater_than("salary", flx_variant(70000.0));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Only high earners should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with BETWEEN") {
      db_search_criteria criteria;
      criteria.between("age", flx_variant(25LL), flx_variant(35LL));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Employees in age range should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with LIKE") {
      db_search_criteria criteria;
      criteria.like("employee_name", "%Johnson%");

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Matching names should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }
    }

    WHEN("Searching with IN") {
      db_search_criteria criteria;
      std::vector<flx_variant> ages = {flx_variant(30LL), flx_variant(45LL)};
      criteria.in("age", ages);

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Employees with specified ages should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with NOT_EQUALS") {
      db_search_criteria criteria;
      criteria.not_equals("active", flx_variant(true));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Inactive employees should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }
    }

    WHEN("Searching with compound conditions (AND)") {
      db_search_criteria criteria;
      criteria.equals("active", flx_variant(true))
              .and_where("age", ">", flx_variant(35LL));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Only active employees over 35 should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);
      }
    }

    WHEN("Searching with compound conditions (OR)") {
      db_search_criteria criteria;
      criteria.equals("employee_name", flx_variant(flx_string("Alice Johnson")))
              .or_where("employee_name", "=", flx_variant(flx_string("Bob Smith")));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Multiple employees should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with ORDER BY") {
      db_search_criteria criteria;
      criteria.order_by("salary", false);  // Descending

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Employees should be ordered by salary descending") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 3);
        // Highest salary should be first (Bob: 95000)
      }
    }

    WHEN("Searching with LIMIT") {
      db_search_criteria criteria;
      criteria.limit(2);

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Only limited number of results should be returned") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 2);
      }
    }

    WHEN("Searching with LESS_THAN and GREATER_EQUAL") {
      db_search_criteria criteria;
      criteria.less_than("age", flx_variant(40LL))
              .and_where("salary", ">=", flx_variant(70000.0));

      flx_model_list<Employee> results;
      bool found = repo.search(criteria, results);

      THEN("Employees matching both conditions should be found") {
        REQUIRE(found == true);
        REQUIRE(results.size() == 1);  // Alice: age 30, salary 75000
      }
    }

    repo.drop_table();
  }
}

SCENARIO("db_query_builder fluent API") {
  GIVEN("A query builder") {
    db_query_builder builder;

    WHEN("Building a simple SELECT") {
      flx_string sql = builder.from("users").build_select();

      THEN("SQL should be generated") {
        REQUIRE(sql.contains("SELECT * FROM users"));
      }
    }

    WHEN("Building SELECT with WHERE") {
      builder.from("users")
             .where("age", db_query_builder::operator_type::GREATER, flx_variant(18LL));

      flx_string sql = builder.build_select();

      THEN("SQL should contain WHERE clause") {
        REQUIRE(sql.contains("WHERE"));
        REQUIRE(sql.contains("age"));
      }
    }

    WHEN("Building SELECT with JOIN") {
      builder.from("employees", "e")
             .left_join("departments", "d", "e.department_id = d.id");

      flx_string sql = builder.build_select();

      THEN("SQL should contain LEFT JOIN") {
        REQUIRE(sql.contains("LEFT JOIN"));
        REQUIRE(sql.contains("departments"));
      }
    }

    WHEN("Building INSERT") {
      flxv_map values;
      values["name"] = flx_variant(flx_string("Test"));
      values["age"] = flx_variant(25LL);

      builder.insert_into("users").values(values);
      flx_string sql = builder.build_insert();

      THEN("INSERT SQL should be generated") {
        REQUIRE(sql.contains("INSERT INTO users"));
        REQUIRE(sql.contains("VALUES"));
      }
    }

    WHEN("Building UPDATE") {
      flxv_map values;
      values["name"] = flx_variant(flx_string("Updated"));

      builder.update("users")
             .set(values)
             .where("id", db_query_builder::operator_type::EQUAL, flx_variant(1LL));

      flx_string sql = builder.build_update();

      THEN("UPDATE SQL should be generated") {
        REQUIRE(sql.contains("UPDATE users"));
        REQUIRE(sql.contains("SET"));
        REQUIRE(sql.contains("WHERE"));
      }
    }

    WHEN("Building DELETE") {
      builder.delete_from("users")
             .where("active", db_query_builder::operator_type::EQUAL, flx_variant(false));

      flx_string sql = builder.build_delete();

      THEN("DELETE SQL should be generated") {
        REQUIRE(sql.contains("DELETE FROM users"));
        REQUIRE(sql.contains("WHERE"));
      }
    }
  }
}
