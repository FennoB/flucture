#ifndef RECONNECT_HELPER_H
#define RECONNECT_HELPER_H

#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

// Forward declaration
class db_connection;

// ============================================================================
// RECONNECT HELPER - Background Reconnection with Exponential Backoff
// ============================================================================
//
// Manages automatic database reconnection attempts in a background thread.
//
// Features:
// - Exponential backoff: 1s → 2s → 4s → 8s → 16s → 32s → 60s (cap)
// - Non-blocking: reconnect attempts run in background thread
// - Thread-safe: safe to call from multiple threads
// - Auto-stops on successful reconnect
// - Polymorphic design: works with any db_connection implementation
//
// Usage:
//   reconnect_helper helper(db_conn);
//   helper.start_reconnect_loop();  // Starts background thread
//
//   // In create_query():
//   if (!is_connected()) {
//     throw db_not_reachable("...", helper.get_retry_after_ms(), ...);
//   }
//
// ============================================================================

class reconnect_helper {
public:
  // Constructor takes a db_connection pointer
  // The connection's reconnect() method will be called on failures
  explicit reconnect_helper(db_connection* connection);

  // Destructor stops the background thread gracefully
  ~reconnect_helper();

  // Start background reconnect loop (safe to call multiple times)
  void start_reconnect_loop();

  // Stop background thread (called automatically on success or in destructor)
  void stop();

  // Get milliseconds until next reconnect attempt (for error messages)
  int get_retry_after_ms() const;

  // Get number of failed reconnect attempts
  int get_attempt_count() const;

  // Check if currently attempting reconnect
  bool is_attempting_reconnect() const;

  // Reset after successful manual reconnect (resets backoff)
  void reset();

private:
  // Background thread function
  void reconnect_loop();

  // Calculate next delay using exponential backoff
  void calculate_next_delay();

  // Database connection (not owned - caller manages lifetime)
  db_connection* connection_;

  // Background thread
  std::thread reconnect_thread_;

  // Thread control
  std::atomic<bool> running_;
  std::atomic<bool> is_reconnecting_;

  // Backoff state (constants MUST come before atomics that use them!)
  const int base_delay_ms_ = 1000;   // 1 second
  const int max_delay_ms_ = 60000;   // 60 seconds (cap)
  std::atomic<int> attempt_count_;
  std::atomic<int> current_delay_ms_;

  // Next attempt time
  std::chrono::steady_clock::time_point next_attempt_;

  // Synchronization
  std::mutex mutex_;
  std::condition_variable cv_;
};

#endif // RECONNECT_HELPER_H
