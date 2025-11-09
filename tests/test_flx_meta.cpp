#include <catch2/catch_all.hpp>
#include <utils/flx_model.h>

SCENARIO("flx_model property metadata") {
  GIVEN("A model with properties that have metadata") {
    class User : public flx_model {
    public:
      flxp_int(id);
      flxp_string(email, {
        {flx_string("type"), flx_variant(flx_string("email"))},
        {flx_string("required"), flx_variant(true)},
        {flx_string("max_length"), flx_variant(255LL)}
      });
      flxp_int(age, {
        {flx_string("min"), flx_variant(0LL)},
        {flx_string("max"), flx_variant(120LL)}
      });
      flxp_bool(active);
    };

    User user;

    WHEN("Accessing metadata on email property") {
      const flxv_map& email_meta = user.email.get_meta();

      THEN("Metadata should contain expected values") {
        REQUIRE(email_meta.find("type") != email_meta.end());
        REQUIRE(email_meta.at("type").string_value() == "email");

        REQUIRE(email_meta.find("required") != email_meta.end());
        REQUIRE(email_meta.at("required").bool_value() == true);

        REQUIRE(email_meta.find("max_length") != email_meta.end());
        REQUIRE(email_meta.at("max_length").int_value() == 255);
      }
    }

    WHEN("Accessing metadata on age property") {
      const flxv_map& age_meta = user.age.get_meta();

      THEN("Metadata should contain min/max values") {
        REQUIRE(age_meta.find("min") != age_meta.end());
        REQUIRE(age_meta.at("min").int_value() == 0);

        REQUIRE(age_meta.find("max") != age_meta.end());
        REQUIRE(age_meta.at("max").int_value() == 120);
      }
    }

    WHEN("Accessing metadata on property without metadata") {
      const flxv_map& id_meta = user.id.get_meta();
      const flxv_map& active_meta = user.active.get_meta();

      THEN("Metadata should be empty") {
        REQUIRE(id_meta.empty() == true);
        REQUIRE(active_meta.empty() == true);
      }
    }
  }
}

SCENARIO("flx_model metadata modification") {
  GIVEN("A model with modifiable metadata") {
    class Product : public flx_model {
    public:
      flxp_string(name, {
        {flx_string("label"), flx_variant(flx_string("Product Name"))}
      });
      flxp_double(price);
    };

    Product product;

    WHEN("Adding metadata to a property") {
      flxv_map& price_meta = product.price.get_meta();
      price_meta[flx_string("currency")] = flx_variant(flx_string("EUR"));
      price_meta[flx_string("precision")] = flx_variant(2LL);

      THEN("Metadata should be accessible") {
        const flxv_map& meta = product.price.get_meta();
        REQUIRE(meta.at("currency").string_value() == "EUR");
        REQUIRE(meta.at("precision").int_value() == 2);
      }
    }

    WHEN("Modifying existing metadata") {
      flxv_map& name_meta = product.name.get_meta();
      name_meta[flx_string("label")] = flx_variant(flx_string("Product Title"));
      name_meta[flx_string("hint")] = flx_variant(flx_string("Enter product name"));

      THEN("Metadata should be updated") {
        const flxv_map& meta = product.name.get_meta();
        REQUIRE(meta.at("label").string_value() == "Product Title");
        REQUIRE(meta.at("hint").string_value() == "Enter product name");
        REQUIRE(meta.size() == 2);
      }
    }
  }
}

SCENARIO("flx_model metadata use cases") {
  GIVEN("A model representing a form field") {
    class FormField : public flx_model {
    public:
      flxp_string(username, {
        {flx_string("label"), flx_variant(flx_string("Username"))},
        {flx_string("required"), flx_variant(true)},
        {flx_string("pattern"), flx_variant(flx_string("^[a-zA-Z0-9_]{3,20}$"))},
        {flx_string("error_message"), flx_variant(flx_string("Username must be 3-20 alphanumeric characters"))}
      });

      flxp_string(password, {
        {flx_string("label"), flx_variant(flx_string("Password"))},
        {flx_string("required"), flx_variant(true)},
        {flx_string("min_length"), flx_variant(8LL)},
        {flx_string("input_type"), flx_variant(flx_string("password"))}
      });
    };

    FormField form;

    WHEN("Validating username constraints from metadata") {
      const flxv_map& meta = form.username.get_meta();

      THEN("All validation rules should be accessible") {
        REQUIRE(meta.at("label").string_value() == "Username");
        REQUIRE(meta.at("required").bool_value() == true);
        REQUIRE(meta.at("pattern").string_value() == "^[a-zA-Z0-9_]{3,20}$");
        REQUIRE(meta.at("error_message").string_value() == "Username must be 3-20 alphanumeric characters");
      }
    }

    WHEN("Checking password field metadata") {
      const flxv_map& meta = form.password.get_meta();

      THEN("Password rules should be defined") {
        REQUIRE(meta.at("label").string_value() == "Password");
        REQUIRE(meta.at("required").bool_value() == true);
        REQUIRE(meta.at("min_length").int_value() == 8);
        REQUIRE(meta.at("input_type").string_value() == "password");
      }
    }
  }
}

SCENARIO("flx_model metadata with different data types") {
  GIVEN("A model with various metadata types") {
    class Settings : public flx_model {
    public:
      flxp_int(timeout, {
        {flx_string("unit"), flx_variant(flx_string("seconds"))},
        {flx_string("default"), flx_variant(30LL)},
        {flx_string("adjustable"), flx_variant(true)}
      });

      flxp_double(threshold, {
        {flx_string("range_min"), flx_variant(0.0)},
        {flx_string("range_max"), flx_variant(1.0)},
        {flx_string("step"), flx_variant(0.1)}
      });

      flxp_vector(tags, {
        {flx_string("allowed_values"), flx_variant(flxv_vector{
          flx_variant(flx_string("production")),
          flx_variant(flx_string("staging")),
          flx_variant(flx_string("development"))
        })}
      });
    };

    Settings settings;

    WHEN("Reading metadata with different value types") {
      const flxv_map& timeout_meta = settings.timeout.get_meta();
      const flxv_map& threshold_meta = settings.threshold.get_meta();
      const flxv_map& tags_meta = settings.tags.get_meta();

      THEN("All metadata types should be preserved") {
        REQUIRE(timeout_meta.at("unit").string_value() == "seconds");
        REQUIRE(timeout_meta.at("default").int_value() == 30);
        REQUIRE(timeout_meta.at("adjustable").bool_value() == true);

        REQUIRE(threshold_meta.at("range_min").double_value() == 0.0);
        REQUIRE(threshold_meta.at("range_max").double_value() == 1.0);
        REQUIRE(threshold_meta.at("step").double_value() == 0.1);

        flxv_vector allowed = tags_meta.at("allowed_values").vector_value();
        REQUIRE(allowed.size() == 3);
        REQUIRE(allowed[0].string_value() == "production");
        REQUIRE(allowed[1].string_value() == "staging");
        REQUIRE(allowed[2].string_value() == "development");
      }
    }
  }
}
