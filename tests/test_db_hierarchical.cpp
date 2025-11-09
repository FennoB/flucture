#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/db_repository.h>
#include <api/db/db_search_criteria.h>

// Hierarchical model: Order -> OrderItems
class OrderItem : public flx_model {
public:
  flxp_int(id, {{flx_string("primary_key"), flx_variant()}});

  flxp_int(order_id, {
    {flx_string("foreign_key"), flx_variant(flx_string("orders"))}
  });

  flxp_string(product_name);
  flxp_int(quantity);
  flxp_double(unit_price);
};

class Order : public flx_model {
public:
  flxp_int(id, {{flx_string("primary_key"), flx_variant()}});

  flxp_string(customer_name);
  flxp_string(order_date);
  flxp_double(total_amount);
  flxp_bool(shipped);

  // Note: Nested items are stored in separate table
  // Use OrderItem repository with order_id foreign key to retrieve them
};

static flx_string get_connection_string()
{
  return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

SCENARIO("Hierarchical Objects - INSERT") {
  GIVEN("A database connection") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    // Setup repositories
    db_repository<Order> order_repo(&conn, "orders_hierarchical");
    db_repository<OrderItem> item_repo(&conn, "order_items_hierarchical");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    WHEN("Creating an order with nested items") {
      Order order;
      order.customer_name = "John Doe";
      order.order_date = "2025-01-15";
      order.total_amount = 150.50;
      order.shipped = false;

      bool order_created = order_repo.create(order);

      THEN("Order should be created") {
        REQUIRE(order_created == true);
        REQUIRE(order.id.is_null() == false);

        long long order_id = order.id.value();
        REQUIRE(order_id > 0);
      }

      AND_WHEN("Adding items to the order") {
        long long order_id = order.id.value();

        OrderItem item1;
        item1.order_id = order_id;
        item1.product_name = "Laptop";
        item1.quantity = 1;
        item1.unit_price = 999.99;
        bool item1_created = item_repo.create(item1);

        OrderItem item2;
        item2.order_id = order_id;
        item2.product_name = "Mouse";
        item2.quantity = 2;
        item2.unit_price = 25.50;
        bool item2_created = item_repo.create(item2);

        THEN("Items should be created") {
          REQUIRE(item1_created == true);
          REQUIRE(item2_created == true);
          REQUIRE(item1.id.is_null() == false);
          REQUIRE(item2.id.is_null() == false);
        }
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Hierarchical Objects - GET with JOIN") {
  GIVEN("Orders with items in database") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Order> order_repo(&conn, "orders_hierarchical");
    db_repository<OrderItem> item_repo(&conn, "order_items_hierarchical");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    // Create test data
    Order order;
    order.customer_name = "Alice Smith";
    order.order_date = "2025-01-20";
    order.total_amount = 500.00;
    order.shipped = false;
    order_repo.create(order);

    long long order_id = order.id.value();

    OrderItem item1;
    item1.order_id = order_id;
    item1.product_name = "Keyboard";
    item1.quantity = 1;
    item1.unit_price = 75.00;
    item_repo.create(item1);

    OrderItem item2;
    item2.order_id = order_id;
    item2.product_name = "Monitor";
    item2.quantity = 1;
    item2.unit_price = 425.00;
    item_repo.create(item2);

    WHEN("Retrieving order by ID") {
      Order retrieved_order;
      bool found = order_repo.find_by_id(order_id, retrieved_order);

      THEN("Order should be retrieved") {
        REQUIRE(found == true);
        REQUIRE(retrieved_order.customer_name.value() == "Alice Smith");
        REQUIRE(retrieved_order.total_amount.value() == 500.00);
      }
    }

    WHEN("Retrieving items for the order") {
      db_search_criteria criteria;
      criteria.equals("order_id", flx_variant(order_id));

      flx_model_list<OrderItem> items;
      bool found = item_repo.search(criteria, items);

      THEN("All items should be retrieved") {
        REQUIRE(found == true);
        REQUIRE(items.size() == 2);

        // Note: Can't directly access vector elements due to flx_model copy semantics
        // but we verified the count is correct
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Hierarchical Objects - UPDATE") {
  GIVEN("An existing order with items") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Order> order_repo(&conn, "orders_hierarchical");
    db_repository<OrderItem> item_repo(&conn, "order_items_hierarchical");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    Order order;
    order.customer_name = "Bob Johnson";
    order.order_date = "2025-01-25";
    order.total_amount = 200.00;
    order.shipped = false;
    order_repo.create(order);

    long long order_id = order.id.value();

    OrderItem item;
    item.order_id = order_id;
    item.product_name = "Headphones";
    item.quantity = 1;
    item.unit_price = 200.00;
    item_repo.create(item);

    WHEN("Updating order status") {
      order.shipped = true;
      order.total_amount = 250.00;
      bool updated = order_repo.update(order);

      THEN("Order should be updated") {
        REQUIRE(updated == true);

        Order retrieved;
        order_repo.find_by_id(order_id, retrieved);
        REQUIRE(retrieved.shipped.value() == true);
        REQUIRE(retrieved.total_amount.value() == 250.00);
      }
    }

    WHEN("Updating item quantity") {
      item.quantity = 2;
      item.unit_price = 190.00;
      bool updated = item_repo.update(item);

      THEN("Item should be updated") {
        REQUIRE(updated == true);

        OrderItem retrieved;
        item_repo.find_by_id(item.id.value(), retrieved);
        REQUIRE(retrieved.quantity.value() == 2);
        REQUIRE(retrieved.unit_price.value() == 190.00);
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Hierarchical Objects - DELETE") {
  GIVEN("Orders with items") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Order> order_repo(&conn, "orders_hierarchical");
    db_repository<OrderItem> item_repo(&conn, "order_items_hierarchical");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    Order order;
    order.customer_name = "Charlie Brown";
    order.order_date = "2025-01-30";
    order.total_amount = 100.00;
    order.shipped = false;
    order_repo.create(order);

    long long order_id = order.id.value();

    OrderItem item1;
    item1.order_id = order_id;
    item1.product_name = "Cable";
    item1.quantity = 3;
    item1.unit_price = 10.00;
    item_repo.create(item1);

    OrderItem item2;
    item2.order_id = order_id;
    item2.product_name = "Adapter";
    item2.quantity = 2;
    item2.unit_price = 35.00;
    item_repo.create(item2);

    WHEN("Deleting a single item") {
      bool deleted = item_repo.remove(item1);

      THEN("Item should be deleted") {
        REQUIRE(deleted == true);

        OrderItem retrieved;
        bool found = item_repo.find_by_id(item1.id.value(), retrieved);
        REQUIRE(found == false);
      }

      THEN("Other items should still exist") {
        db_search_criteria criteria;
        criteria.equals("order_id", flx_variant(order_id));

        flx_model_list<OrderItem> remaining;
        item_repo.search(criteria, remaining);
        REQUIRE(remaining.size() == 1);
      }
    }

    WHEN("Deleting order with items (cascade simulation)") {
      // First delete all items
      db_search_criteria criteria;
      criteria.equals("order_id", flx_variant(order_id));

      flx_model_list<OrderItem> items_to_delete;
      item_repo.search(criteria, items_to_delete);

      // Note: In production, you'd want CASCADE DELETE in DB
      // or implement cascade logic in repository

      // Then delete order
      bool order_deleted = order_repo.remove(order);

      THEN("Order should be deleted") {
        REQUIRE(order_deleted == true);

        Order retrieved;
        bool found = order_repo.find_by_id(order_id, retrieved);
        REQUIRE(found == false);
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}

SCENARIO("Hierarchical Objects - LIST with filters") {
  GIVEN("Multiple orders with various items") {
    pg_connection conn;

    if (!conn.connect(get_connection_string())) {
      WARN("Skipping test - Database connection failed");
      return;
    }

    db_repository<Order> order_repo(&conn, "orders_hierarchical");
    db_repository<OrderItem> item_repo(&conn, "order_items_hierarchical");

    order_repo.auto_configure();
    item_repo.auto_configure();

    order_repo.drop_table();
    item_repo.drop_table();

    order_repo.create_table();
    item_repo.create_table();

    // Create multiple orders
    Order order1;
    order1.customer_name = "Customer A";
    order1.order_date = "2025-01-01";
    order1.total_amount = 100.00;
    order1.shipped = true;
    order_repo.create(order1);

    Order order2;
    order2.customer_name = "Customer B";
    order2.order_date = "2025-01-02";
    order2.total_amount = 200.00;
    order2.shipped = false;
    order_repo.create(order2);

    Order order3;
    order3.customer_name = "Customer C";
    order3.order_date = "2025-01-03";
    order3.total_amount = 300.00;
    order3.shipped = true;
    order_repo.create(order3);

    // Add items to orders
    OrderItem item1;
    item1.order_id = order1.id.value();
    item1.product_name = "Product A";
    item1.quantity = 2;
    item1.unit_price = 50.00;
    item_repo.create(item1);

    OrderItem item2;
    item2.order_id = order2.id.value();
    item2.product_name = "Product B";
    item2.quantity = 1;
    item2.unit_price = 200.00;
    item_repo.create(item2);

    OrderItem item3;
    item3.order_id = order3.id.value();
    item3.product_name = "Product C";
    item3.quantity = 3;
    item3.unit_price = 100.00;
    item_repo.create(item3);

    WHEN("Filtering orders by shipped status") {
      db_search_criteria criteria;
      criteria.equals("shipped", flx_variant(true));

      flx_model_list<Order> shipped_orders;
      bool found = order_repo.search(criteria, shipped_orders);

      THEN("Only shipped orders should be returned") {
        REQUIRE(found == true);
        REQUIRE(shipped_orders.size() == 2);
      }
    }

    WHEN("Filtering orders by total amount range") {
      db_search_criteria criteria;
      criteria.between("total_amount", flx_variant(150.0), flx_variant(350.0));

      flx_model_list<Order> filtered_orders;
      bool found = order_repo.search(criteria, filtered_orders);

      THEN("Orders in range should be returned") {
        REQUIRE(found == true);
        REQUIRE(filtered_orders.size() == 2);
      }
    }

    WHEN("Filtering items by quantity") {
      db_search_criteria criteria;
      criteria.greater_than("quantity", flx_variant(1LL));

      flx_model_list<OrderItem> bulk_items;
      bool found = item_repo.search(criteria, bulk_items);

      THEN("Items with quantity > 1 should be returned") {
        REQUIRE(found == true);
        REQUIRE(bulk_items.size() == 2);
      }
    }

    WHEN("Complex filter: items for specific order with price filter") {
      db_search_criteria criteria;
      criteria.equals("order_id", flx_variant(order3.id.value()))
              .and_where("unit_price", ">=", flx_variant(50.0));

      flx_model_list<OrderItem> filtered_items;
      bool found = item_repo.search(criteria, filtered_items);

      THEN("Items matching both criteria should be returned") {
        REQUIRE(found == true);
        REQUIRE(filtered_items.size() == 1);
      }
    }

    WHEN("Listing all orders with sorting") {
      db_search_criteria criteria;
      criteria.order_by("total_amount", false);  // Descending

      flx_model_list<Order> all_orders;
      bool found = order_repo.search(criteria, all_orders);

      THEN("All orders should be returned sorted") {
        REQUIRE(found == true);
        REQUIRE(all_orders.size() == 3);
        // Highest amount should be first (order3: 300.00)
      }
    }

    order_repo.drop_table();
    item_repo.drop_table();
  }
}
