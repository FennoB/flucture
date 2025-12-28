#include <catch2/catch_all.hpp>
#include "../api/db/reconnect_helper.h"
#include "../api/db/db_connection.h"
#include "../api/db/db_query.h"
#include <thread>
#include <chrono>

// ============================================================================
// MOCK CONNECTION - For Unit Testing reconnect_helper
// ============================================================================

class mock_connection : public db_connection {
private:
  bool connected_;
  int reconnect_call_count_;
  bool next_reconnect_will_succeed_;
  flx_string connection_string_;

public:
  mock_connection()
    : connected_(false)
    , reconnect_call_count_(0)
    , next_reconnect_will_succeed_(true)
  {}

  // Control mock behavior
  void set_connected(bool connected) { connected_ = connected; }
  void set_next_reconnect_succeeds(bool succeeds) { next_reconnect_will_succeed_ = succeeds; }
  int get_reconnect_call_count() const { return reconnect_call_count_; }
  void reset_call_count() { reconnect_call_count_ = 0; }

  // db_connection interface
  bool connect(const flx_string& connection_string) override {
    connection_string_ = connection_string;
    connected_ = true;
    return true;
  }

  bool disconnect() override {
    connected_ = false;
    return true;
  }

  bool is_connected() const override {
    return connected_;
  }

  std::unique_ptr<db_query> create_query() override {
    return nullptr;  // Not needed for reconnect tests
  }

  flx_string get_last_error() const override {
    return "";
  }

  bool reconnect() override {
    reconnect_call_count_++;

    if (next_reconnect_will_succeed_) {
      connected_ = true;
      return true;
    } else {
      connected_ = false;
      return false;
    }
  }
};

// ============================================================================
// UNIT TESTS - reconnect_helper with mock_connection
// ============================================================================

SCENARIO("reconnect_helper manages background reconnection", "[unit][reconnect]") {

  GIVEN("A disconnected mock connection") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(true);  // Will succeed on first try

    reconnect_helper helper(&mock);

    WHEN("reconnect loop is started") {
      helper.start_reconnect_loop();

      // Wait for reconnect to complete
      std::this_thread::sleep_for(std::chrono::milliseconds(300));

      THEN("reconnect is called once") {
        REQUIRE(mock.get_reconnect_call_count() == 1);
      }

      THEN("connection is restored") {
        REQUIRE(mock.is_connected());
      }

      THEN("attempt count is reset after success") {
        REQUIRE(helper.get_attempt_count() == 0);
      }

      // Clean shutdown - thread stops automatically after success
      // Give it time to exit cleanly
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  GIVEN("A connection that fails reconnect multiple times") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(false);  // All attempts fail

    reconnect_helper helper(&mock);

    WHEN("reconnect loop runs for several attempts") {
      helper.start_reconnect_loop();

      // Wait for multiple attempts (1s + 2s = 3s + buffer)
      std::this_thread::sleep_for(std::chrono::milliseconds(3500));

      THEN("multiple reconnect attempts are made") {
        // Should have attempt 1 (immediate), attempt 2 (after 1s), attempt 3 (after 2s)
        REQUIRE(mock.get_reconnect_call_count() >= 2);
      }

      THEN("attempt count increases") {
        REQUIRE(helper.get_attempt_count() >= 2);
      }

      THEN("delay increases exponentially") {
        int delay = helper.get_retry_after_ms();
        // After 2 failures: 1s -> 2s -> 4s (next delay)
        REQUIRE(delay >= 2000);  // At least 2 seconds
        REQUIRE(delay <= 60000);  // Capped at 60 seconds
      }

      helper.stop();  // Clean stop
    }
  }

  GIVEN("A connection that fails twice then succeeds") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(false);

    reconnect_helper helper(&mock);

    WHEN("reconnect succeeds on third attempt") {
      helper.start_reconnect_loop();

      // Let first two attempts fail
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));

      // Now make next attempt succeed
      mock.set_next_reconnect_succeeds(true);

      // Wait for successful reconnect
      std::this_thread::sleep_for(std::chrono::milliseconds(2500));

      THEN("connection is eventually restored") {
        REQUIRE(mock.is_connected());
      }

      THEN("backoff is reset after success") {
        REQUIRE(helper.get_attempt_count() == 0);
        REQUIRE(helper.get_retry_after_ms() == 1000);  // Back to 1s
      }
    }
  }

  GIVEN("A helper with already running loop") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(false);  // Keep failing

    reconnect_helper helper(&mock);

    WHEN("start_reconnect_loop is called twice") {
      helper.start_reconnect_loop();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      mock.reset_call_count();
      helper.start_reconnect_loop();  // Second call - should be ignored

      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      THEN("only one thread is running") {
        // If two threads were started, call count would be ~double
        // With one thread, we expect ~1 call in 200ms
        REQUIRE(mock.get_reconnect_call_count() <= 2);
      }

      helper.stop();
    }
  }
}

SCENARIO("reconnect_helper provides accurate retry information", "[unit][reconnect]") {

  GIVEN("A helper that has failed reconnects") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(false);  // Keep failing

    reconnect_helper helper(&mock);

    WHEN("reconnect attempts are in progress") {
      helper.start_reconnect_loop();

      // Wait long enough for at least one attempt to be in progress or waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      THEN("is_attempting_reconnect or attempt_count indicates activity") {
        // Either currently attempting, or has already attempted (and is waiting for next)
        REQUIRE((helper.is_attempting_reconnect() || helper.get_attempt_count() >= 1));
      }

      THEN("retry_after_ms returns current delay") {
        int delay = helper.get_retry_after_ms();
        REQUIRE(delay >= 1000);  // At least base delay
      }

      THEN("attempt_count increases with failures") {
        REQUIRE(helper.get_attempt_count() >= 1);
      }

      helper.stop();
    }
  }

  GIVEN("A helper that has succeeded reconnect") {
    mock_connection mock;
    mock.set_connected(false);
    mock.set_next_reconnect_succeeds(true);

    reconnect_helper helper(&mock);
    helper.start_reconnect_loop();

    WHEN("reconnect succeeds") {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      THEN("is_attempting_reconnect returns false") {
        REQUIRE_FALSE(helper.is_attempting_reconnect());
      }

      THEN("attempt_count is reset to 0") {
        REQUIRE(helper.get_attempt_count() == 0);
      }
    }
  }
}
