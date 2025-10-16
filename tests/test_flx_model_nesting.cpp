#include <catch2/catch_all.hpp>
#include <utils/flx_model.h>

// Test model classes for nesting
class NestedTestModel : public flx_model {
public:
    flxp_string(name);
    flxp_int(value);
};

class ParentTestModel : public flx_model {
public:
    flxp_string(title);
    flxp_model(child, NestedTestModel);
    flxp_model_list(children, NestedTestModel);
};

SCENARIO("flx_model supports nested models") {
    GIVEN("A parent model with nested child model") {
        ParentTestModel parent;
        
        WHEN("Setting values on nested model") {
            parent.title = "Parent Title";
            parent.child.name = "Child Name";
            parent.child.value = 42;
            
            THEN("Values should be accessible") {
                REQUIRE(parent.title == "Parent Title");
                REQUIRE(parent.child.name == "Child Name");
                REQUIRE(parent.child.value == 42);
            }
            
            THEN("Child should not be null") {
                REQUIRE_FALSE(parent.child.name.is_null());
                REQUIRE_FALSE(parent.child.value.is_null());
            }
        }
    }
}

SCENARIO("flx_model_list supports typed model collections") {
    GIVEN("A parent model with model list") {
        ParentTestModel parent;
        
        WHEN("Adding models to the list") {
            NestedTestModel child1;
            child1.name = "First Child";
            child1.value = 10;
            
            NestedTestModel child2;
            child2.name = "Second Child";
            child2.value = 20;
            
            parent.children.push_back(child1);
            parent.children.push_back(child2);
            
            THEN("List should contain the models") {
                REQUIRE(parent.children.size() == 2);
                REQUIRE(parent.children[0].name == "First Child");
                REQUIRE(parent.children[0].value == 10);
                REQUIRE(parent.children[1].name == "Second Child");
                REQUIRE(parent.children[1].value == 20);
            }
            
            THEN("Back() should return last element") {
                REQUIRE(parent.children.back().name == "Second Child");
                REQUIRE(parent.children.back().value == 20);
            }
        }
        
        WHEN("Accessing empty list") {
            THEN("Size should be 0") {
                REQUIRE(parent.children.size() == 0);
            }
            
            THEN("Out of range access should throw") {
                REQUIRE_THROWS_AS(parent.children.at(0), std::out_of_range);
            }
        }
    }
}

SCENARIO("flx_model nesting supports const access") {
    GIVEN("A const parent model with data") {
        ParentTestModel parent;
        parent.child.name = "Test Child";
        parent.child.value = 123;
        
        const ParentTestModel& const_parent = parent;
        
        WHEN("Accessing nested data in const context") {
            THEN("Should be able to read values") {
                REQUIRE(const_parent.child.name == "Test Child");
                REQUIRE(const_parent.child.value == 123);
            }
        }
    }
}

SCENARIO("flx_model_list supports const iteration") {
    GIVEN("A parent with populated model list") {
        ParentTestModel parent;
        
        NestedTestModel child;
        child.name = "Test";
        child.value = 99;
        parent.children.push_back(child);
        
        const ParentTestModel& const_parent = parent;
        
        WHEN("Accessing list in const context") {
            THEN("Should be able to read size and elements") {
                REQUIRE(const_parent.children.size() == 1);
                REQUIRE(const_parent.children.at(0).name == "Test");
                REQUIRE(const_parent.children.at(0).value == 99);
            }
        }
    }
}