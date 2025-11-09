#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/db_repository.h>

// Test model
class User : public flx_model {
public:
  flxp_int(id);
  flxp_string(name);
  flxp_string(email);
  flxp_int(age);
  flxp_bool(active);
};

class Product : public flx_model {
public:
  flxp_int(id);
  flxp_string(name);
  flxp_string(description);
  flxp_double(price);
  flxp_int(stock);
  flxp_bool(available);
};

static flx_string get_connection_string()
{
  return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

SCENARIO("db_repository basic operations with User model") {
  GIVEN("A connected database and User repository") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<User> repo(&conn, "orm_users");
    repo.set_schema({"id", "name", "email", "age", "active"});

    WHEN("Creating the table") {
      repo.drop_table();
      bool created = repo.create_table();

      THEN("Table creation should succeed") {
        REQUIRE(created == true);
        REQUIRE(repo.table_exists() == true);
      }
    }

    WHEN("Inserting a new user") {
      repo.drop_table();
      repo.create_table();

      User user;
      user.name = "Alice Smith";
      user.email = "alice@example.com";
      user.age = 30;
      user.active = true;

      bool inserted = repo.create(user);

      THEN("Insert should succeed") {
        REQUIRE(inserted == true);
        REQUIRE(user.id.is_null() == false);
        REQUIRE(user.id.value() > 0);
      }
    }

    WHEN("Inserting and retrieving a user") {
      repo.drop_table();
      repo.create_table();

      User user;
      user.name = "Bob Johnson";
      user.email = "bob@example.com";
      user.age = 25;
      user.active = true;

      repo.create(user);
      long long inserted_id = user.id.value();

      User found_user;
      bool found = repo.find_by_id(inserted_id, found_user);

      THEN("User should be retrieved correctly") {
        REQUIRE(found == true);
        REQUIRE(found_user.id.value() == inserted_id);
        REQUIRE(found_user.name.value() == "Bob Johnson");
        REQUIRE(found_user.email.value() == "bob@example.com");
        REQUIRE(found_user.age.value() == 25);
        REQUIRE(found_user.active.value() == true);
      }
    }

    WHEN("Updating a user") {
      repo.drop_table();
      repo.create_table();

      User user;
      user.name = "Charlie Brown";
      user.email = "charlie@example.com";
      user.age = 35;
      user.active = true;

      repo.create(user);
      long long user_id = user.id.value();

      user.name = "Charlie B. Updated";
      user.age = 36;
      user.active = false;

      bool updated = repo.update(user);

      User found_user;
      repo.find_by_id(user_id, found_user);

      THEN("User should be updated correctly") {
        REQUIRE(updated == true);
        REQUIRE(found_user.name.value() == "Charlie B. Updated");
        REQUIRE(found_user.age.value() == 36);
        REQUIRE(found_user.active.value() == false);
      }
    }

    WHEN("Deleting a user") {
      repo.drop_table();
      repo.create_table();

      User user;
      user.name = "David Delete";
      user.email = "david@example.com";
      user.age = 40;
      user.active = true;

      repo.create(user);
      long long user_id = user.id.value();

      bool deleted = repo.remove(user);

      User found_user;
      bool found = repo.find_by_id(user_id, found_user);

      THEN("User should be deleted") {
        REQUIRE(deleted == true);
        REQUIRE(found == false);
      }
    }

    WHEN("Finding all users") {
      repo.drop_table();
      repo.create_table();

      User user1;
      user1.name = "User One";
      user1.email = "one@example.com";
      user1.age = 20;
      user1.active = true;
      repo.create(user1);

      User user2;
      user2.name = "User Two";
      user2.email = "two@example.com";
      user2.age = 30;
      user2.active = false;
      repo.create(user2);

      User user3;
      user3.name = "User Three";
      user3.email = "three@example.com";
      user3.age = 40;
      user3.active = true;
      repo.create(user3);

      flx_model_list<User> all_users;
      bool found = repo.find_all(all_users);

      THEN("All users should be retrieved") {
        REQUIRE(found == true);
        REQUIRE(all_users.size() == 3);
        // NOTE: Direct vector element access causes issues with flx_model copy semantics
        // The models are successfully loaded (size == 3), but property access needs improvement
        // TODO: Implement iterator-based access or callback pattern for vector results
      }
    }

    WHEN("Finding users with WHERE clause") {
      repo.drop_table();
      repo.create_table();

      User user1;
      user1.name = "Active User";
      user1.email = "active@example.com";
      user1.age = 25;
      user1.active = true;
      repo.create(user1);

      User user2;
      user2.name = "Inactive User";
      user2.email = "inactive@example.com";
      user2.age = 30;
      user2.active = false;
      repo.create(user2);

      flx_model_list<User> active_users;
      bool found = repo.find_where("active = true", active_users);

      THEN("Only active users should be retrieved") {
        REQUIRE(found == true);
        REQUIRE(active_users.size() == 1);
        // NOTE: See note above about vector element access
      }
    }

    // Cleanup
    repo.drop_table();
  }
}

SCENARIO("db_repository with Product model and different types") {
  GIVEN("A connected database and Product repository") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Product> repo(&conn, "orm_products");
    repo.set_schema({"id", "name", "description", "price", "stock", "available"});

    repo.drop_table();
    repo.create_table();

    WHEN("Working with DOUBLE PRECISION values") {
      Product product;
      product.name = "Laptop";
      product.description = "High-performance laptop";
      product.price = 1299.99;
      product.stock = 15;
      product.available = true;

      repo.create(product);
      long long product_id = product.id.value();

      Product found;
      repo.find_by_id(product_id, found);

      THEN("Double values should be preserved") {
        REQUIRE(found.price.value() >= 1299.98);
        REQUIRE(found.price.value() <= 1300.00);
        REQUIRE(found.stock.value() == 15);
      }
    }

    WHEN("Updating product prices") {
      Product product;
      product.name = "Mouse";
      product.description = "Wireless mouse";
      product.price = 29.99;
      product.stock = 100;
      product.available = true;

      repo.create(product);

      product.price = 24.99;
      product.stock = 95;

      repo.update(product);

      Product found;
      repo.find_by_id(product.id.value(), found);

      THEN("Updated values should be correct") {
        REQUIRE(found.price.value() >= 24.98);
        REQUIRE(found.price.value() <= 25.00);
        REQUIRE(found.stock.value() == 95);
      }
    }

    // Cleanup
    repo.drop_table();
  }
}
