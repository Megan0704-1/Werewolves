#include "template_communication.h"
#include <iostream>

namespace werewolf {

bool TemplateCommunication::initialize(int num_slots) {
    std::cout << "TemplateCommunication init with " << num_slots << std::endl;
    return true;
}

void TemplateCommunication::shutdown() {
    std::cout << "TemplateCommunication shutdown" << std::endl;
}

bool TemplateCommunication::send_to_player(int slot, const std::string& msg) {
    std::cout << "[server -> player " << slot << "] " << msg << "\n";
    return true;
}

std::optional<std::string> TemplateCommunication::recv_from_player(int) {
    return std::nullopt;
}

bool TemplateCommunication::send_to_server(int slot, const std::string& msg) {
    std::cout << "[player " << slot << " -> server] " << msg << "\n";
    return true;
}

std::optional<std::string> TemplateCommunication::recv_from_server(int) {
    return std::nullopt;
}

std::unique_ptr<ICommunication> make_template_communication() {
    return std::make_unique<TemplateCommunication>();
}
} // namespace werewolf
