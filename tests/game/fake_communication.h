#pragma once

#include "werewolf/communication.h"

#include <optional>
#include <string>
#include <vector>

namespace werewolf::test {

class FakeCommunication : public ICommunication {
public:
    bool initialize_result = true;
    int initialize_called = 0;
    int shutdown_called = 0;
    int last_initialize_slots = -1;

    std::vector<std::pair<int, std::string>> sent_to_player;
    std::vector<std::pair<int, std::string>> sent_to_server_msgs;

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

    std::optional<std::string> recv_from_player(int) override {
        return std::nullopt;
    }

    bool send_to_server(int slot, const std::string& msg) override {
        sent_to_server_msgs.emplace_back(slot, msg);
        return true;
    }

    std::optional<std::string> recv_from_server(int) override {
        return std::nullopt;
    }
};

} // namespace werewolf::test
