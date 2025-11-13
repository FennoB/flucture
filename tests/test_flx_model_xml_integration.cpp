/*
 * ============================================================================
 * XML → flx_model Integration Tests
 * ============================================================================
 *
 * Tests the complete integration of:
 * - flx_xml (pugixml wrapper for XML parsing)
 * - flx_variant (type system with maps/vectors)
 * - flx_model (property system with xml_path metadata)
 *
 * **Run these tests:**
 *
 *   cd build
 *   ./flucture_tests "[xml_model_integration]"
 *
 * **New API (absolute paths with [] placeholders):**
 *
 *   model.read_xml(xml, "root/path")
 *
 * **Test Coverage:**
 * - Basic type mapping (string, int, double, bool)
 * - Fieldname override (store under different name in internal map)
 * - Nested paths (e.g., "level1/level2/value")
 * - Nested models (flxp_model)
 * - Model lists (flxp_model_list) with [] placeholder
 * - XML attributes via @ prefix (e.g., "@id")
 * - XML text content via #text
 * - Nested paths to attributes (e.g., "config/database/@host")
 * - Mixed content (attributes + text + child elements)
 * - Fieldname override combined with @ and # paths
 * - Edge cases: empty attributes, missing data, partial XML
 * - Deep nesting: departments[]/teams[]/members[]
 *
 * ============================================================================
 */

#include <catch2/catch_all.hpp>
#include <api/xml/flx_xml.h>
#include <utils/flx_model.h>

// ============================================================================
// Test Models - Increasing Complexity
// ============================================================================

// Level 1: Simple model with basic types
class SimpleData : public flx_model {
public:
    flxp_string(text, {{"xml_path", "text"}});
    flxp_int(number, {{"xml_path", "number"}});
    flxp_double(score, {{"xml_path", "score"}});
    flxp_bool(active, {{"xml_path", "active"}});
};

// Level 2: Model with fieldname override
class FieldNameTest : public flx_model {
public:
    // Property name in C++ is "cpp_name", but stored as "storage_name" in map
    flxp_string(cpp_name, {{"xml_path", "xml_name"}, {"fieldname", "storage_name"}});
    flxp_int(normal_field, {{"xml_path", "normal"}});
};

// Level 3: Model with nested path (using "/" in xml_path)
class NestedPathData : public flx_model {
public:
    flxp_string(deep_value, {{"xml_path", "level1/level2/value"}});
    flxp_int(deep_number, {{"xml_path", "level1/number"}});
    flxp_string(root_value, {{"xml_path", "root"}});
};

// Level 4: Nested models
class Address : public flx_model {
public:
    flxp_string(street, {{"xml_path", "street"}});
    flxp_string(city, {{"xml_path", "city"}});
    flxp_int(zip, {{"xml_path", "zipcode"}});  // Different XML name than C++ property
};

class Contact : public flx_model {
public:
    // Using fieldname override AND custom xml_path
    flxp_string(email, {{"xml_path", "email_address"}, {"fieldname", "contact_email"}});
    flxp_string(phone, {{"xml_path", "phone"}});
};

class Person : public flx_model {
public:
    flxp_string(name, {{"xml_path", "fullname"}});
    flxp_int(age, {{"xml_path", "age"}});
    flxp_model(address, Address, {{"xml_path", "address"}});
    flxp_model(contact, Contact, {{"xml_path", "contact"}});
};

// Level 5: Model lists with [] placeholder
class Task : public flx_model {
public:
    flxp_string(title, {{"xml_path", "title"}});
    flxp_bool(completed, {{"xml_path", "done"}});
    flxp_int(priority, {{"xml_path", "priority"}});
};

class Team : public flx_model {
public:
    flxp_string(name, {{"xml_path", "teamname"}});
    flxp_model_list(members, Person, {{"xml_path", "member[]"}});  // [] PLACEHOLDER!
    flxp_model_list(tasks, Task, {{"xml_path", "task[]"}});        // [] PLACEHOLDER!
};

// Level 6: Deep nesting with nested paths, model lists, and fieldname overrides
class Department : public flx_model {
public:
    flxp_string(dept_name, {{"xml_path", "name"}, {"fieldname", "department_name"}});
    flxp_model_list(teams, Team, {{"xml_path", "team[]"}});  // [] PLACEHOLDER!
    flxp_int(budget, {{"xml_path", "financials/budget"}});
    flxp_string(currency, {{"xml_path", "financials/currency"}});
    flxp_string(manager, {{"xml_path", "meta/manager/name"}});  // 3-level nested path
};

// Level 7: Root container with everything
class Organization : public flx_model {
public:
    flxp_string(company, {{"xml_path", "company_name"}});
    flxp_model_list(departments, Department, {{"xml_path", "department[]"}});  // [] PLACEHOLDER!
    flxp_int(founded, {{"xml_path", "metadata/founded"}});
};

// Level 8: Model with attributes (XML attributes stored with @ prefix)
class ProductWithAttrs : public flx_model {
public:
    flxp_int(product_id, {{"xml_path", "@id"}});  // XML attribute
    flxp_string(category, {{"xml_path", "@category"}});
    flxp_string(name, {{"xml_path", "#text"}});  // Text content
};

// Level 9: Mixed content - attributes, text, and child elements
class MixedContentNode : public flx_model {
public:
    flxp_string(node_type, {{"xml_path", "@type"}});
    flxp_int(node_id, {{"xml_path", "@id"}});
    flxp_string(text_content, {{"xml_path", "#text"}});
    flxp_string(child_value, {{"xml_path", "child"}});
};

// Level 10: Nested paths to attributes and text
class DeepAttributeAccess : public flx_model {
public:
    // Nested path to attribute: config/database/@host
    flxp_string(db_host, {{"xml_path", "config/database/@host"}});
    flxp_int(db_port, {{"xml_path", "config/database/@port"}});
    // Nested path to text content: config/database/#text (e.g. connection string)
    flxp_string(db_connection, {{"xml_path", "config/database/#text"}});
    // Regular nested element
    flxp_string(app_name, {{"xml_path", "config/app/name"}});
};

// Level 11: Model list where elements have attributes
class TaggedItem : public flx_model {
public:
    flxp_int(item_id, {{"xml_path", "@id"}});
    flxp_string(item_type, {{"xml_path", "@type"}});
    flxp_string(label, {{"xml_path", "#text"}});
};

class TaggedCollection : public flx_model {
public:
    flxp_string(collection_name, {{"xml_path", "@name"}});
    flxp_model_list(items, TaggedItem, {{"xml_path", "item[]"}});  // [] PLACEHOLDER!
};

// Level 12: Fieldname override WITH attribute access
class AdvancedFieldMapping : public flx_model {
public:
    // Attribute with fieldname override
    flxp_int(cpp_id, {{"xml_path", "@xml_id"}, {"fieldname", "stored_id"}});
    // Text content with fieldname override
    flxp_string(cpp_text, {{"xml_path", "#text"}, {"fieldname", "stored_text"}});
    // Regular element with fieldname override
    flxp_string(cpp_name, {{"xml_path", "xml_name"}, {"fieldname", "stored_name"}});
};

// ============================================================================
// SCENARIO 1: Basic XML to Model Mapping
// ============================================================================

SCENARIO("flx_model basic XML mapping with all data types", "[xml_model_integration]") {
    GIVEN("Simple XML with different data types") {
        flx_string xml_str = R"(
            <data>
                <text>Hello World</text>
                <number>42</number>
                <score>98.5</score>
                <active>true</active>
            </data>
        )";

        WHEN("Parsing and applying to model") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            SimpleData model;
            model.read_xml(xml, "data");  // NEW API: parser + root path

            THEN("All properties should be mapped with correct types") {
                REQUIRE(model.text.value() == "Hello World");
                REQUIRE(model.number.value() == 42);
                REQUIRE(model.score.value() == Catch::Approx(98.5));
                REQUIRE(model.active.value() == true);
            }
        }
    }
}

// ============================================================================
// SCENARIO 2: Fieldname Override Test
// ============================================================================

SCENARIO("flx_model fieldname metadata override", "[xml_model_integration]") {
    GIVEN("Model with fieldname override") {
        flx_string xml_str = R"(
            <data>
                <xml_name>Test Value</xml_name>
                <normal>123</normal>
            </data>
        )";

        WHEN("Applying XML with read_xml()") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            FieldNameTest model;
            model.read_xml(xml, "data");

            THEN("Property should be accessible via C++ name") {
                REQUIRE(model.cpp_name.value() == "Test Value");
                REQUIRE(model.normal_field.value() == 123);
            }

            THEN("Data should be stored under fieldname in internal map") {
                // Access internal map to verify storage name
                flxv_map& internal = *model;
                REQUIRE(internal.find("storage_name") != internal.end());
                REQUIRE(internal["storage_name"].string_value() == "Test Value");
                REQUIRE(internal.find("normal_field") != internal.end());
            }
        }
    }
}

// ============================================================================
// SCENARIO 3: Nested Path Test (level1/level2/value)
// ============================================================================

SCENARIO("flx_model nested path in xml_path", "[xml_model_integration]") {
    GIVEN("XML with nested structure and model using path notation") {
        flx_string xml_str = R"(
            <root>
                <root>Root Value</root>
                <level1>
                    <number>999</number>
                    <level2>
                        <value>Deep Value</value>
                    </level2>
                </level1>
            </root>
        )";

        WHEN("Applying XML with nested paths") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            NestedPathData model;
            model.read_xml(xml, "root");

            THEN("Nested path values should be extracted") {
                REQUIRE(model.deep_value.value() == "Deep Value");
                REQUIRE(model.deep_number.value() == 999);
                REQUIRE(model.root_value.value() == "Root Value");
            }
        }
    }
}

// ============================================================================
// SCENARIO 4: Nested Models Test
// ============================================================================

SCENARIO("flx_model nested model mapping", "[xml_model_integration]") {
    GIVEN("XML with nested person structure") {
        flx_string xml_str = R"(
            <person>
                <fullname>John Doe</fullname>
                <age>35</age>
                <address>
                    <street>Main Street 123</street>
                    <city>New York</city>
                    <zipcode>10001</zipcode>
                </address>
                <contact>
                    <email_address>john@example.com</email_address>
                    <phone>555-1234</phone>
                </contact>
            </person>
        )";

        WHEN("Applying to nested model") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Person person;
            person.read_xml(xml, "person");

            THEN("Parent properties should be mapped") {
                REQUIRE(person.name.value() == "John Doe");
                REQUIRE(person.age.value() == 35);
            }

            THEN("Nested address model should be populated") {
                REQUIRE(person.address.street.value() == "Main Street 123");
                REQUIRE(person.address.city.value() == "New York");
                REQUIRE(person.address.zip.value() == 10001);  // Note: xml "zipcode" -> cpp "zip"
            }

            THEN("Nested contact with fieldname override should work") {
                REQUIRE(person.contact.email.value() == "john@example.com");
                REQUIRE(person.contact.phone.value() == "555-1234");

                // Verify fieldname override in storage
                flxv_map& contact_map = *person.contact;
                REQUIRE(contact_map.find("contact_email") != contact_map.end());
            }
        }
    }
}

// ============================================================================
// SCENARIO 5: Model List with Multiple Elements
// ============================================================================

SCENARIO("flx_model_list with multiple XML elements", "[xml_model_integration]") {
    GIVEN("XML with multiple member elements") {
        flx_string xml_str = R"(
            <team>
                <teamname>Development</teamname>
                <member>
                    <fullname>Alice</fullname>
                    <age>30</age>
                    <address>
                        <street>Oak Ave 1</street>
                        <city>Boston</city>
                        <zipcode>02101</zipcode>
                    </address>
                    <contact>
                        <email_address>alice@example.com</email_address>
                        <phone>555-0001</phone>
                    </contact>
                </member>
                <member>
                    <fullname>Bob</fullname>
                    <age>28</age>
                    <address>
                        <street>Pine Rd 2</street>
                        <city>Seattle</city>
                        <zipcode>98101</zipcode>
                    </address>
                    <contact>
                        <email_address>bob@example.com</email_address>
                        <phone>555-0002</phone>
                    </contact>
                </member>
                <member>
                    <fullname>Charlie</fullname>
                    <age>32</age>
                    <address>
                        <street>Elm St 3</street>
                        <city>Austin</city>
                        <zipcode>73301</zipcode>
                    </address>
                    <contact>
                        <email_address>charlie@example.com</email_address>
                        <phone>555-0003</phone>
                    </contact>
                </member>
            </team>
        )";

        WHEN("Applying to model with model_list") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Team team;
            team.read_xml(xml, "team");

            THEN("List should contain all members") {
                REQUIRE(team.name.value() == "Development");
                REQUIRE(team.members.size() == 3);
            }

            THEN("First member should be complete") {
                auto& alice = team.members[0];
                REQUIRE(alice.name.value() == "Alice");
                REQUIRE(alice.age.value() == 30);
                REQUIRE(alice.address.street.value() == "Oak Ave 1");
                REQUIRE(alice.address.city.value() == "Boston");
                REQUIRE(alice.contact.email.value() == "alice@example.com");
            }

            THEN("All members should be distinct") {
                REQUIRE(team.members[0].name.value() == "Alice");
                REQUIRE(team.members[1].name.value() == "Bob");
                REQUIRE(team.members[2].name.value() == "Charlie");

                REQUIRE(team.members[0].address.city.value() == "Boston");
                REQUIRE(team.members[1].address.city.value() == "Seattle");
                REQUIRE(team.members[2].address.city.value() == "Austin");
            }
        }
    }
}

// ============================================================================
// SCENARIO 6: Single Element Auto-Conversion to Array
// ============================================================================

SCENARIO("flx_model_list with single XML element (auto-array conversion)", "[xml_model_integration]") {
    GIVEN("XML with only ONE member element") {
        flx_string xml_str = R"(
            <team>
                <teamname>Solo Team</teamname>
                <member>
                    <fullname>Alice</fullname>
                    <age>30</age>
                    <address>
                        <street>Main St 1</street>
                        <city>Denver</city>
                        <zipcode>80201</zipcode>
                    </address>
                    <contact>
                        <email_address>alice@solo.com</email_address>
                        <phone>555-9999</phone>
                    </contact>
                </member>
            </team>
        )";

        WHEN("Applying to model_list") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Team team;
            team.read_xml(xml, "team");

            THEN("Single element should be converted to array with size 1") {
                REQUIRE(team.members.size() == 1);
                REQUIRE(team.members[0].name.value() == "Alice");
                REQUIRE(team.members[0].age.value() == 30);
            }
        }
    }
}

// ============================================================================
// SCENARIO 7: Multiple Model Lists in Same Model
// ============================================================================

SCENARIO("flx_model with multiple model_lists", "[xml_model_integration]") {
    GIVEN("XML with both members and tasks lists") {
        flx_string xml_str = R"(
            <team>
                <teamname>Full Team</teamname>
                <member>
                    <fullname>Alice</fullname>
                    <age>30</age>
                    <address><street>St 1</street><city>NYC</city><zipcode>10001</zipcode></address>
                    <contact><email_address>a@t.com</email_address><phone>555-1</phone></contact>
                </member>
                <member>
                    <fullname>Bob</fullname>
                    <age>28</age>
                    <address><street>St 2</street><city>LA</city><zipcode>90001</zipcode></address>
                    <contact><email_address>b@t.com</email_address><phone>555-2</phone></contact>
                </member>
                <task>
                    <title>Task One</title>
                    <done>true</done>
                    <priority>1</priority>
                </task>
                <task>
                    <title>Task Two</title>
                    <done>false</done>
                    <priority>2</priority>
                </task>
                <task>
                    <title>Task Three</title>
                    <done>false</done>
                    <priority>3</priority>
                </task>
            </team>
        )";

        WHEN("Applying to model with two lists") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Team team;
            team.read_xml(xml, "team");

            THEN("Both lists should be populated independently") {
                REQUIRE(team.members.size() == 2);
                REQUIRE(team.tasks.size() == 3);
            }

            THEN("Members list should be correct") {
                REQUIRE(team.members[0].name.value() == "Alice");
                REQUIRE(team.members[1].name.value() == "Bob");
            }

            THEN("Tasks list should be correct") {
                REQUIRE(team.tasks[0].title.value() == "Task One");
                REQUIRE(team.tasks[0].completed.value() == true);
                REQUIRE(team.tasks[0].priority.value() == 1);

                REQUIRE(team.tasks[1].title.value() == "Task Two");
                REQUIRE(team.tasks[1].completed.value() == false);
                REQUIRE(team.tasks[1].priority.value() == 2);

                REQUIRE(team.tasks[2].title.value() == "Task Three");
                REQUIRE(team.tasks[2].priority.value() == 3);
            }
        }
    }
}

// ============================================================================
// SCENARIO 8: DEEP NESTING - The Ultimate Test
// ============================================================================

SCENARIO("flx_model deeply nested structure with all features combined", "[xml_model_integration]") {
    GIVEN("Extremely complex nested XML with all features") {
        flx_string xml_str = R"(
            <organization>
                <company_name>TechCorp Inc</company_name>
                <metadata>
                    <founded>1995</founded>
                </metadata>
                <department>
                    <name>Engineering</name>
                    <financials>
                        <budget>5000000</budget>
                        <currency>USD</currency>
                    </financials>
                    <meta>
                        <manager>
                            <name>Sarah Johnson</name>
                        </manager>
                    </meta>
                    <team>
                        <teamname>Backend Team</teamname>
                        <member>
                            <fullname>Alice Anderson</fullname>
                            <age>30</age>
                            <address>
                                <street>Tech Plaza 100</street>
                                <city>San Francisco</city>
                                <zipcode>94105</zipcode>
                            </address>
                            <contact>
                                <email_address>alice@techcorp.com</email_address>
                                <phone>555-0100</phone>
                            </contact>
                        </member>
                        <member>
                            <fullname>Bob Brown</fullname>
                            <age>28</age>
                            <address>
                                <street>Innovation Ave 200</street>
                                <city>San Francisco</city>
                                <zipcode>94106</zipcode>
                            </address>
                            <contact>
                                <email_address>bob@techcorp.com</email_address>
                                <phone>555-0101</phone>
                            </contact>
                        </member>
                        <task>
                            <title>API Development</title>
                            <done>false</done>
                            <priority>1</priority>
                        </task>
                        <task>
                            <title>Database Migration</title>
                            <done>true</done>
                            <priority>2</priority>
                        </task>
                    </team>
                    <team>
                        <teamname>Frontend Team</teamname>
                        <member>
                            <fullname>Charlie Chen</fullname>
                            <age>32</age>
                            <address>
                                <street>UI Street 300</street>
                                <city>San Francisco</city>
                                <zipcode>94107</zipcode>
                            </address>
                            <contact>
                                <email_address>charlie@techcorp.com</email_address>
                                <phone>555-0102</phone>
                            </contact>
                        </member>
                        <task>
                            <title>Component Library</title>
                            <done>false</done>
                            <priority>1</priority>
                        </task>
                    </team>
                </department>
                <department>
                    <name>Sales</name>
                    <financials>
                        <budget>2000000</budget>
                        <currency>USD</currency>
                    </financials>
                    <meta>
                        <manager>
                            <name>David Davis</name>
                        </manager>
                    </meta>
                    <team>
                        <teamname>Enterprise Sales</teamname>
                        <member>
                            <fullname>Eve Evans</fullname>
                            <age>35</age>
                            <address>
                                <street>Sales Plaza 400</street>
                                <city>New York</city>
                                <zipcode>10001</zipcode>
                            </address>
                            <contact>
                                <email_address>eve@techcorp.com</email_address>
                                <phone>555-0200</phone>
                            </contact>
                        </member>
                        <task>
                            <title>Q1 Targets</title>
                            <done>true</done>
                            <priority>1</priority>
                        </task>
                    </team>
                </department>
            </organization>
        )";

        WHEN("Applying deeply nested structure") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Organization org;
            org.read_xml(xml, "organization");

            THEN("Root level properties should work") {
                REQUIRE(org.company.value() == "TechCorp Inc");
                REQUIRE(org.founded.value() == 1995);
            }

            THEN("Organization should have 2 departments") {
                REQUIRE(org.departments.size() == 2);
            }

            THEN("Engineering department (first) should be complete") {
                auto& eng = org.departments[0];

                // Test fieldname override at department level
                REQUIRE(eng.dept_name.value() == "Engineering");

                // Verify internal storage uses overridden name
                flxv_map& eng_map = *eng;
                REQUIRE(eng_map.find("department_name") != eng_map.end());

                // Test nested path
                REQUIRE(eng.budget.value() == 5000000);
                REQUIRE(eng.currency.value() == "USD");

                // Test 3-level nested path
                REQUIRE(eng.manager.value() == "Sarah Johnson");

                // Test teams list
                REQUIRE(eng.teams.size() == 2);
            }

            THEN("Backend team should be fully populated") {
                auto& backend = org.departments[0].teams[0];
                REQUIRE(backend.name.value() == "Backend Team");
                REQUIRE(backend.members.size() == 2);
                REQUIRE(backend.tasks.size() == 2);

                // First member
                auto& alice = backend.members[0];
                REQUIRE(alice.name.value() == "Alice Anderson");
                REQUIRE(alice.age.value() == 30);
                REQUIRE(alice.address.street.value() == "Tech Plaza 100");
                REQUIRE(alice.address.city.value() == "San Francisco");
                REQUIRE(alice.address.zip.value() == 94105);
                REQUIRE(alice.contact.email.value() == "alice@techcorp.com");
                REQUIRE(alice.contact.phone.value() == "555-0100");

                // Second member
                auto& bob = backend.members[1];
                REQUIRE(bob.name.value() == "Bob Brown");
                REQUIRE(bob.age.value() == 28);

                // Tasks
                REQUIRE(backend.tasks[0].title.value() == "API Development");
                REQUIRE(backend.tasks[0].completed.value() == false);
                REQUIRE(backend.tasks[1].title.value() == "Database Migration");
                REQUIRE(backend.tasks[1].completed.value() == true);
            }

            THEN("Frontend team should have correct data") {
                auto& frontend = org.departments[0].teams[1];
                REQUIRE(frontend.name.value() == "Frontend Team");
                REQUIRE(frontend.members.size() == 1);
                REQUIRE(frontend.tasks.size() == 1);

                auto& charlie = frontend.members[0];
                REQUIRE(charlie.name.value() == "Charlie Chen");
                REQUIRE(charlie.age.value() == 32);
                REQUIRE(charlie.address.city.value() == "San Francisco");
            }

            THEN("Sales department (second) should be complete") {
                auto& sales = org.departments[1];
                REQUIRE(sales.dept_name.value() == "Sales");
                REQUIRE(sales.budget.value() == 2000000);
                REQUIRE(sales.manager.value() == "David Davis");
                REQUIRE(sales.teams.size() == 1);

                auto& sales_team = sales.teams[0];
                REQUIRE(sales_team.name.value() == "Enterprise Sales");
                REQUIRE(sales_team.members.size() == 1);
                REQUIRE(sales_team.members[0].name.value() == "Eve Evans");
                REQUIRE(sales_team.members[0].address.city.value() == "New York");
            }

            THEN("Total counts should be correct across entire structure") {
                // 2 departments
                REQUIRE(org.departments.size() == 2);

                // Engineering: 2 teams, Sales: 1 team = 3 total
                size_t total_teams = 0;
                for (size_t i = 0; i < org.departments.size(); i++) {
                    total_teams += org.departments[i].teams.size();
                }
                REQUIRE(total_teams == 3);

                // Backend: 2 members, Frontend: 1, Sales: 1 = 4 total
                size_t total_members = 0;
                for (size_t i = 0; i < org.departments.size(); i++) {
                    for (size_t j = 0; j < org.departments[i].teams.size(); j++) {
                        total_members += org.departments[i].teams[j].members.size();
                    }
                }
                REQUIRE(total_members == 4);

                // Backend: 2 tasks, Frontend: 1, Sales: 1 = 4 total
                size_t total_tasks = 0;
                for (size_t i = 0; i < org.departments.size(); i++) {
                    for (size_t j = 0; j < org.departments[i].teams.size(); j++) {
                        total_tasks += org.departments[i].teams[j].tasks.size();
                    }
                }
                REQUIRE(total_tasks == 4);
            }
        }
    }
}

// ============================================================================
// SCENARIO 9: XML Attributes in xml_path - Simple Mapping
// ============================================================================

SCENARIO("flx_model mapping XML attributes via xml_path", "[xml_model_integration]") {
    GIVEN("XML element with multiple attributes") {
        flx_string xml_str = R"(
            <product id="42" category="Electronics" active="true">
                Laptop Pro 15
            </product>
        )";

        WHEN("Mapping attributes to model properties") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            ProductWithAttrs product;
            product.read_xml(xml, "product");

            THEN("Attributes should be accessible via @-prefixed paths") {
                REQUIRE(product.product_id.value() == 42);
                REQUIRE(product.category.value() == "Electronics");
            }

            THEN("Text content should be accessible via #text path") {
                REQUIRE(product.name.value() == "Laptop Pro 15");
            }
        }
    }
}

// ============================================================================
// SCENARIO 10: Mixed Content - Attributes + Text + Child Elements
// ============================================================================

SCENARIO("flx_model with mixed content (attributes + text + children)", "[xml_model_integration]") {
    GIVEN("XML with attributes, text content, and child elements") {
        flx_string xml_str = R"(
            <node type="container" id="123">
                Some text content here
                <child>Child Value</child>
            </node>
        )";

        WHEN("Parsing and mapping mixed content") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            MixedContentNode node;
            node.read_xml(xml, "node");

            THEN("All content types should be accessible") {
                REQUIRE(node.node_type.value() == "container");
                REQUIRE(node.node_id.value() == 123);
                REQUIRE(node.text_content.value() == "Some text content here");
                REQUIRE(node.child_value.value() == "Child Value");
            }
        }
    }
}

// ============================================================================
// SCENARIO 11: Nested Paths to Attributes - The Hard Test
// ============================================================================

SCENARIO("flx_model nested paths to attributes and text content", "[xml_model_integration]") {
    GIVEN("XML with deeply nested attributes and text") {
        flx_string xml_str = R"(
            <root>
                <config>
                    <database host="localhost" port="5432">
                        postgres://user:pass@localhost:5432/db
                    </database>
                    <app>
                        <name>MyApp</name>
                    </app>
                </config>
            </root>
        )";

        WHEN("Using nested paths with @ and #text") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            DeepAttributeAccess model;
            model.read_xml(xml, "root");

            THEN("Nested path to attribute should work (config/database/@host)") {
                REQUIRE(model.db_host.value() == "localhost");
            }

            THEN("Nested path to attribute with int conversion (config/database/@port)") {
                REQUIRE(model.db_port.value() == 5432);
            }

            THEN("Nested path to text content (config/database/#text)") {
                REQUIRE(model.db_connection.value() == "postgres://user:pass@localhost:5432/db");
            }

            THEN("Regular nested element should still work") {
                REQUIRE(model.app_name.value() == "MyApp");
            }
        }
    }
}

// ============================================================================
// SCENARIO 12: Model List with Attributes on Each Element
// ============================================================================

SCENARIO("flx_model_list where each element has attributes", "[xml_model_integration]") {
    GIVEN("XML array where each element has attributes AND text") {
        flx_string xml_str = R"(
            <collection name="MyTags">
                <item id="1" type="urgent">Critical Bug</item>
                <item id="2" type="feature">New Dashboard</item>
                <item id="3" type="task">Code Review</item>
            </collection>
        )";

        WHEN("Mapping to model_list with attribute access") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            TaggedCollection collection;
            collection.read_xml(xml, "collection");

            THEN("Collection attribute should be mapped") {
                REQUIRE(collection.collection_name.value() == "MyTags");
            }

            THEN("All items should be in the list") {
                REQUIRE(collection.items.size() == 3);
            }

            THEN("First item attributes and text should be mapped") {
                auto& item1 = collection.items[0];
                REQUIRE(item1.item_id.value() == 1);
                REQUIRE(item1.item_type.value() == "urgent");
                REQUIRE(item1.label.value() == "Critical Bug");
            }

            THEN("All items should have distinct values") {
                REQUIRE(collection.items[0].item_id.value() == 1);
                REQUIRE(collection.items[1].item_id.value() == 2);
                REQUIRE(collection.items[2].item_id.value() == 3);

                REQUIRE(collection.items[0].item_type.value() == "urgent");
                REQUIRE(collection.items[1].item_type.value() == "feature");
                REQUIRE(collection.items[2].item_type.value() == "task");

                REQUIRE(collection.items[0].label.value() == "Critical Bug");
                REQUIRE(collection.items[1].label.value() == "New Dashboard");
                REQUIRE(collection.items[2].label.value() == "Code Review");
            }
        }
    }
}

// ============================================================================
// SCENARIO 13: Single Element List with Attributes
// ============================================================================

SCENARIO("flx_model_list single element with attributes (auto-array)", "[xml_model_integration]") {
    GIVEN("XML with single item having attributes") {
        flx_string xml_str = R"(
            <collection name="SingleItem">
                <item id="999" type="special">Only One</item>
            </collection>
        )";

        WHEN("Auto-converting single element to array") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            TaggedCollection collection;
            collection.read_xml(xml, "collection");

            THEN("Single item should be in array with size 1") {
                REQUIRE(collection.items.size() == 1);
            }

            THEN("Item attributes and text should be preserved") {
                REQUIRE(collection.items[0].item_id.value() == 999);
                REQUIRE(collection.items[0].item_type.value() == "special");
                REQUIRE(collection.items[0].label.value() == "Only One");
            }
        }
    }
}

// ============================================================================
// SCENARIO 14: Fieldname Override with Attributes and Text
// ============================================================================

SCENARIO("flx_model fieldname override combined with @ and # paths", "[xml_model_integration]") {
    GIVEN("XML element with attributes and text") {
        flx_string xml_str = R"(
            <data xml_id="777">
                Text Content Here
                <xml_name>Element Name</xml_name>
            </data>
        )";

        WHEN("Using fieldname override with special paths") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            AdvancedFieldMapping model;
            model.read_xml(xml, "data");

            THEN("Attribute with fieldname override should work") {
                REQUIRE(model.cpp_id.value() == 777);

                // Check internal storage uses overridden name
                flxv_map& internal = *model;
                REQUIRE(internal.find("stored_id") != internal.end());
                REQUIRE(internal["stored_id"].int_value() == 777);
            }

            THEN("Text content with fieldname override should work") {
                REQUIRE(model.cpp_text.value() == "Text Content Here");

                // Check internal storage
                flxv_map& internal = *model;
                REQUIRE(internal.find("stored_text") != internal.end());
                REQUIRE(internal["stored_text"].string_value() == "Text Content Here");
            }

            THEN("Regular element with fieldname override should work") {
                REQUIRE(model.cpp_name.value() == "Element Name");

                // Check internal storage
                flxv_map& internal = *model;
                REQUIRE(internal.find("stored_name") != internal.end());
                REQUIRE(internal["stored_name"].string_value() == "Element Name");
            }
        }
    }
}

// ============================================================================
// SCENARIO 15: Edge Cases - Empty and Missing Data
// ============================================================================

SCENARIO("flx_model edge cases with missing/empty data", "[xml_model_integration]") {
    GIVEN("XML with some missing optional fields") {
        flx_string xml_str = R"(
            <person>
                <fullname>Minimal Person</fullname>
                <age>25</age>
                <address>
                    <street>Street Only</street>
                    <city></city>
                    <zipcode>12345</zipcode>
                </address>
            </person>
        )";

        WHEN("Applying to model with optional nested fields") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            Person person;
            person.read_xml(xml, "person");

            THEN("Present fields should be populated") {
                REQUIRE(person.name.value() == "Minimal Person");
                REQUIRE(person.age.value() == 25);
                REQUIRE(person.address.street.value() == "Street Only");
                REQUIRE(person.address.zip.value() == 12345);
            }

            THEN("Missing nested model should not crash") {
                // contact model was not in XML, should be null
                REQUIRE(person.contact.email.is_null() == true);
                REQUIRE(person.contact.phone.is_null() == true);
            }

            THEN("Empty string should be treated as empty") {
                // city is empty in XML
                REQUIRE(person.address.city.value() == "");
            }
        }
    }
}

// ============================================================================
// SCENARIO 16: Nested Attribute Path with Missing Intermediate Nodes
// ============================================================================

SCENARIO("flx_model nested attribute paths with missing nodes", "[xml_model_integration]") {
    GIVEN("XML missing some intermediate nodes") {
        flx_string xml_str = R"(
            <root>
                <config>
                    <app>
                        <name>TestApp</name>
                    </app>
                </config>
            </root>
        )";

        WHEN("Accessing nested paths to missing nodes") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            DeepAttributeAccess model;
            model.read_xml(xml, "root");

            THEN("Present nested path should work") {
                REQUIRE(model.app_name.value() == "TestApp");
            }

            THEN("Missing nested attribute paths should result in null properties") {
                // config/database/@host doesn't exist (database node missing)
                REQUIRE(model.db_host.is_null() == true);
                REQUIRE(model.db_port.is_null() == true);
                REQUIRE(model.db_connection.is_null() == true);
            }
        }
    }
}

// ============================================================================
// SCENARIO 17: Namespace Handling in Models
// ============================================================================

SCENARIO("flx_model with namespaced XML", "[xml_model_integration]") {
    class NamespacedPerson : public flx_model {
    public:
        flxp_string(name, {{"xml_path", "fullname"}});
        flxp_int(age, {{"xml_path", "age"}});
        flxp_string(email, {{"xml_path", "@email"}});
    };

    class NamespacedTeam : public flx_model {
    public:
        flxp_string(name, {{"xml_path", "teamname"}});
        flxp_model_list(members, NamespacedPerson, {{"xml_path", "member[]"}});
    };

    GIVEN("XML with namespaces on all elements") {
        flx_string xml_str = R"(
            <ns:team xmlns:ns="http://company.com/ns">
                <ns:teamname>Dev Team</ns:teamname>
                <ns:member ns:email="alice@test.com">
                    <ns:fullname>Alice Smith</ns:fullname>
                    <ns:age>30</ns:age>
                </ns:member>
                <ns:member ns:email="bob@test.com">
                    <ns:fullname>Bob Jones</ns:fullname>
                    <ns:age>28</ns:age>
                </ns:member>
            </ns:team>
        )";

        WHEN("Reading model from namespaced XML") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            NamespacedTeam team;
            team.read_xml(xml, "team");

            THEN("Model should read correctly without namespace prefixes in paths") {
                REQUIRE(team.name.value() == "Dev Team");
                REQUIRE(team.members.size() == 2);

                REQUIRE(team.members[0].name.value() == "Alice Smith");
                REQUIRE(team.members[0].age.value() == 30);
                REQUIRE(team.members[0].email.value() == "alice@test.com");

                REQUIRE(team.members[1].name.value() == "Bob Jones");
                REQUIRE(team.members[1].age.value() == 28);
                REQUIRE(team.members[1].email.value() == "bob@test.com");
            }
        }
    }

    GIVEN("XML with mixed namespaces") {
        flx_string xml_str = R"(
            <data:team xmlns:data="http://data.com" xmlns:meta="http://meta.com">
                <data:teamname>Mixed NS Team</data:teamname>
                <data:member meta:email="test@example.com">
                    <data:fullname>Test User</data:fullname>
                    <data:age>35</data:age>
                </data:member>
            </data:team>
        )";

        WHEN("Reading model with mixed namespace prefixes") {
            flxv_map data;
            flx_xml xml(&data);
            REQUIRE(xml.parse(xml_str) == true);

            NamespacedTeam team;
            team.read_xml(xml, "team");

            THEN("All namespaces should be transparently stripped") {
                REQUIRE(team.name.value() == "Mixed NS Team");
                REQUIRE(team.members.size() == 1);
                REQUIRE(team.members[0].name.value() == "Test User");
                REQUIRE(team.members[0].age.value() == 35);
                REQUIRE(team.members[0].email.value() == "test@example.com");
            }
        }
    }
}
