#include <catch2/catch_all.hpp>
#include <utils/flx_model.h>

SCENARIO("flx_model fieldname override via metadata") {
  GIVEN("A model with fieldname overrides") {
    class ApiUser : public flx_model {
    public:
      flxp_int(id);
      flxp_string(email, {
        {flx_string("fieldname"), flx_variant(flx_string("email_address"))}
      });
      flxp_string(first_name, {
        {flx_string("fieldname"), flx_variant(flx_string("firstName"))}
      });
    };

    ApiUser user;

    WHEN("Setting values via properties") {
      user.id = 123;
      user.email = "test@example.com";
      user.first_name = "John";

      THEN("Values should be accessible via properties") {
        REQUIRE(user.id.value() == 123);
        REQUIRE(user.email.value() == "test@example.com");
        REQUIRE(user.first_name.value() == "John");
      }

      THEN("Underlying map should use overridden field names") {
        flxv_map map = *user;

        REQUIRE(map.find("id") != map.end());
        REQUIRE(map["id"].int_value() == 123);

        REQUIRE(map.find("email_address") != map.end());
        REQUIRE(map["email_address"].string_value() == "test@example.com");

        REQUIRE(map.find("firstName") != map.end());
        REQUIRE(map["firstName"].string_value() == "John");
      }

      THEN("Original property names should NOT exist in map") {
        flxv_map map = *user;
        REQUIRE(map.find("email") == map.end());
        REQUIRE(map.find("first_name") == map.end());
      }
    }

    WHEN("Setting values directly in underlying map") {
      flxv_map& map = *user;
      map[flx_string("id")] = flx_variant(456LL);
      map[flx_string("email_address")] = flx_variant(flx_string("direct@example.com"));
      map[flx_string("firstName")] = flx_variant(flx_string("Jane"));

      THEN("Properties should reflect the values") {
        REQUIRE(user.id.value() == 456);
        REQUIRE(user.email.value() == "direct@example.com");
        REQUIRE(user.first_name.value() == "Jane");
      }
    }
  }
}

SCENARIO("flx_model fieldname for JSON mapping") {
  GIVEN("A model representing API response with camelCase fields") {
    class ApiResponse : public flx_model {
    public:
      flxp_int(user_id, {
        {flx_string("fieldname"), flx_variant(flx_string("userId"))}
      });
      flxp_bool(is_active, {
        {flx_string("fieldname"), flx_variant(flx_string("isActive"))}
      });
    };

    ApiResponse response;

    WHEN("Populating from simulated JSON data") {
      flxv_map& map = *response;
      map[flx_string("userId")] = flx_variant(789LL);
      map[flx_string("isActive")] = flx_variant(true);

      THEN("C++ properties should access the camelCase fields") {
        REQUIRE(response.user_id.value() == 789);
        REQUIRE(response.is_active.value() == true);
      }
    }

    WHEN("Setting via C++ properties") {
      response.user_id = 999;
      response.is_active = false;

      THEN("Underlying map should have camelCase keys") {
        flxv_map map = *response;
        REQUIRE(map["userId"].int_value() == 999);
        REQUIRE(map["isActive"].bool_value() == false);
      }
    }
  }
}

SCENARIO("flx_model fieldname without override") {
  GIVEN("A model with properties that don't override fieldname") {
    class SimpleModel : public flx_model {
    public:
      flxp_int(id);
      flxp_string(name);
    };

    SimpleModel model;

    WHEN("Setting and accessing values") {
      model.id = 1;
      model.name = "Test";

      THEN("Map should use original property names") {
        flxv_map map = *model;
        REQUIRE(map["id"].int_value() == 1);
        REQUIRE(map["name"].string_value() == "Test");
      }
    }
  }
}
