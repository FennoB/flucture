#ifndef DB_TEST_MODELS_H
#define DB_TEST_MODELS_H

#include "../../../utils/flx_model.h"

// ============================================================================
// SIMPLE FLAT MODEL - For basic CRUD tests
// ============================================================================

class test_simple_product : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_simple_products"}});
    flxp_string(name, {{"column", "name"}, {"unique", "true"}});
    flxp_double(price, {{"column", "price"}});
    flxp_int(stock_quantity, {{"column", "stock_quantity"}});
    flxp_bool(active, {{"column", "active"}});
};

// ============================================================================
// HIERARCHICAL MODELS - For nested model tests (3 levels)
// ============================================================================

// Level 3: Employee (child of Department)
class test_employee : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_employees"}});
    flxp_int(department_id, {{"foreign_key", "test_departments"}, {"column", "department_id"}});
    flxp_string(name, {{"column", "name"}});
    flxp_string(email, {{"column", "email"}});
    flxp_double(salary, {{"column", "salary"}});
};

// Level 2: Department (child of Company, parent of Employee)
class test_department : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_departments"}});
    flxp_int(company_id, {{"foreign_key", "test_companies"}, {"column", "company_id"}});
    flxp_string(name, {{"column", "name"}, {"not_null", "true"}});
    flxp_string(location, {{"column", "location"}});
    flxp_double(budget, {{"column", "budget"}});

    // 1:N relation to employees
    flxp_model_list(employees, test_employee);
};

// Level 1: Company (root)
class test_company : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_companies"}});
    flxp_string(name, {{"column", "name"}});
    flxp_string(founded_year, {{"column", "founded_year"}});
    flxp_int(employee_count, {{"column", "employee_count"}});

    // 1:N relation to departments
    flxp_model_list(departments, test_department);
};

#endif // DB_TEST_MODELS_H
