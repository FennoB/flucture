/**
 * @file test_mcp_adapter.cpp
 * @brief Tests for MCP adapter layer
 */

#include <catch2/catch_all.hpp>

#ifdef FLX_ENABLE_MCP

#include "api/mcp/flx_mcp_adapter.h"
#include "utils/flx_variant.h"
#include "utils/flx_string.h"

// Include MCP headers for test validation
#include "mcp_message.h"

using namespace flx;

SCENARIO("MCP adapter can check availability", "[mcp][unit]") {
    GIVEN("MCP support is compiled in") {
        WHEN("checking if MCP is available") {
            bool available = mcp_adapter::is_mcp_available();

            THEN("it should return true") {
                REQUIRE(available == true);
            }
        }

        WHEN("getting MCP version") {
            flx_string version = mcp_adapter::get_mcp_version();

            THEN("it should return a valid version string") {
                REQUIRE(!version.empty());
                REQUIRE(version == "2024-11-05");
            }
        }
    }
}

SCENARIO("MCP adapter can convert basic types", "[mcp][unit]") {
    GIVEN("a string variant") {
        flx_variant variant(flx_string("Hello, MCP!"));

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_string());
                REQUIRE(json.get<std::string>() == "Hello, MCP!");
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve value") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_string());
                    REQUIRE(result.string_value() == "Hello, MCP!");
                }
            }
        }
    }

    GIVEN("an integer variant") {
        flx_variant variant(42LL);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_number_integer());
                REQUIRE(json.get<int64_t>() == 42);
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve value") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_int());
                    REQUIRE(result.int_value() == 42LL);
                }
            }
        }
    }

    GIVEN("a boolean variant") {
        flx_variant variant(true);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_boolean());
                REQUIRE(json.get<bool>() == true);
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve value") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_bool());
                    REQUIRE(result.bool_value() == true);
                }
            }
        }
    }

    GIVEN("a double variant") {
        flx_variant variant(3.14159);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_number_float());
                REQUIRE(json.get<double>() == Catch::Approx(3.14159));
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve value") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_double());
                    REQUIRE(result.double_value() == Catch::Approx(3.14159));
                }
            }
        }
    }

    GIVEN("a null variant") {
        flx_variant variant;

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_null());
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve null state") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_null());
                }
            }
        }
    }
}

SCENARIO("MCP adapter can convert vectors", "[mcp][unit]") {
    GIVEN("a variant vector with mixed types") {
        flxv_vector vec;
        vec.push_back(flx_variant(flx_string("text")));
        vec.push_back(flx_variant(42LL));
        vec.push_back(flx_variant(true));
        vec.push_back(flx_variant(3.14));

        flx_variant variant(vec);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_array());
                REQUIRE(json.size() == 4);
                REQUIRE(json[0].get<std::string>() == "text");
                REQUIRE(json[1].get<int64_t>() == 42);
                REQUIRE(json[2].get<bool>() == true);
                REQUIRE(json[3].get<double>() == Catch::Approx(3.14));
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve array") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_vector());

                    const flxv_vector& result_vec = result.vector_value();
                    REQUIRE(result_vec.size() == 4);
                    REQUIRE(result_vec[0].string_value() == "text");
                    REQUIRE(result_vec[1].int_value() == 42LL);
                    REQUIRE(result_vec[2].bool_value() == true);
                    REQUIRE(result_vec[3].double_value() == Catch::Approx(3.14));
                }
            }
        }
    }

    GIVEN("a flxv_vector directly") {
        flxv_vector vec;
        vec.push_back(flx_variant(1LL));
        vec.push_back(flx_variant(2LL));
        vec.push_back(flx_variant(3LL));

        WHEN("converting using vector_to_mcp_json") {
            mcp::json json;
            bool success = mcp_adapter::vector_to_mcp_json(vec, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_array());
                REQUIRE(json.size() == 3);
            }

            AND_WHEN("converting back using mcp_json_to_vector") {
                flxv_vector result;
                bool back_success = mcp_adapter::mcp_json_to_vector(&json, result);

                THEN("round-trip conversion should preserve vector") {
                    REQUIRE(back_success);
                    REQUIRE(result.size() == 3);
                    REQUIRE(result[0].int_value() == 1LL);
                    REQUIRE(result[1].int_value() == 2LL);
                    REQUIRE(result[2].int_value() == 3LL);
                }
            }
        }
    }
}

SCENARIO("MCP adapter can convert maps", "[mcp][unit]") {
    GIVEN("a variant map with mixed value types") {
        flxv_map map;
        map[flx_string("name")] = flx_variant(flx_string("Alice"));
        map[flx_string("age")] = flx_variant(30LL);
        map[flx_string("active")] = flx_variant(true);
        map[flx_string("score")] = flx_variant(95.5);

        flx_variant variant(map);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_object());
                REQUIRE(json["name"].get<std::string>() == "Alice");
                REQUIRE(json["age"].get<int64_t>() == 30);
                REQUIRE(json["active"].get<bool>() == true);
                REQUIRE(json["score"].get<double>() == Catch::Approx(95.5));
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve object") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_map());

                    const flxv_map& result_map = result.map_value();
                    REQUIRE(result_map.at(flx_string("name")).string_value() == "Alice");
                    REQUIRE(result_map.at(flx_string("age")).int_value() == 30LL);
                    REQUIRE(result_map.at(flx_string("active")).bool_value() == true);
                    REQUIRE(result_map.at(flx_string("score")).double_value() == Catch::Approx(95.5));
                }
            }
        }
    }

    GIVEN("a flxv_map directly") {
        flxv_map map;
        map[flx_string("x")] = flx_variant(10LL);
        map[flx_string("y")] = flx_variant(20LL);

        WHEN("converting using map_to_mcp_json") {
            mcp::json json;
            bool success = mcp_adapter::map_to_mcp_json(map, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_object());
                REQUIRE(json["x"].get<int64_t>() == 10);
                REQUIRE(json["y"].get<int64_t>() == 20);
            }

            AND_WHEN("converting back using mcp_json_to_map") {
                flxv_map result;
                bool back_success = mcp_adapter::mcp_json_to_map(&json, result);

                THEN("round-trip conversion should preserve map") {
                    REQUIRE(back_success);
                    REQUIRE(result.size() == 2);
                    REQUIRE(result[flx_string("x")].int_value() == 10LL);
                    REQUIRE(result[flx_string("y")].int_value() == 20LL);
                }
            }
        }
    }
}

SCENARIO("MCP adapter can convert nested structures", "[mcp][unit]") {
    GIVEN("a nested structure with maps and vectors") {
        // Create: {"user": {"name": "Bob", "scores": [85, 90, 95]}}
        flxv_vector scores;
        scores.push_back(flx_variant(85LL));
        scores.push_back(flx_variant(90LL));
        scores.push_back(flx_variant(95LL));

        flxv_map user;
        user[flx_string("name")] = flx_variant(flx_string("Bob"));
        user[flx_string("scores")] = flx_variant(scores);

        flxv_map root;
        root[flx_string("user")] = flx_variant(user);

        flx_variant variant(root);

        WHEN("converting to MCP JSON") {
            mcp::json json;
            bool success = mcp_adapter::to_mcp_json(variant, &json);

            THEN("conversion should succeed") {
                REQUIRE(success);
                REQUIRE(json.is_object());
                REQUIRE(json["user"]["name"].get<std::string>() == "Bob");
                REQUIRE(json["user"]["scores"].is_array());
                REQUIRE(json["user"]["scores"].size() == 3);
                REQUIRE(json["user"]["scores"][0].get<int64_t>() == 85);
                REQUIRE(json["user"]["scores"][1].get<int64_t>() == 90);
                REQUIRE(json["user"]["scores"][2].get<int64_t>() == 95);
            }

            AND_WHEN("converting back to variant") {
                flx_variant result;
                bool back_success = mcp_adapter::from_mcp_json(&json, result);

                THEN("round-trip conversion should preserve nested structure") {
                    REQUIRE(back_success);
                    REQUIRE(result.is_map());

                    const flxv_map& result_root = result.map_value();
                    const flxv_map& result_user = result_root.at(flx_string("user")).map_value();
                    const flxv_vector& result_scores = result_user.at(flx_string("scores")).vector_value();

                    REQUIRE(result_user.at(flx_string("name")).string_value() == "Bob");
                    REQUIRE(result_scores.size() == 3);
                    REQUIRE(result_scores[0].int_value() == 85LL);
                    REQUIRE(result_scores[1].int_value() == 90LL);
                    REQUIRE(result_scores[2].int_value() == 95LL);
                }
            }
        }
    }
}

SCENARIO("MCP adapter handles error cases gracefully", "[mcp][unit]") {
    GIVEN("null pointers") {
        flx_variant variant(42LL);

        WHEN("converting with null output pointer") {
            bool success = mcp_adapter::to_mcp_json(variant, nullptr);

            THEN("it should return false") {
                REQUIRE(success == false);
            }
        }

        WHEN("converting from null input pointer") {
            flx_variant result;
            bool success = mcp_adapter::from_mcp_json(nullptr, result);

            THEN("it should return false") {
                REQUIRE(success == false);
            }
        }
    }

    GIVEN("a non-array JSON value") {
        mcp::json json = "not an array";

        WHEN("trying to convert to vector") {
            flxv_vector vec;
            bool success = mcp_adapter::mcp_json_to_vector(&json, vec);

            THEN("it should return false") {
                REQUIRE(success == false);
            }
        }
    }

    GIVEN("a non-object JSON value") {
        mcp::json json = 42;

        WHEN("trying to convert to map") {
            flxv_map map;
            bool success = mcp_adapter::mcp_json_to_map(&json, map);

            THEN("it should return false") {
                REQUIRE(success == false);
            }
        }
    }
}

#else // FLX_ENABLE_MCP not defined

// TEST_CASE("MCP support is disabled", "[mcp]") {
//     REQUIRE(flx::mcp_adapter::is_mcp_available() == false);
// }

#endif // FLX_ENABLE_MCP
