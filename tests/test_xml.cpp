#include <catch2/catch_all.hpp>
#include <api/xml/flx_xml.h>

SCENARIO("flx_xml basic parsing and creation") {
  GIVEN("An empty flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing simple XML with text content") {
      flx_string xml_str = "<root><name>Test</name><age>30</age></root>";
      bool parsed = xml.parse(xml_str);

      THEN("Parsing should succeed") {
        REQUIRE(parsed == true);
        REQUIRE(test_map.find("root") != test_map.end());

        flxv_map root = test_map["root"].to_map();
        REQUIRE(root.find("name") != root.end());
        REQUIRE(root["name"].string_value() == "Test");
        REQUIRE(root.find("age") != root.end());
        REQUIRE(root["age"].int_value() == 30);
      }
    }

    WHEN("Creating XML from flxv_map") {
      flxv_map root_data;
      root_data[flx_string("name")] = flx_variant(flx_string("Alice"));
      root_data[flx_string("age")] = flx_variant(25LL);

      test_map[flx_string("person")] = flx_variant(root_data);

      flx_string xml_output = xml.create();

      THEN("XML output should be valid") {
        REQUIRE(xml_output.empty() == false);
        REQUIRE(xml_output.contains("<person>") == true);
        REQUIRE(xml_output.contains("<name>Alice</name>") == true);
        REQUIRE(xml_output.contains("<age>25</age>") == true);
        REQUIRE(xml_output.contains("</person>") == true);
      }
    }
  }
}

SCENARIO("flx_xml attribute handling") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing XML with attributes") {
      flx_string xml_str = R"(<root id="123" active="true"><name>Test</name></root>)";
      bool parsed = xml.parse(xml_str);

      THEN("Attributes should be parsed with @ prefix") {
        REQUIRE(parsed == true);
        flxv_map& root = const_cast<flxv_map&>(test_map["root"].map_value());

        REQUIRE(root.find("@id") != root.end());
        REQUIRE(root["@id"].int_value() == 123);

        REQUIRE(root.find("@active") != root.end());
        REQUIRE(root["@active"].bool_value() == true);

        REQUIRE(root.find("name") != root.end());
        REQUIRE(root["name"].string_value() == "Test");
      }
    }

    WHEN("Creating XML with attributes") {
      flxv_map root_data;
      root_data[flx_string("@id")] = flx_variant(456LL);
      root_data[flx_string("@active")] = flx_variant(false);
      root_data[flx_string("name")] = flx_variant(flx_string("Bob"));

      test_map[flx_string("item")] = flx_variant(root_data);

      flx_string xml_output = xml.create();

      THEN("XML should contain attributes") {
        REQUIRE(xml_output.contains("id=\"456\"") == true);
        REQUIRE(xml_output.contains("active=\"false\"") == true);
        REQUIRE(xml_output.contains("<name>Bob</name>") == true);
      }
    }
  }
}

SCENARIO("flx_xml text content handling") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing XML with mixed content") {
      flx_string xml_str = R"(<item id="1">Some text content</item>)";
      bool parsed = xml.parse(xml_str);

      THEN("Both attribute and text should be parsed") {
        REQUIRE(parsed == true);
        flxv_map item = test_map["item"].to_map();

        REQUIRE(item.find("@id") != item.end());
        REQUIRE(item["@id"].int_value() == 1);

        REQUIRE(item.find("#text") != item.end());
        REQUIRE(item["#text"].string_value() == "Some text content");
      }
    }

    WHEN("Creating XML with text content and attributes") {
      flxv_map item_data;
      item_data[flx_string("@id")] = flx_variant(789LL);
      item_data[flx_string("#text")] = flx_variant(flx_string("Content here"));

      test_map[flx_string("element")] = flx_variant(item_data);

      flx_string xml_output = xml.create();

      THEN("XML should contain both attribute and text") {
        REQUIRE(xml_output.contains("id=\"789\"") == true);
        REQUIRE(xml_output.contains("Content here") == true);
      }
    }
  }
}

SCENARIO("flx_xml array handling") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing XML with multiple elements of same name") {
      flx_string xml_str = R"(
        <root>
          <item>First</item>
          <item>Second</item>
          <item>Third</item>
        </root>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Elements should be converted to array") {
        REQUIRE(parsed == true);
        flxv_map root = test_map["root"].to_map();

        REQUIRE(root.find("item") != root.end());
        REQUIRE(root["item"].in_state() == flx_variant::vector_state);

        flxv_vector items = root["item"].to_vector();
        REQUIRE(items.size() == 3);
        REQUIRE(items[0].string_value() == "First");
        REQUIRE(items[1].string_value() == "Second");
        REQUIRE(items[2].string_value() == "Third");
      }
    }

    WHEN("Creating XML from array") {
      flxv_vector items;
      items.push_back(flx_variant(flx_string("Apple")));
      items.push_back(flx_variant(flx_string("Banana")));
      items.push_back(flx_variant(flx_string("Cherry")));

      flxv_map root_data;
      root_data[flx_string("fruit")] = flx_variant(items);

      test_map[flx_string("fruits")] = flx_variant(root_data);

      flx_string xml_output = xml.create();

      THEN("XML should contain multiple elements") {
        REQUIRE(xml_output.contains("<fruit>Apple</fruit>") == true);
        REQUIRE(xml_output.contains("<fruit>Banana</fruit>") == true);
        REQUIRE(xml_output.contains("<fruit>Cherry</fruit>") == true);
      }
    }
  }
}

SCENARIO("flx_xml type detection") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing XML with different data types") {
      flx_string xml_str = R"(
        <data>
          <string_val>Hello</string_val>
          <int_val>42</int_val>
          <double_val>3.14</double_val>
          <bool_true>true</bool_true>
          <bool_false>false</bool_false>
        </data>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Types should be detected correctly") {
        REQUIRE(parsed == true);
        flxv_map data = test_map["data"].to_map();

        REQUIRE(data["string_val"].in_state() == flx_variant::string_state);
        REQUIRE(data["string_val"].string_value() == "Hello");

        REQUIRE(data["int_val"].in_state() == flx_variant::int_state);
        REQUIRE(data["int_val"].int_value() == 42);

        REQUIRE(data["double_val"].in_state() == flx_variant::double_state);
        REQUIRE(data["double_val"].double_value() >= 3.13);
        REQUIRE(data["double_val"].double_value() <= 3.15);

        REQUIRE(data["bool_true"].in_state() == flx_variant::bool_state);
        REQUIRE(data["bool_true"].bool_value() == true);

        REQUIRE(data["bool_false"].in_state() == flx_variant::bool_state);
        REQUIRE(data["bool_false"].bool_value() == false);
      }
    }
  }
}

SCENARIO("flx_xml round-trip conversion") {
  GIVEN("An flxv_map with complex data") {
    flxv_map test_map;

    // Build complex structure
    flxv_map user_data;
    user_data[flx_string("@id")] = flx_variant(999LL);
    user_data[flx_string("name")] = flx_variant(flx_string("John Doe"));
    user_data[flx_string("age")] = flx_variant(35LL);
    user_data[flx_string("score")] = flx_variant(98.5);
    user_data[flx_string("active")] = flx_variant(true);

    test_map[flx_string("user")] = flx_variant(user_data);

    flx_xml xml(&test_map);

    WHEN("Converting to XML and back") {
      flx_string xml_str = xml.create();

      // Parse back into new map
      flxv_map result_map;
      flx_xml xml2(&result_map);
      bool parsed = xml2.parse(xml_str);

      THEN("Data should be preserved") {
        REQUIRE(parsed == true);
        REQUIRE(result_map.find("user") != result_map.end());

        flxv_map user = result_map["user"].to_map();
        REQUIRE(user["@id"].int_value() == 999);
        REQUIRE(user["name"].string_value() == "John Doe");
        REQUIRE(user["age"].int_value() == 35);
        REQUIRE(user["score"].double_value() >= 98.4);
        REQUIRE(user["score"].double_value() <= 98.6);
        REQUIRE(user["active"].bool_value() == true);
      }
    }
  }
}

SCENARIO("flx_xml nested structures") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing nested XML") {
      flx_string xml_str = R"(
        <company>
          <name>Tech Corp</name>
          <employee>
            <name>Alice</name>
            <role>Engineer</role>
          </employee>
          <employee>
            <name>Bob</name>
            <role>Manager</role>
          </employee>
        </company>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Nested structure should be preserved") {
        REQUIRE(parsed == true);
        flxv_map company = test_map["company"].to_map();

        REQUIRE(company["name"].string_value() == "Tech Corp");
        REQUIRE(company["employee"].in_state() == flx_variant::vector_state);

        flxv_vector employees = company["employee"].to_vector();
        REQUIRE(employees.size() == 2);

        flxv_map emp1 = employees[0].to_map();
        REQUIRE(emp1["name"].string_value() == "Alice");
        REQUIRE(emp1["role"].string_value() == "Engineer");

        flxv_map emp2 = employees[1].to_map();
        REQUIRE(emp2["name"].string_value() == "Bob");
        REQUIRE(emp2["role"].string_value() == "Manager");
      }
    }
  }
}
