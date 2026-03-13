#pragma once

#include "werewolf/types.h"
#include "werewolf/communication.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace werewolf::frontends {

struct ServerOptions {
    GameConfig game_cfg;
    std::string pipe_root = "/home/moderator/pipes/";
    bool create_fifos = false;
    bool show_help = false;
};

using CommunicationFactory = std::function<std::unique_ptr<ICommunication>(const ServerOptions&)>;

void PrintServerUsage(std::string_view prog_name);

ServerOptions ParseServerArgs(int argc, char* argv[]);

int RunServer(const ServerOptions& options, const CommunicationFactory& factory);

}; // namespace werewolf::frontends
