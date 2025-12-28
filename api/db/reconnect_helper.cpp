#include "reconnect_helper.h"
#include "db_connection.h"
#include <iostream>
#include <algorithm>

reconnect_helper::reconnect_helper(db_connection* connection)
  : connection_(connection)
  , running_(false)
  , is_reconnecting_(false)
  , attempt_count_(0)
  , current_delay_ms_(base_delay_ms_)
{
}

reconnect_helper::~reconnect_helper()
{
  // Signal thread to stop
  {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
  }
  cv_.notify_all();

  // ALWAYS join thread in destructor if it exists
  if (reconnect_thread_.joinable()) {
    reconnect_thread_.join();
  }
}

void reconnect_helper::start_reconnect_loop()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Already running - don't start another thread
  if (running_) {
    return;
  }

  running_ = true;
  next_attempt_ = std::chrono::steady_clock::now(); // Start immediately

  // Launch background thread
  reconnect_thread_ = std::thread(&reconnect_helper::reconnect_loop, this);
}

void reconnect_helper::stop()
{
  bool was_running = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    was_running = running_;
    running_ = false;
  }

  // Only join if thread was actually running
  if (was_running) {
    // Wake up thread if waiting
    cv_.notify_all();

    // Wait for thread to finish (only if joinable)
    if (reconnect_thread_.joinable()) {
      reconnect_thread_.join();
    }
  }
}

void reconnect_helper::reconnect_loop()
{
  while (running_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);

      // Wait until next attempt time or stop signal
      if (cv_.wait_until(lock, next_attempt_, [this] { return !running_; })) {
        // running_ became false - exit
        break;
      }
    }

    // Attempt reconnect
    is_reconnecting_ = true;

    int current_attempt = attempt_count_ + 1;
    std::cout << "[DB] Attempting reconnect (attempt " << current_attempt << ")..." << std::endl;

    bool success = connection_->reconnect();

    if (success) {
      std::cout << "[DB] Reconnect successful!" << std::endl;
      reset();
      is_reconnecting_ = false;

      // Stop thread - we're connected!
      {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
      }
      break;
    } else {
      // Reconnect failed
      attempt_count_++;
      calculate_next_delay();

      int delay_ms = current_delay_ms_.load();
      next_attempt_ = std::chrono::steady_clock::now() +
                      std::chrono::milliseconds(delay_ms);

      std::cout << "[DB] Reconnect failed. Next attempt in "
                << delay_ms / 1000 << " seconds" << std::endl;

      is_reconnecting_ = false;
    }
  }
}

void reconnect_helper::calculate_next_delay()
{
  // Exponential backoff: double the delay each time, cap at max_delay_ms_
  int current = current_delay_ms_.load();
  int new_delay = std::min(current * 2, max_delay_ms_);
  current_delay_ms_.store(new_delay);
}

int reconnect_helper::get_retry_after_ms() const
{
  return current_delay_ms_;
}

int reconnect_helper::get_attempt_count() const
{
  return attempt_count_;
}

bool reconnect_helper::is_attempting_reconnect() const
{
  return is_reconnecting_;
}

void reconnect_helper::reset()
{
  attempt_count_.store(0);
  current_delay_ms_.store(base_delay_ms_);
}
