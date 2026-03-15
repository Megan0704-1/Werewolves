#pragma once

#include "werewolf/client_communication.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace werewolf::frontends {

struct ClientOptions {
    int slot = -1;
    std::string shm_name = "/werewolf_shm";
    bool show_help = false;
};

using ClientCommunicationFactory = std::function<std::unique_ptr<werewolf::IClientCommunication>(const ClientOptions& options)>;

void PrintClientUsage(std::string_view bin);

ClientOptions ParseClientArgs(int argc, char* argv[]);

int RunClient(const ClientOptions& options, const ClientCommunicationFactory& factory);

} // namespace werewolf::frontends
