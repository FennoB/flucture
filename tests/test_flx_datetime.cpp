#include <catch2/catch_all.hpp>
#include "utils/flx_datetime.h"

SCENARIO("flx_datetime basic construction and conversion") {
    GIVEN("a datetime from components") {
        flx_datetime dt(2023, 12, 15, 14, 30, 45, 123);
        
        WHEN("accessing components") {
            THEN("all components are correct") {
                REQUIRE(dt.year() == 2023);
                REQUIRE(dt.month() == 12);
                REQUIRE(dt.day() == 15);
                REQUIRE(dt.hour() == 14);
                REQUIRE(dt.minute() == 30);
                REQUIRE(dt.second() == 45);
                REQUIRE(dt.millisecond() == 123);
            }
        }
        
        WHEN("converting to ISO string") {
            flx_string iso = dt.to_iso();
            THEN("ISO format is correct") {
                REQUIRE(iso.contains("2023-12-15T"));
                REQUIRE(iso.contains(":30:45"));
                REQUIRE(iso.contains("14:30:45"));
            }
        }
    }
    
    GIVEN("an ISO string") {
        flx_string iso_str = "2023-12-15T14:30:45.123Z";
        
        WHEN("constructing datetime from string") {
            flx_datetime dt(iso_str);
            THEN("components are parsed correctly") {
                REQUIRE(dt.year() == 2023);
                REQUIRE(dt.month() == 12);
                REQUIRE(dt.day() == 15);
                REQUIRE(dt.hour() == 14);
                REQUIRE(dt.minute() == 30);
                REQUIRE(dt.second() == 45);
                REQUIRE(dt.millisecond() == 123);
            }
        }
    }
}

SCENARIO("flx_datetime implicit string conversion") {
    GIVEN("a datetime object") {
        flx_datetime dt(2023, 6, 1, 12, 0, 0, 0);
        
        WHEN("implicitly converting to string") {
            flx_string str = dt;
            THEN("conversion works seamlessly") {
                REQUIRE(str.contains("2023-06-01"));
                REQUIRE(str.contains("12:00:00"));
            }
        }
        
        WHEN("assigning from string") {
            dt = "2024-01-01T00:00:00Z";
            THEN("assignment works") {
                REQUIRE(dt.year() == 2024);
                REQUIRE(dt.month() == 1);
                REQUIRE(dt.day() == 1);
            }
        }
    }
}

SCENARIO("flx_duration functionality") {
    GIVEN("duration objects") {
        auto dur1 = flx_duration::hours(2);
        auto dur2 = flx_duration::minutes(30);
        auto dur3 = flx_duration::seconds(45);
        
        WHEN("combining durations") {
            auto total = dur1 + dur2 + dur3;
            THEN("totals are correct") {
                REQUIRE(total.total_hours() == 2);
                REQUIRE(total.total_minutes() == 150);
                REQUIRE(total.total_seconds() == 9045);
            }
        }
    }
}

SCENARIO("flx_datetime arithmetic") {
    GIVEN("a base datetime") {
        flx_datetime dt(2023, 6, 15, 12, 0, 0, 0);
        
        WHEN("adding durations") {
            auto future = dt + flx_duration::days(7);
            THEN("date advances correctly") {
                REQUIRE(future.day() == 22);
                REQUIRE(future.month() == 6);
            }
        }
        
        WHEN("adding months") {
            auto next_month = dt.add_months(1);
            THEN("month advances correctly") {
                REQUIRE(next_month.month() == 7);
                REQUIRE(next_month.day() == 15);
            }
        }
        
        WHEN("calculating differences") {
            auto future = dt.add_days(30);
            auto diff = future.days_between(dt);
            THEN("difference is correct") {
                REQUIRE(diff == 30);
            }
        }
    }
}

SCENARIO("flx_datetime formatting") {
    GIVEN("a datetime") {
        flx_datetime dt(2023, 3, 5, 9, 15, 30, 0);
        
        WHEN("formatting to different strings") {
            THEN("all formats work") {
                REQUIRE(dt.to_iso_date() == "2023-03-05");
                REQUIRE(dt.to_iso_time() == "09:15:30");
                REQUIRE(dt.to_date_string() == "05.03.2023");
                REQUIRE(dt.to_time_string() == "09:15:30");
                REQUIRE(dt.to_datetime_string() == "05.03.2023 09:15:30");
            }
        }
        
        WHEN("using custom format") {
            auto formatted = dt.format("%Y/%m/%d");
            THEN("custom format works") {
                REQUIRE(formatted == "2023/03/05");
            }
        }
    }
}

SCENARIO("flx_datetime boundary operations") {
    GIVEN("a datetime in the middle of week/month/year") {
        flx_datetime dt(2023, 6, 15, 14, 30, 45, 0);
        
        WHEN("getting boundaries") {
            THEN("all boundaries are correct") {
                REQUIRE(dt.start_of_day().hour() == 0);
                REQUIRE(dt.start_of_day().minute() == 0);
                REQUIRE(dt.end_of_day().hour() == 23);
                REQUIRE(dt.end_of_day().minute() == 59);
                
                REQUIRE(dt.start_of_month().day() == 1);
                REQUIRE(dt.end_of_month().day() == 30);
                
                REQUIRE(dt.start_of_year().month() == 1);
                REQUIRE(dt.start_of_year().day() == 1);
            }
        }
        
        WHEN("checking day properties") {
            THEN("day properties work") {
                REQUIRE(dt.day_of_week() >= 1);
                REQUIRE(dt.day_of_week() <= 7);
                REQUIRE(dt.weekday_name().size() > 3);
                REQUIRE(dt.weekday_short().size() == 3);
            }
        }
    }
}

SCENARIO("flx_datetime validation and error handling") {
    GIVEN("invalid date components") {
        WHEN("constructing with invalid date") {
            THEN("exception is thrown") {
                REQUIRE_THROWS_AS(flx_datetime(2023, 13, 1), flx_datetime_exception);
                REQUIRE_THROWS_AS(flx_datetime(2023, 2, 30), flx_datetime_exception);
                REQUIRE_THROWS_AS(flx_datetime(2023, 6, 15, 25, 0, 0), flx_datetime_exception);
            }
        }
        
        WHEN("parsing invalid ISO string") {
            THEN("exception is thrown") {
                REQUIRE_THROWS_AS(flx_datetime("invalid"), flx_datetime_exception);
                REQUIRE_THROWS_AS(flx_datetime("2023-13-01"), flx_datetime_exception);
            }
        }
    }
    
    GIVEN("validation functions") {
        WHEN("checking valid dates") {
            THEN("validation works correctly") {
                REQUIRE(flx_datetime::is_valid_date(2023, 2, 28));
                REQUIRE(flx_datetime::is_valid_date(2024, 2, 29));
                REQUIRE_FALSE(flx_datetime::is_valid_date(2023, 2, 29));
                
                REQUIRE(flx_datetime::is_valid_time(12, 30, 45, 123));
                REQUIRE_FALSE(flx_datetime::is_valid_time(25, 0, 0));
            }
        }
    }
}

SCENARIO("flx_datetime utility functions") {
    GIVEN("datetime objects for comparison") {
        flx_datetime dt1(2023, 6, 15, 12, 0, 0, 0);
        flx_datetime dt2(2023, 6, 15, 18, 0, 0, 0);
        flx_datetime dt3(2023, 7, 15, 12, 0, 0, 0);
        
        WHEN("comparing dates") {
            THEN("comparisons work") {
                REQUIRE(dt1.is_same_day(dt2));
                REQUIRE_FALSE(dt1.is_same_day(dt3));
                REQUIRE(dt1.is_same_month(dt2));
                REQUIRE_FALSE(dt1.is_same_month(dt3));
                REQUIRE(dt1.is_same_year(dt3));
            }
        }
        
        WHEN("checking weekend") {
            auto saturday = flx_datetime(2023, 6, 17);
            auto monday = flx_datetime(2023, 6, 19);
            THEN("weekend detection works") {
                REQUIRE(saturday.is_weekend());
                REQUIRE_FALSE(monday.is_weekend());
            }
        }
        
        WHEN("calculating age") {
            auto birth = flx_datetime(1990, 6, 15);
            auto reference = flx_datetime(2023, 6, 14);
            auto reference2 = flx_datetime(2023, 6, 16);
            THEN("age calculation works") {
                REQUIRE(birth.age_at_date(reference) == 32);
                REQUIRE(birth.age_at_date(reference2) == 33);
            }
        }
    }
}

SCENARIO("flx_string enhanced functionality") {
    GIVEN("strings for testing") {
        flx_string text = "  Hello World  ";
        flx_string number = "123.45";
        flx_string mixed = "The Quick Brown Fox";
        
        WHEN("trimming whitespace") {
            THEN("trim functions work") {
                REQUIRE(text.trim() == "Hello World");
                REQUIRE(text.trim_left() == "Hello World  ");
                REQUIRE(text.trim_right() == "  Hello World");
            }
        }
        
        WHEN("checking string properties") {
            THEN("property checks work") {
                REQUIRE(text.starts_with("  Hello"));
                REQUIRE(text.ends_with("World  "));
                REQUIRE(number.is_numeric());
                REQUIRE_FALSE(mixed.is_numeric());
            }
        }
        
        WHEN("manipulating case") {
            THEN("case functions work") {
                REQUIRE(mixed.capitalize() == "The quick brown fox");
                REQUIRE(mixed.title_case() == "The Quick Brown Fox");
                REQUIRE(mixed.upper() == "THE QUICK BROWN FOX");
            }
        }
        
        WHEN("padding and repeating") {
            flx_string short_str = "Hi";
            THEN("padding works") {
                REQUIRE(short_str.pad_left(5, '*') == "***Hi");
                REQUIRE(short_str.pad_right(5, '*') == "Hi***");
                REQUIRE(short_str.repeat(3) == "HiHiHi");
            }
        }
        
        WHEN("counting and searching") {
            THEN("search functions work") {
                REQUIRE(mixed.count("o") == 2);
                REQUIRE(mixed.count('o') == 2);
                REQUIRE(mixed.remove("Quick ") == "The Brown Fox");
            }
        }
    }
}