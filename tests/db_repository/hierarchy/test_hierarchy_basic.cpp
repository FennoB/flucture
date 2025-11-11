#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include "../../../api/db/db_exceptions.h"

// ============================================================================
// HIERARCHICAL MODELS - Basic nested model operations
// ============================================================================
//
// Tests fundamental hierarchical operations with 3-level model:
//   Company → Departments → Employees
//
// Verifies:
//   - Automatic nested save (save_nested_objects)
//   - Automatic nested load (load_nested_objects)
//   - Round-trip integrity for hierarchies
//   - CASCADE DELETE behavior
//
// USAGE:
//   ./flucture_tests "[hierarchy][basic]"
//   ./flucture_tests "[hierarchy][unit]"
//
// ============================================================================

// ----------------------------------------------------------------------------
// Test #1: Repository saves nested objects automatically
// ----------------------------------------------------------------------------

SCENARIO("Repository saves nested objects automatically on create", "[repo][hierarchy][basic][unit][db]") {
    GIVEN("A company with nested departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_save");

        test_company company;
        company.name = cleanup.prefix() + "TechCorp";
        company.founded_year = "2020";
        company.employee_count = 100;

        // Add departments (1:N)
        test_department dept1, dept2;
        dept1.name = "Engineering";
        dept1.location = "Berlin";
        dept1.budget = 500000.0;

        dept2.name = "Sales";
        dept2.location = "Munich";
        dept2.budget = 300000.0;

        company.departments.push_back(dept1);
        company.departments.push_back(dept2);

        WHEN("Creating the company") {
            REQUIRE_NOTHROW(repo.create(company));

            THEN("Company is created") {
                REQUIRE(!company.id.is_null());
            }

            AND_THEN("Departments are also created") {
                REQUIRE(company.departments.size() == 2);
                REQUIRE(!company.departments[0].id.is_null());
                REQUIRE(!company.departments[1].id.is_null());
            }

            AND_THEN("Foreign keys are set correctly") {
                REQUIRE(company.departments[0].company_id == company.id);
                REQUIRE(company.departments[1].company_id == company.id);
            }

            cleanup.track_id(company.id);
        }
    }
}

// ----------------------------------------------------------------------------
// Test #2: Repository loads nested objects automatically
// ----------------------------------------------------------------------------

SCENARIO("Repository loads nested objects automatically on find_by_id", "[repo][hierarchy][basic][unit][db]") {
    GIVEN("A saved company with departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_load");

        test_company original;
        original.name = cleanup.prefix() + "LoadCorp";

        test_department dept;
        dept.name = "Engineering";
        dept.location = "Berlin";
        original.departments.push_back(dept);

        repo.create(original);
        cleanup.track_id(original.id);

        WHEN("Loading the company by ID") {
            test_company loaded;
            REQUIRE_NOTHROW(repo.find_by_id(original.id, loaded));

            THEN("Departments are loaded automatically") {
                REQUIRE(loaded.departments.size() == 1);
            }

            AND_THEN("Department data is correct") {
                REQUIRE(loaded.departments[0].name == "Engineering");
                REQUIRE(loaded.departments[0].location == "Berlin");
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #3: Repository round-trips 3-level hierarchies
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips 3-level hierarchies", "[repo][hierarchy][basic][integration][db]") {
    GIVEN("A company with departments and employees (3 levels)") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_3level");

        test_company original;
        original.name = cleanup.prefix() + "DeepCorp";
        original.founded_year = "2021";

        // Level 2: Department
        test_department dept;
        dept.name = "Engineering";
        dept.location = "Berlin";
        dept.budget = 500000.0;

        // Level 3: Employees
        test_employee emp1, emp2;
        emp1.name = "Alice";
        emp1.email = "alice@deepcorp.com";
        emp1.salary = 75000.0;

        emp2.name = "Bob";
        emp2.email = "bob@deepcorp.com";
        emp2.salary = 80000.0;

        dept.employees.push_back(emp1);
        dept.employees.push_back(emp2);
        original.departments.push_back(dept);

        WHEN("Saving and loading the company") {
            repo.create(original);
            cleanup.track_id(original.id);

            test_company loaded;
            repo.find_by_id(original.id, loaded);

            THEN("Company data is preserved") {
                REQUIRE(loaded.name == original.name);
                REQUIRE(loaded.founded_year == original.founded_year);
            }

            AND_THEN("Department is preserved") {
                REQUIRE(loaded.departments.size() == 1);
                REQUIRE(loaded.departments[0].name == "Engineering");
                REQUIRE(loaded.departments[0].budget == Catch::Approx(500000.0).epsilon(0.01));
            }

            AND_THEN("Employees are preserved") {
                REQUIRE(loaded.departments[0].employees.size() == 2);
                REQUIRE(loaded.departments[0].employees[0].name == "Alice");
                REQUIRE(loaded.departments[0].employees[1].name == "Bob");
            }

            AND_THEN("All foreign keys are correct") {
                REQUIRE(loaded.departments[0].company_id == loaded.id);
                REQUIRE(loaded.departments[0].employees[0].department_id == loaded.departments[0].id);
                REQUIRE(loaded.departments[0].employees[1].department_id == loaded.departments[0].id);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #4: Repository updates nested objects correctly
// ----------------------------------------------------------------------------

SCENARIO("Repository updates nested objects on parent update", "[repo][hierarchy][update][integration][db]") {
    GIVEN("A saved company with departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_update");

        test_company company;
        company.name = cleanup.prefix() + "UpdateCorp";

        test_department dept1, dept2;
        dept1.name = "Old Dept 1";
        dept2.name = "Old Dept 2";

        company.departments.push_back(dept1);
        company.departments.push_back(dept2);

        repo.create(company);
        cleanup.track_id(company.id);

        WHEN("Modifying departments and updating") {
            // Clear and add new departments
            company.departments.clear();

            test_department dept3;
            dept3.name = "New Dept";
            dept3.location = "Hamburg";
            company.departments.push_back(dept3);

            REQUIRE_NOTHROW(repo.update(company));

            AND_WHEN("Reloading the company") {
                test_company reloaded;
                repo.find_by_id(company.id, reloaded);

                THEN("New departments are present") {
                    REQUIRE(reloaded.departments.size() == 1);
                    REQUIRE(reloaded.departments[0].name == "New Dept");
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #5: Repository handles empty nested lists
// ----------------------------------------------------------------------------

SCENARIO("Repository handles companies with no departments", "[repo][hierarchy][boundary][unit][db]") {
    GIVEN("A company without departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_empty");

        test_company company;
        company.name = cleanup.prefix() + "EmptyCorp";
        // departments list stays empty

        WHEN("Creating the company") {
            REQUIRE_NOTHROW(repo.create(company));

            AND_WHEN("Loading the company") {
                test_company loaded;
                repo.find_by_id(company.id, loaded);

                THEN("Departments list is empty") {
                    REQUIRE(loaded.departments.size() == 0);
                }
            }

            cleanup.track_id(company.id);
        }
    }
}

// ----------------------------------------------------------------------------
// Test #6: Repository preserves multiple children per parent
// ----------------------------------------------------------------------------

SCENARIO("Repository preserves multiple departments and employees", "[repo][hierarchy][basic][integration][db]") {
    GIVEN("A company with 3 departments, each with 2 employees") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "hierarchy_multiple");

        test_company company;
        company.name = cleanup.prefix() + "MultipleCorp";

        // Create 3 departments
        for (int d = 0; d < 3; d++) {
            test_department dept;
            dept.name = "Dept " + std::to_string(d);
            dept.location = "City " + std::to_string(d);

            // Create 2 employees per department
            for (int e = 0; e < 2; e++) {
                test_employee emp;
                emp.name = "Employee " + std::to_string(d) + "-" + std::to_string(e);
                emp.email = "emp" + std::to_string(d) + std::to_string(e) + "@test.com";
                dept.employees.push_back(emp);
            }

            company.departments.push_back(dept);
        }

        WHEN("Saving and loading") {
            repo.create(company);
            cleanup.track_id(company.id);

            test_company loaded;
            repo.find_by_id(company.id, loaded);

            THEN("All departments are present") {
                REQUIRE(loaded.departments.size() == 3);
            }

            AND_THEN("Each department has 2 employees") {
                REQUIRE(loaded.departments[0].employees.size() == 2);
                REQUIRE(loaded.departments[1].employees.size() == 2);
                REQUIRE(loaded.departments[2].employees.size() == 2);
            }

            AND_THEN("Employee data is preserved") {
                REQUIRE(loaded.departments[0].employees[0].name == "Employee 0-0");
                REQUIRE(loaded.departments[2].employees[1].name == "Employee 2-1");
            }
        }
    }
}
