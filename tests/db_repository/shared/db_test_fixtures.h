#ifndef DB_TEST_FIXTURES_H
#define DB_TEST_FIXTURES_H

#include "../../../api/db/pg_connection.h"
#include "../../../api/db/db_repository.h"
#include "db_test_models.h"
#include <string>
#include <chrono>
#include <atomic>

// ============================================================================
// DATABASE CONNECTION
// ============================================================================

static inline flx_string get_connection_string() {
    const char* host = std::getenv("DB_HOST");
    const char* port = std::getenv("DB_PORT");
    const char* dbname = std::getenv("DB_NAME");
    const char* user = std::getenv("DB_USER");
    const char* password = std::getenv("DB_PASSWORD");

    if (!host || !port || !dbname || !user || !password) {
        throw std::runtime_error("Database credentials not found in environment variables. "
                                 "Ensure .env file exists with DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASSWORD");
    }

    return flx_string("host=") + host + " port=" + port +
           " dbname=" + dbname + " user=" + user + " password=" + password;
}

// Static connection shared by all tests (connect once, reuse everywhere)
static inline pg_connection& get_test_connection() {
    static pg_connection test_conn;
    static bool is_connected = false;

    if (!is_connected) {
        flx_string conn_str = get_connection_string();
        is_connected = test_conn.connect(conn_str);
    }

    return test_conn;
}

// ============================================================================
// TABLE MANAGEMENT
// ============================================================================

static inline void drop_all_test_tables() {
    pg_connection& conn = get_test_connection();
    auto query = conn.create_query();

    std::vector<flx_string> tables = {
        // Hierarchical (drop in reverse order: children first)
        "test_employees",
        "test_departments",
        "test_companies",
        // Simple
        "test_simple_products"
    };

    for (const auto& table : tables) {
        query->prepare(flx_string("DROP TABLE IF EXISTS ") + table + " CASCADE");
        query->execute();
    }
}

// Global test fixture - ensures tables exist before any tests run
static inline bool global_db_setup() {
    static bool setup_done = false;
    static int schema_version = 2; // Increment this when schema changes!
    static int last_schema_version = 0;

    // Force rebuild if schema changed
    if (last_schema_version != schema_version) {
        setup_done = false;
        last_schema_version = schema_version;
    }

    if (setup_done) return true;

    pg_connection& conn = get_test_connection();
    if (!conn.is_connected()) {
        return false;
    }

    // Drop and recreate all tables once at start
    drop_all_test_tables();

    // Create all table structures using repositories
    db_repository repo(&conn);
    test_simple_product simple_sample;
    repo.ensure_structures(simple_sample);

    test_company company_sample;
    repo.ensure_structures(company_sample);  // Creates company + departments tables

    test_employee employee_sample;
    repo.ensure_structures(employee_sample);  // Creates employees table with FK to departments

    setup_done = true;
    return true;
}

// ============================================================================
// TEST DATA CLEANUP - Per-test cleanup using test prefix
// ============================================================================

class db_test_cleanup {
private:
    pg_connection* conn_;
    flx_string test_prefix_;
    std::vector<long long> created_ids_;

public:
    db_test_cleanup(pg_connection* conn, const flx_string& test_name)
        : conn_(conn)
    {
        // Generate unique prefix based on test name + timestamp
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        test_prefix_ = flx_string(test_name.c_str()) + "_" + flx_string(std::to_string(timestamp).c_str()) + "_";
    }

    ~db_test_cleanup() {
        cleanup();
    }

    flx_string prefix() const { return test_prefix_; }

    void track_id(long long id) {
        created_ids_.push_back(id);
    }

    void cleanup() {
        if (conn_ == nullptr || !conn_->is_connected()) return;

        // Delete all tracked records
        for (long long id : created_ids_) {
            auto query = conn_->create_query();
            query->prepare("DELETE FROM test_simple_products WHERE id = :id");
            query->bind("id", flx_variant(id));
            query->execute();
        }

        created_ids_.clear();
    }
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Execute count query
static inline int execute_count_query(pg_connection& conn, const flx_string& sql) {
    auto query = conn.create_query();
    query->prepare(sql);
    query->execute();
    if (query->next()) {
        auto row = query->get_row();
        return static_cast<int>(row["cnt"].int_value());
    }
    return 0;
}

// Compare two products for deep equality
static inline bool products_are_equal(const test_simple_product& a, const test_simple_product& b) {
    // Compare IDs
    if (a.id.is_null() != b.id.is_null()) return false;
    if (!a.id.is_null() && a.id.value() != b.id.value()) return false;

    // Compare name
    if (a.name.is_null() != b.name.is_null()) return false;
    if (!a.name.is_null() && a.name.value() != b.name.value()) return false;

    // Compare price (with epsilon for doubles)
    if (a.price.is_null() != b.price.is_null()) return false;
    if (!a.price.is_null()) {
        double diff = std::abs(a.price.value() - b.price.value());
        if (diff > 0.01) return false;
    }

    // Compare stock_quantity
    if (a.stock_quantity.is_null() != b.stock_quantity.is_null()) return false;
    if (!a.stock_quantity.is_null() && a.stock_quantity.value() != b.stock_quantity.value()) return false;

    // Compare active
    if (a.active.is_null() != b.active.is_null()) return false;
    if (!a.active.is_null() && a.active.value() != b.active.value()) return false;

    return true;
}

#endif // DB_TEST_FIXTURES_H
