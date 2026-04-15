#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "werewolf/server_communication.h"

namespace werewolf::test {

class FakeServerCommunication : public IServerCommunication {
 public:
  bool initialize_result = true;
  int initialize_called = 0;
  int shutdown_called = 0;
  int last_initialize_slots = -1;
  int broadcast_called = 0;

  std::vector<std::pair<int, std::string>> sent_to_player;
  std::vector<std::pair<int, std::string>> sent_to_server_msgs;

  std::vector<std::deque<std::string>> slot_msg_q;

  explicit FakeServerCommunication(int n_players = 4) : slot_msg_q(n_players) {}

  bool initialize(int num_slots) override {
    std::lock_guard<std::mutex> lk(mu_);
    ++initialize_called;
    last_initialize_slots = num_slots;
    return initialize_result;
  }

  void shutdown() override {
    std::lock_guard<std::mutex> lk(mu_);
    ++shutdown_called;
    cv_.notify_all();
  }

  bool send(int slot, const std::string& msg) override {
    {
      std::lock_guard<std::mutex> lk(mu_);
      sent_to_player.emplace_back(slot, msg);
    }
    cv_.notify_all();
    return true;
  }

  std::optional<std::string> recv(int slot) override {
    std::lock_guard<std::mutex> lk(mu_);
    auto& q = slot_msg_q.at(slot);
    if (q.empty()) return std::nullopt;
    std::string msg = q.front();
    q.pop_front();
    return msg;
  }

  void broadcast(const std::string& msg,
                 const std::vector<int>& slots) override {
    {
      std::lock_guard<std::mutex> lk(mu_);
      ++broadcast_called;
      for (int slot : slots) {
        sent_to_player.emplace_back(slot, msg);
      }
    }
    cv_.notify_all();
  }

  void push_msg(int slot, const std::string& msg) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      slot_msg_q.at(slot).push_back(msg);
    }
    cv_.notify_all();
  }

  void connect_automatically() {
    for (int i = 0; i < static_cast<int>(slot_msg_q.size()); ++i) {
      push_msg(i, "connect");
    }
  }

  bool wait_until_sent(
      int slot, const std::string& needle,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)) {
    std::unique_lock<std::mutex> lk(mu_);
    return cv_.wait_for(lk, timeout, [&] {
      for (const auto& [s, msg] : sent_to_player) {
        if (s == slot && msg.find(needle) != std::string::npos) {
          return true;
        }
      }
      return false;
    });
  }

  bool wait_until_sent_to_all(
      const std::vector<int>& slots, const std::string& needle,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)) {
    std::unique_lock<std::mutex> lk(mu_);
    return cv_.wait_for(lk, timeout, [&] {
      for (int slot : slots) {
        bool found = false;
        for (const auto& [s, msg] : sent_to_player) {
          if (s == slot && msg.find(needle) != std::string::npos) {
            found = true;
            break;
          }
        }
        if (!found) return false;
      }
      return true;
    });
  }

 private:
  mutable std::mutex mu_;
  std::condition_variable cv_;
};

}  // namespace werewolf::test
