#include <catch2/catch_all.hpp>
#include <utils/flx_model.h>

SCENARIO("flx_model nested path access with /") {
  GIVEN("A model with nested path properties") {
    class ApiResponse : public flx_model {
    public:
      flxp_string(user_email, {
        {flx_string("fieldname"), flx_variant(flx_string("user/email"))}
      });
      flxp_string(user_name, {
        {flx_string("fieldname"), flx_variant(flx_string("user/name"))}
      });
      flxp_int(user_age, {
        {flx_string("fieldname"), flx_variant(flx_string("user/age"))}
      });
    };

    ApiResponse response;

    WHEN("Setting values via properties") {
      response.user_email = "test@example.com";
      response.user_name = "John Doe";
      response.user_age = 30;

      THEN("Values should be accessible via properties") {
        REQUIRE(response.user_email.value() == "test@example.com");
        REQUIRE(response.user_name.value() == "John Doe");
        REQUIRE(response.user_age.value() == 30);
      }

      THEN("Underlying map should have nested structure") {
        flxv_map map = *response;

        REQUIRE(map.find("user") != map.end());
        REQUIRE(map["user"].in_state() == flx_variant::map_state);

        flxv_map user_map = map["user"].to_map();
        REQUIRE(user_map.find("email") != user_map.end());
        REQUIRE(user_map["email"].string_value() == "test@example.com");

        REQUIRE(user_map.find("name") != user_map.end());
        REQUIRE(user_map["name"].string_value() == "John Doe");

        REQUIRE(user_map.find("age") != user_map.end());
        REQUIRE(user_map["age"].int_value() == 30);
      }
    }

    WHEN("Setting values directly in nested map") {
      flxv_map& map = *response;

      flxv_map user_map;
      user_map[flx_string("email")] = flx_variant(flx_string("direct@example.com"));
      user_map[flx_string("name")] = flx_variant(flx_string("Jane Smith"));
      user_map[flx_string("age")] = flx_variant(25LL);

      map[flx_string("user")] = flx_variant(user_map);

      THEN("Properties should reflect nested values") {
        REQUIRE(response.user_email.value() == "direct@example.com");
        REQUIRE(response.user_name.value() == "Jane Smith");
        REQUIRE(response.user_age.value() == 25);
      }
    }
  }
}

SCENARIO("flx_model deep nested path access") {
  GIVEN("A model with deeply nested properties") {
    class Config : public flx_model {
    public:
      flxp_string(db_host, {
        {flx_string("fieldname"), flx_variant(flx_string("database/connection/host"))}
      });
      flxp_int(db_port, {
        {flx_string("fieldname"), flx_variant(flx_string("database/connection/port"))}
      });
      flxp_string(api_key, {
        {flx_string("fieldname"), flx_variant(flx_string("api/credentials/key"))}
      });
    };

    Config config;

    WHEN("Setting deeply nested values") {
      config.db_host = "localhost";
      config.db_port = 5432;
      config.api_key = "secret123";

      THEN("Values should be accessible") {
        REQUIRE(config.db_host.value() == "localhost");
        REQUIRE(config.db_port.value() == 5432);
        REQUIRE(config.api_key.value() == "secret123");
      }

      THEN("Nested structure should be created") {
        flxv_map map = *config;

        // Check database/connection/host path
        REQUIRE(map.find("database") != map.end());
        flxv_map db_map = map["database"].to_map();
        REQUIRE(db_map.find("connection") != db_map.end());
        flxv_map conn_map = db_map["connection"].to_map();
        REQUIRE(conn_map["host"].string_value() == "localhost");
        REQUIRE(conn_map["port"].int_value() == 5432);

        // Check api/credentials/key path
        REQUIRE(map.find("api") != map.end());
        flxv_map api_map = map["api"].to_map();
        REQUIRE(api_map.find("credentials") != api_map.end());
        flxv_map cred_map = api_map["credentials"].to_map();
        REQUIRE(cred_map["key"].string_value() == "secret123");
      }
    }
  }
}

SCENARIO("flx_model mixed regular and nested properties") {
  GIVEN("A model with both regular and nested properties") {
    class User : public flx_model {
    public:
      flxp_int(id);  // Regular property
      flxp_string(name);  // Regular property
      flxp_string(profile_bio, {
        {flx_string("fieldname"), flx_variant(flx_string("profile/bio"))}
      });
      flxp_string(profile_avatar, {
        {flx_string("fieldname"), flx_variant(flx_string("profile/avatar_url"))}
      });
      flxp_bool(active);  // Regular property
    };

    User user;

    WHEN("Setting both types of properties") {
      user.id = 1;
      user.name = "Alice";
      user.profile_bio = "Software Developer";
      user.profile_avatar = "https://example.com/avatar.jpg";
      user.active = true;

      THEN("All properties should be accessible") {
        REQUIRE(user.id.value() == 1);
        REQUIRE(user.name.value() == "Alice");
        REQUIRE(user.profile_bio.value() == "Software Developer");
        REQUIRE(user.profile_avatar.value() == "https://example.com/avatar.jpg");
        REQUIRE(user.active.value() == true);
      }

      THEN("Map should have correct structure") {
        flxv_map map = *user;

        // Regular properties at root level
        REQUIRE(map["id"].int_value() == 1);
        REQUIRE(map["name"].string_value() == "Alice");
        REQUIRE(map["active"].bool_value() == true);

        // Nested properties under profile
        REQUIRE(map.find("profile") != map.end());
        flxv_map profile = map["profile"].to_map();
        REQUIRE(profile["bio"].string_value() == "Software Developer");
        REQUIRE(profile["avatar_url"].string_value() == "https://example.com/avatar.jpg");
      }
    }
  }
}

SCENARIO("flx_model nested path with auto-creation") {
  GIVEN("A model with nested paths") {
    class Data : public flx_model {
    public:
      flxp_string(deep_value, {
        {flx_string("fieldname"), flx_variant(flx_string("level1/level2/level3/value"))}
      });
    };

    Data data;

    WHEN("Setting a deeply nested value on empty model") {
      data.deep_value = "test";

      THEN("All intermediate maps should be auto-created") {
        flxv_map map = *data;

        REQUIRE(map.find("level1") != map.end());
        flxv_map l1 = map["level1"].to_map();

        REQUIRE(l1.find("level2") != l1.end());
        flxv_map l2 = l1["level2"].to_map();

        REQUIRE(l2.find("level3") != l2.end());
        flxv_map l3 = l2["level3"].to_map();

        REQUIRE(l3["value"].string_value() == "test");
      }
    }
  }
}
