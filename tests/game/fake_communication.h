#pragma once

#include "werewolf/communication.h"

#include <optional>
#include <string>
#include <vector>
#include <queue>

namespace werewolf::test {

class FakeCommunication : public ICommunication {
public:
    bool initialize_result = true;
    int initialize_called = 0;
    int shutdown_called = 0;
    int last_initialize_slots = -1;

    std::vector<std::pair<int, std::string>> sent_to_player;
    std::vector<std::pair<int, std::string>> sent_to_server_msgs;

    std::vector<std::deque<std::string>> slot_msg_q;

    explicit FakeCommunication(int n_players=4): slot_msg_q(n_players) {}

    bool initialize(int num_slots) override {
        ++initialize_called;
        last_initialize_slots = num_slots;
        return initialize_result;
    }

    void shutdown() override {
        ++shutdown_called;
    }

    bool send_to_player(int slot, const std::string& msg) override {
        sent_to_player.emplace_back(slot, msg);
        return true;
    }

    std::optional<std::string> recv_from_player(int slot) override {
        auto &q = slot_msg_q.at(slot);
        if(q.empty()) return std::nullopt;
        std::string msg = q.front(); q.pop_front();
        return msg;
    }

    bool send_to_server(int slot, const std::string& msg) override {
        sent_to_server_msgs.emplace_back(slot, msg);
        return true;
    }

    std::optional<std::string> recv_from_server(int) override {
        return std::nullopt;
    }

    void push_msg(int slot, const std::string& msg) {
        slot_msg_q.at(slot).push_back(msg);
    }

    void connect_automatically() {
        for(int i=0; i<slot_msg_q.size(); ++i) {
            push_msg(i, "connect");
        }
    }
};

} // namespace werewolf::test
