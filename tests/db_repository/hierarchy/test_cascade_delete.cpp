#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include "../../../api/db/db_exceptions.h"

// ============================================================================
// CASCADE DELETE BEHAVIOR
// ============================================================================
//
// Tests that verify CASCADE DELETE integrity in hierarchical models:
//   - Parent deletion cascades to children (1:N relations)
//   - No orphaned records remain in database
//   - Multi-level cascades work correctly (3-level hierarchy)
//   - Foreign key constraints are enforced
//
// CONTRACT:
//   ∀ parent P with children C: delete(P) ⇒ ∀ child c ∈ C: deleted(c)
//
// USAGE:
//   ./flucture_tests "[repo][cascade]"
//   ./flucture_tests "[repo][hierarchy][cascade]"
//
// ============================================================================

// ----------------------------------------------------------------------------
// Test #1: Deleting company cascades to departments
// ----------------------------------------------------------------------------

SCENARIO("Deleting company cascades to all departments", "[repo][cascade][hierarchy][integration][db]") {
    GIVEN("A company with 2 departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_company");

        test_company company;
        company.name = cleanup.prefix() + "CascadeCorp";

        test_department dept1, dept2;
        dept1.name = "Engineering";
        dept2.name = "Sales";
        company.departments.push_back(dept1);
        company.departments.push_back(dept2);

        repo.create(company);
        cleanup.track_id(company.id);

        long long dept1_id = company.departments[0].id.value();
        long long dept2_id = company.departments[1].id.value();

        WHEN("Deleting the company") {
            REQUIRE_NOTHROW(repo.remove(company));

            THEN("Company no longer exists") {
                test_company check_company;
                REQUIRE_THROWS_AS(repo.find_by_id(company.id, check_company), db_record_not_found);
            }

            AND_THEN("First department is also deleted") {
                test_department check_dept;
                REQUIRE_THROWS_AS(repo.find_by_id(dept1_id, check_dept), db_record_not_found);
            }

            AND_THEN("Second department is also deleted") {
                test_department check_dept;
                REQUIRE_THROWS_AS(repo.find_by_id(dept2_id, check_dept), db_record_not_found);
            }

            AND_THEN("No orphaned departments remain") {
                auto query = conn.create_query();
                query->prepare(flx_string("SELECT COUNT(*) as cnt FROM test_departments WHERE company_id = :id"));
                query->bind("id", flx_variant(company.id.value()));
                query->execute();
                if (query->next()) {
                    auto row = query->get_row();
                    REQUIRE(row["cnt"].int_value() == 0);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #2: 3-level cascade (Company → Department → Employee)
// ----------------------------------------------------------------------------

SCENARIO("Deleting company cascades through 3 levels to employees", "[repo][cascade][hierarchy][integration][db]") {
    GIVEN("A company with department containing employees") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_3level");

        test_company company;
        company.name = cleanup.prefix() + "DeepCascade";

        test_department dept;
        dept.name = "Engineering";

        test_employee emp1, emp2;
        emp1.name = "Alice";
        emp1.email = "alice@test.com";
        emp2.name = "Bob";
        emp2.email = "bob@test.com";

        dept.employees.push_back(emp1);
        dept.employees.push_back(emp2);
        company.departments.push_back(dept);

        repo.create(company);
        cleanup.track_id(company.id);

        long long dept_id = company.departments[0].id.value();
        long long emp1_id = company.departments[0].employees[0].id.value();
        long long emp2_id = company.departments[0].employees[1].id.value();

        WHEN("Deleting the company") {
            repo.remove(company);

            THEN("Department is deleted") {
                test_department check_dept;
                REQUIRE_THROWS_AS(repo.find_by_id(dept_id, check_dept), db_record_not_found);
            }

            AND_THEN("First employee is deleted") {
                test_employee check_emp;
                REQUIRE_THROWS_AS(repo.find_by_id(emp1_id, check_emp), db_record_not_found);
            }

            AND_THEN("Second employee is deleted") {
                test_employee check_emp;
                REQUIRE_THROWS_AS(repo.find_by_id(emp2_id, check_emp), db_record_not_found);
            }

            AND_THEN("No orphaned employees remain") {
                auto query = conn.create_query();
                query->prepare(flx_string("SELECT COUNT(*) as cnt FROM test_employees WHERE department_id = :id"));
                query->bind("id", flx_variant(dept_id));
                query->execute();
                if (query->next()) {
                    auto row = query->get_row();
                    REQUIRE(row["cnt"].int_value() == 0);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #3: Deleting middle-level cascades only downward
// ----------------------------------------------------------------------------

SCENARIO("Deleting department cascades to employees but not parent company", "[repo][cascade][hierarchy][integration][db]") {
    GIVEN("A company with department containing employees") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_middle");

        test_company company;
        company.name = cleanup.prefix() + "MiddleCascade";

        test_department dept;
        dept.name = "Engineering";

        test_employee emp;
        emp.name = "Alice";
        emp.email = "alice@test.com";
        dept.employees.push_back(emp);
        company.departments.push_back(dept);

        repo.create(company);
        cleanup.track_id(company.id);

        long long company_id = company.id.value();
        long long dept_id = company.departments[0].id.value();
        long long emp_id = company.departments[0].employees[0].id.value();

        WHEN("Deleting the department") {
            test_department dept_to_delete;
            repo.find_by_id(dept_id, dept_to_delete);
            REQUIRE_NOTHROW(repo.remove(dept_to_delete));

            THEN("Employee is also deleted (cascade downward)") {
                test_employee check_emp;
                REQUIRE_THROWS_AS(repo.find_by_id(emp_id, check_emp), db_record_not_found);
            }

            AND_THEN("Parent company still exists (no upward cascade)") {
                test_company check_company;
                REQUIRE_NOTHROW(repo.find_by_id(company_id, check_company));
                REQUIRE(check_company.name == cleanup.prefix() + "MiddleCascade");
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #4: Cascade with multiple children at each level
// ----------------------------------------------------------------------------

SCENARIO("Cascade delete handles multiple children at each level", "[repo][cascade][hierarchy][integration][db]") {
    GIVEN("A company with 3 departments, each with 2 employees") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_multiple");

        test_company company;
        company.name = cleanup.prefix() + "MultiCascade";

        std::vector<long long> dept_ids;
        std::vector<long long> emp_ids;

        // Create 3 departments with 2 employees each
        for (int d = 0; d < 3; d++) {
            test_department dept;
            dept.name = "Dept" + std::to_string(d);

            for (int e = 0; e < 2; e++) {
                test_employee emp;
                emp.name = "Emp" + std::to_string(d) + "-" + std::to_string(e);
                emp.email = "emp" + std::to_string(d) + std::to_string(e) + "@test.com";
                dept.employees.push_back(emp);
            }

            company.departments.push_back(dept);
        }

        repo.create(company);
        cleanup.track_id(company.id);

        // Collect all IDs
        for (size_t d = 0; d < company.departments.size(); d++) {
            dept_ids.push_back(company.departments[d].id.value());
            for (size_t e = 0; e < company.departments[d].employees.size(); e++) {
                emp_ids.push_back(company.departments[d].employees[e].id.value());
            }
        }

        WHEN("Deleting the company") {
            repo.remove(company);

            THEN("All 3 departments are deleted") {
                int found_count = 0;
                for (long long dept_id : dept_ids) {
                    test_department check;
                    try {
                        repo.find_by_id(dept_id, check);
                        found_count++;
                    } catch (const db_record_not_found&) {
                        // Not found, don't increment
                    }
                }
                REQUIRE(found_count == 0);
            }

            AND_THEN("All 6 employees are deleted") {
                int found_count = 0;
                for (long long emp_id : emp_ids) {
                    test_employee check;
                    try {
                        repo.find_by_id(emp_id, check);
                        found_count++;
                    } catch (const db_record_not_found&) {
                        // Not found, don't increment
                    }
                }
                REQUIRE(found_count == 0);
            }

            AND_THEN("Departments table has no orphaned records") {
                auto query = conn.create_query();
                query->prepare(flx_string("SELECT COUNT(*) as cnt FROM test_departments WHERE company_id = :id"));
                query->bind("id", flx_variant(company.id.value()));
                query->execute();
                if (query->next()) {
                    auto row = query->get_row();
                    REQUIRE(row["cnt"].int_value() == 0);
                }
            }

            AND_THEN("Employees table has no orphaned records for any department") {
                for (long long dept_id : dept_ids) {
                    auto query = conn.create_query();
                    query->prepare(flx_string("SELECT COUNT(*) as cnt FROM test_employees WHERE department_id = :id"));
                    query->bind("id", flx_variant(dept_id));
                    query->execute();
                    if (query->next()) {
                        auto row = query->get_row();
                        REQUIRE(row["cnt"].int_value() == 0);
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #5: Empty parent deletes cleanly (no children to cascade)
// ----------------------------------------------------------------------------

SCENARIO("Deleting company with no departments succeeds without errors", "[repo][cascade][hierarchy][boundary][unit][db]") {
    GIVEN("A company without any departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_empty");

        test_company company;
        company.name = cleanup.prefix() + "EmptyCascade";
        // No departments added

        repo.create(company);
        long long company_id = company.id.value();

        WHEN("Deleting the company") {
            REQUIRE_NOTHROW(repo.remove(company));

            THEN("Company no longer exists") {
                test_company check;
                REQUIRE_THROWS_AS(repo.find_by_id(company_id, check), db_record_not_found);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #6: Cascade preserves siblings
// ----------------------------------------------------------------------------

SCENARIO("Deleting one company does not affect other companies", "[repo][cascade][hierarchy][integration][db]") {
    GIVEN("Two separate companies with departments") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "cascade_siblings");

        // Company 1
        test_company company1;
        company1.name = cleanup.prefix() + "Company1";
        test_department dept1;
        dept1.name = "Dept1";
        company1.departments.push_back(dept1);
        repo.create(company1);
        cleanup.track_id(company1.id);

        // Company 2
        test_company company2;
        company2.name = cleanup.prefix() + "Company2";
        test_department dept2;
        dept2.name = "Dept2";
        company2.departments.push_back(dept2);
        repo.create(company2);
        cleanup.track_id(company2.id);

        long long company2_id = company2.id.value();
        long long dept2_id = company2.departments[0].id.value();

        WHEN("Deleting company1") {
            repo.remove(company1);

            THEN("Company2 still exists") {
                test_company check;
                REQUIRE_NOTHROW(repo.find_by_id(company2_id, check));
            }

            AND_THEN("Company2's department still exists") {
                test_department check;
                REQUIRE_NOTHROW(repo.find_by_id(dept2_id, check));
            }

            AND_THEN("Company2's data is intact") {
                test_company loaded;
                repo.find_by_id(company2_id, loaded);
                REQUIRE(loaded.name == cleanup.prefix() + "Company2");
                REQUIRE(loaded.departments.size() == 1);
                REQUIRE(loaded.departments[0].name == "Dept2");
            }
        }
    }
}
