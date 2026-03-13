#include "template_communication.h"
#include <iostream>

namespace werewolf {

// server
bool TemplateServerCommunication::initialize(int num_slots) {
    std::cout << "TemplateServerCommunication init with " << num_slots << std::endl;
    return true;
}

void TemplateServerCommunication::shutdown() {
    std::cout << "TemplateServerCommunication shutdown" << std::endl;
}

bool TemplateServerCommunication::send(int slot, const std::string& msg) {
    std::cout << "[server -> player " << slot << "] " << msg << "\n";
    return true;
}

std::optional<std::string> TemplateServerCommunication::recv(int) {
    return std::nullopt;
}

// client
bool TemplateClientCommunication::initialize(int slot_num) {
    std::cout << "TemplateClientCommunication init with id: " << player_id << std::endl;
    return true;
}

void TemplateClientCommunication::shutdown() {
    std::cout << "TemplateClientCommunication shutdown" << std::endl;
}

bool TemplateClientCommunication::send(const std::string& msg) {
    std::cout << "[player " << player_id << " -> server] " << msg << "\n";
    return true;
}

std::optional<std::string> TemplateClientCommunication::recv() {
    return std::nullopt;
}

std::unique_ptr<IServerCommunication> make_server_template_communication() {
    return std::make_unique<TemplateServerCommunication>();
}

std::unique_ptr<IClientCommunication> make_client_template_communication() {
    return std::make_unique<TemplateClientCommunication>();
}

} // namespace werewolf
