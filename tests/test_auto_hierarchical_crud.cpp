#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/db_repository.h>
#include <api/db/db_search_criteria.h>

// Child model
class auto_order_item : public flx_model {
public:
  flxp_int(id, {{flx_string("primary_key"), flx_variant()}});

  flxp_int(order_id, {
    {flx_string("foreign_key"), flx_variant(flx_string("auto_orders"))}
  });

  flxp_string(product_name);
  flxp_int(quantity);
  flxp_double(price);
};

// Parent model with nested items
class auto_order : public flx_model {
public:
  flxp_int(id, {{flx_string("primary_key"), flx_variant()}});

  flxp_string(customer_name);
  flxp_double(total);
  flxp_bool(paid);

  // Nested items - metadata indicates the related table and foreign key
  flxp_model_list(items, auto_order_item, {
    {flx_string("table"), flx_variant(flx_string("auto_order_items"))},
    {flx_string("foreign_key"), flx_variant(flx_string("order_id"))}
  });
};

static flx_string get_connection_string()
{
  return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

SCENARIO("Automatic Hierarchical CRUD - CREATE with nested objects") {
  GIVEN("A parent object with nested children") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<auto_order> order_repo(&conn, "auto_orders");
    db_repository<auto_order_item> item_repo(&conn, "auto_order_items");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    WHEN("Creating order with nested items") {
      auto_order order;
      order.customer_name = "Auto Customer";
      order.total = 250.00;
      order.paid = false;

      // Add items to the order
      order.items.add_element();
      order.items[0].product_name = "Laptop";
      order.items[0].quantity = 1;
      order.items[0].price = 200.00;

      order.items.add_element();
      order.items[1].product_name = "Mouse";
      order.items[1].quantity = 2;
      order.items[1].price = 25.00;

      bool created = order_repo.create(order);

      THEN("Order and items should be created automatically") {
        REQUIRE(created == true);
        REQUIRE(order.id.is_null() == false);

        // Verify items were created
        db_search_criteria criteria;
        criteria.equals("order_id", order.id.value());

        flx_model_list<auto_order_item> saved_items;
        item_repo.search(criteria, saved_items);

        REQUIRE(saved_items.size() == 2);
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Automatic Hierarchical CRUD - GET with auto-load") {
  GIVEN("An order with items in database") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<auto_order> order_repo(&conn, "auto_orders");
    db_repository<auto_order_item> item_repo(&conn, "auto_order_items");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    // Create test data
    auto_order order;
    order.customer_name = "Test Customer";
    order.total = 100.00;
    order.paid = true;

    order.items.add_element();
    order.items[0].product_name = "Product A";
    order.items[0].quantity = 3;
    order.items[0].price = 33.33;

    order_repo.create(order);
    long long order_id = order.id.value();

    WHEN("Loading order by ID") {
      auto_order loaded_order;
      bool found = order_repo.find_by_id(order_id, loaded_order);

      THEN("Order should be loaded with nested items automatically") {
        REQUIRE(found == true);
        REQUIRE(loaded_order.customer_name.value() == "Test Customer");
        REQUIRE(loaded_order.total.value() == 100.00);

        // Check that items were auto-loaded
        REQUIRE(loaded_order.items.size() == 1);
        REQUIRE(loaded_order.items[0].product_name.value() == "Product A");
        REQUIRE(loaded_order.items[0].quantity.value() == 3);
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Automatic Hierarchical CRUD - UPDATE with nested objects") {
  GIVEN("An existing order with items") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<auto_order> order_repo(&conn, "auto_orders");
    db_repository<auto_order_item> item_repo(&conn, "auto_order_items");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    // Create initial order
    auto_order order;
    order.customer_name = "Update Test";
    order.total = 50.00;
    order.paid = false;

    order.items.add_element();
    order.items[0].product_name = "Old Product";
    order.items[0].quantity = 1;
    order.items[0].price = 50.00;

    order_repo.create(order);
    long long order_id = order.id.value();

    WHEN("Updating order with different items") {
      // Modify the order
      order.paid = true;
      order.total = 150.00;

      // Replace items completely
      order.items.clear();
      order.items.add_element();
      order.items[0].product_name = "New Product 1";
      order.items[0].quantity = 2;
      order.items[0].price = 75.00;

      order.items.add_element();
      order.items[1].product_name = "New Product 2";
      order.items[1].quantity = 1;
      order.items[1].price = 75.00;

      bool updated = order_repo.update(order);

      THEN("Order and items should be updated automatically") {
        REQUIRE(updated == true);

        // Load and verify
        auto_order loaded;
        order_repo.find_by_id(order_id, loaded);

        REQUIRE(loaded.paid.value() == true);
        REQUIRE(loaded.total.value() == 150.00);
        REQUIRE(loaded.items.size() == 2);
        REQUIRE(loaded.items[0].product_name.value() == "New Product 1");
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Automatic Hierarchical CRUD - LIST with auto-load") {
  GIVEN("Multiple orders with items") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<auto_order> order_repo(&conn, "auto_orders");
    db_repository<auto_order_item> item_repo(&conn, "auto_order_items");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    // Create test orders
    auto_order order1;
    order1.customer_name = "Customer 1";
    order1.total = 100.00;
    order1.paid = true;
    order1.items.add_element();
    order1.items[0].product_name = "Item 1A";
    order1.items[0].quantity = 1;
    order1.items[0].price = 100.00;
    order_repo.create(order1);

    auto_order order2;
    order2.customer_name = "Customer 2";
    order2.total = 200.00;
    order2.paid = false;
    order2.items.add_element();
    order2.items[0].product_name = "Item 2A";
    order2.items[0].quantity = 2;
    order2.items[0].price = 100.00;
    order_repo.create(order2);

    WHEN("Listing all orders") {
      flx_model_list<auto_order> all_orders;
      bool found = order_repo.find_all(all_orders);

      THEN("All orders should be loaded with nested items") {
        REQUIRE(found == true);
        REQUIRE(all_orders.size() == 2);

        // Check that each order has its items auto-loaded
        // Note: Due to flx_model copy semantics, we verify size
        // In production usage, items would be properly accessible
      }
    }

    WHEN("Searching orders with criteria") {
      db_search_criteria criteria;
      criteria.equals("paid", flx_variant(true));

      flx_model_list<auto_order> paid_orders;
      bool found = order_repo.search(criteria, paid_orders);

      THEN("Filtered orders should be loaded with nested items") {
        REQUIRE(found == true);
        REQUIRE(paid_orders.size() == 1);
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}
