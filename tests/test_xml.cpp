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
        flxv_map name = root["name"].to_map();
        REQUIRE(name["#text"].string_value() == "Test");
        REQUIRE(root.find("age") != root.end());
        flxv_map age = root["age"].to_map();
        REQUIRE(age["#text"].string_value().to_int(0) == 30);
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
        REQUIRE(root["@id"].string_value().to_int(0) == 123);

        REQUIRE(root.find("@active") != root.end());
        REQUIRE(root["@active"].string_value() == "true");

        REQUIRE(root.find("name") != root.end());
        flxv_map name = root["name"].to_map();
        REQUIRE(name["#text"].string_value() == "Test");
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
        REQUIRE(item["@id"].string_value().to_int(0) == 1);

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
        flxv_map item0 = items[0].to_map();
        REQUIRE(item0["#text"].string_value() == "First");
        flxv_map item1 = items[1].to_map();
        REQUIRE(item1["#text"].string_value() == "Second");
        flxv_map item2 = items[2].to_map();
        REQUIRE(item2["#text"].string_value() == "Third");
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

SCENARIO("flx_xml string-based parsing") {
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

      THEN("All values are parsed as strings") {
        REQUIRE(parsed == true);
        flxv_map data = test_map["data"].to_map();

        flxv_map string_val = data["string_val"].to_map();
        REQUIRE(string_val["#text"].in_state() == flx_variant::string_state);
        REQUIRE(string_val["#text"].string_value() == "Hello");

        flxv_map int_val = data["int_val"].to_map();
        REQUIRE(int_val["#text"].in_state() == flx_variant::string_state);
        REQUIRE(int_val["#text"].string_value().to_int(0) == 42);

        flxv_map double_val = data["double_val"].to_map();
        REQUIRE(double_val["#text"].in_state() == flx_variant::string_state);
        double parsed_double = double_val["#text"].string_value().to_double(0.0);
        REQUIRE(parsed_double >= 3.13);
        REQUIRE(parsed_double <= 3.15);

        flxv_map bool_true = data["bool_true"].to_map();
        REQUIRE(bool_true["#text"].in_state() == flx_variant::string_state);
        REQUIRE(bool_true["#text"].string_value() == "true");

        flxv_map bool_false = data["bool_false"].to_map();
        REQUIRE(bool_false["#text"].in_state() == flx_variant::string_state);
        REQUIRE(bool_false["#text"].string_value() == "false");
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
        REQUIRE(user["@id"].string_value().to_int(0) == 999);

        flxv_map name = user["name"].to_map();
        REQUIRE(name["#text"].string_value() == "John Doe");

        flxv_map age = user["age"].to_map();
        REQUIRE(age["#text"].string_value().to_int(0) == 35);

        flxv_map score = user["score"].to_map();
        double parsed_score = score["#text"].string_value().to_double(0.0);
        REQUIRE(parsed_score >= 98.4);
        REQUIRE(parsed_score <= 98.6);

        flxv_map active = user["active"].to_map();
        REQUIRE(active["#text"].string_value() == "true");
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

        flxv_map company_name = company["name"].to_map();
        REQUIRE(company_name["#text"].string_value() == "Tech Corp");
        REQUIRE(company["employee"].in_state() == flx_variant::vector_state);

        flxv_vector employees = company["employee"].to_vector();
        REQUIRE(employees.size() == 2);

        flxv_map emp1 = employees[0].to_map();
        flxv_map emp1_name = emp1["name"].to_map();
        REQUIRE(emp1_name["#text"].string_value() == "Alice");
        flxv_map emp1_role = emp1["role"].to_map();
        REQUIRE(emp1_role["#text"].string_value() == "Engineer");

        flxv_map emp2 = employees[1].to_map();
        flxv_map emp2_name = emp2["name"].to_map();
        REQUIRE(emp2_name["#text"].string_value() == "Bob");
        flxv_map emp2_role = emp2["role"].to_map();
        REQUIRE(emp2_role["#text"].string_value() == "Manager");
      }
    }
  }
}

SCENARIO("flx_xml namespace handling") {
  GIVEN("An flxv_map and flx_xml instance") {
    flxv_map test_map;
    flx_xml xml(&test_map);

    WHEN("Parsing XML with namespaces on elements") {
      flx_string xml_str = R"(
        <ns:root xmlns:ns="http://example.com/ns">
          <ns:name>Test</ns:name>
          <ns:age>42</ns:age>
        </ns:root>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Namespaces should be stripped from element names") {
        REQUIRE(parsed == true);
        REQUIRE(test_map.find("root") != test_map.end());

        flxv_map root = test_map["root"].to_map();
        REQUIRE(root.find("name") != root.end());
        flxv_map name = root["name"].to_map();
        REQUIRE(name["#text"].string_value() == "Test");
        REQUIRE(root.find("age") != root.end());
        flxv_map age = root["age"].to_map();
        REQUIRE(age["#text"].string_value().to_int(0) == 42);
      }
    }

    WHEN("Parsing XML with namespaces on attributes") {
      flx_string xml_str = R"(
        <root xmlns:custom="http://example.com/custom" custom:id="123" custom:active="true">
          <name>Test</name>
        </root>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Namespaces should be stripped from attribute names") {
        REQUIRE(parsed == true);
        flxv_map root = test_map["root"].to_map();

        REQUIRE(root.find("@id") != root.end());
        REQUIRE(root["@id"].string_value().to_int(0) == 123);
        REQUIRE(root.find("@active") != root.end());
        REQUIRE(root["@active"].string_value() == "true");
        REQUIRE(root.find("name") != root.end());
        flxv_map name = root["name"].to_map();
        REQUIRE(name["#text"].string_value() == "Test");
      }
    }

    WHEN("Parsing XML with mixed namespaces") {
      flx_string xml_str = R"(
        <doc:document xmlns:doc="http://doc.com" xmlns:meta="http://meta.com">
          <meta:title>Report</meta:title>
          <doc:section meta:id="1">
            <doc:content>Text here</doc:content>
          </doc:section>
        </doc:document>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("All namespaces should be stripped consistently") {
        REQUIRE(parsed == true);
        REQUIRE(test_map.find("document") != test_map.end());

        flxv_map document = test_map["document"].to_map();
        REQUIRE(document.find("title") != document.end());
        flxv_map title = document["title"].to_map();
        REQUIRE(title["#text"].string_value() == "Report");

        REQUIRE(document.find("section") != document.end());
        flxv_map section = document["section"].to_map();
        REQUIRE(section.find("@id") != section.end());
        REQUIRE(section["@id"].string_value().to_int(0) == 1);
        REQUIRE(section.find("content") != section.end());
        flxv_map content = section["content"].to_map();
        REQUIRE(content["#text"].string_value() == "Text here");
      }
    }

    WHEN("Parsing XML with nested namespaced arrays") {
      flx_string xml_str = R"(
        <app:team xmlns:app="http://app.com">
          <app:member>
            <app:name>Alice</app:name>
          </app:member>
          <app:member>
            <app:name>Bob</app:name>
          </app:member>
        </app:team>
      )";
      bool parsed = xml.parse(xml_str);

      THEN("Namespaced arrays should work correctly") {
        REQUIRE(parsed == true);
        flxv_map team = test_map["team"].to_map();

        REQUIRE(team.find("member") != team.end());
        REQUIRE(team["member"].in_state() == flx_variant::vector_state);

        flxv_vector members = team["member"].to_vector();
        REQUIRE(members.size() == 2);

        flxv_map member1 = members[0].to_map();
        flxv_map name1 = member1["name"].to_map();
        REQUIRE(name1["#text"].string_value() == "Alice");

        flxv_map member2 = members[1].to_map();
        flxv_map name2 = member2["name"].to_map();
        REQUIRE(name2["#text"].string_value() == "Bob");
      }
    }
  }
}
