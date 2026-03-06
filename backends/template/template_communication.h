#pragma once

#include "werewolf/communication.h"

namespace werewolf {

class TemplateCommunication : public ICommunication {
public:
    bool initialize(int num_slots) override;
    void shutdown() override;
    bool send_to_player(int slot, const std::string& msg) override;
    std::optional<std::string> recv_from_player(int slot) override;
    bool send_to_server(int slot, const std::string& msg) override;
    std::optional<std::string> recv_from_server(int slot) override;
};

std::unique_ptr<ICommunication> make_template_communication();

}// namespace werewolf

