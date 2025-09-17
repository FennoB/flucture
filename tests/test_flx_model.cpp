#include <catch2/catch_all.hpp>
#include <utils/flx_model.h>
#include <utils/flx_string.h>

// Test model class using the new flx_model system
class TestModel : public flx_model {
public:
    flxp_int(id);
    flxp_string(name);
    flxp_bool(active);
    flxp_double(score);
    flxp_vector(tags);
    flxp_map(metadata);
};

SCENARIO("flx_model properties work with const-correct access") {
    GIVEN("A test model instance") {
        TestModel model;
        
        WHEN("Setting values on non-const properties") {
            model.id = 42;
            model.name = "Test User";
            model.active = true;
            model.score = 95.5;
            
            THEN("Values should be stored correctly") {
                REQUIRE(model.id == 42);
                REQUIRE(model.name == "Test User");
                REQUIRE(model.active == true);
                REQUIRE(model.score == 95.5);
            }
        }
        
        WHEN("Using reference assignment syntax") {
            auto& id_ref = model.id;
            flx_string& name_ref = model.name;
            
            id_ref = 123;
            name_ref = "Modified";
            
            THEN("Original model values should change") {
                REQUIRE(model.id == 123);
                REQUIRE(model.name == "Modified");
            }
        }
    }
}

SCENARIO("flx_model properties handle null values correctly") {
    GIVEN("An empty test model") {
        TestModel model;
        
        WHEN("Checking if fields are null") {
            THEN("All fields should initially be null") {
                REQUIRE(model.id.is_null() == true);
                REQUIRE(model.name.is_null() == true);
                REQUIRE(model.active.is_null() == true);
            }
        }
        
        WHEN("Accessing non-const properties") {
            long long id_value = model.id.value();  // Should create default value explicitly
            
            THEN("Property should no longer be null") {
                REQUIRE(model.id.is_null() == false);
                REQUIRE(id_value == 0);  // Default int value
            }
        }
    }
}

SCENARIO("flx_model const access throws exceptions for null fields") {
    GIVEN("An empty test model") {
        TestModel model;
        const TestModel& const_model = model;
        
        WHEN("Accessing null fields in const context") {
            THEN("Should throw flx_null_field_exception") {
                REQUIRE_THROWS_AS(const_model.name.value(), flx_null_field_exception);
                REQUIRE_THROWS_AS(*const_model.id, flx_null_field_exception);
            }
        }
        
        WHEN("Setting a value first, then accessing in const context") {
            model.name = "Test";
            
            THEN("Const access should work") {
                REQUIRE_NOTHROW(const_model.name.value());
                REQUIRE(const_model.name == "Test");
            }
        }
    }
}

SCENARIO("flx_model properties support comparison operators") {
    GIVEN("A test model with values") {
        TestModel model;
        model.id = 42;
        model.name = "Test";
        
        WHEN("Comparing with values") {
            THEN("Equality operators should work") {
                REQUIRE(model.id == 42);
                REQUIRE(model.name == "Test");
                REQUIRE(model.id != 43);
                REQUIRE(model.name != "Other");
            }
        }
    }
}

SCENARIO("flx_lazy_ptr provides const-correct lazy loading") {
    GIVEN("A lazy pointer to a map") {
        flx_lazy_ptr<flxv_map> lazy_map;
        
        WHEN("Accessing non-const") {
            auto& map_ref = *lazy_map;
            map_ref["key"] = "value";
            
            THEN("Object should be created and value stored") {
                REQUIRE_FALSE(lazy_map.is_null());
                REQUIRE((*lazy_map)["key"].string_value() == "value");
            }
        }
        
        WHEN("Accessing const before creation") {
            const auto& const_lazy = lazy_map;
            
            THEN("Should throw flx_null_access_exception") {
                REQUIRE_THROWS_AS(*const_lazy, flx_null_access_exception);
                REQUIRE_THROWS_AS(const_lazy->size(), flx_null_access_exception);
            }
        }
    }
}
