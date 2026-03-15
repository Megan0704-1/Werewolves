#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "werewolf/server_communication.h"
#include "werewolf/types.h"

namespace werewolf::frontends {

struct ServerOptions {
  GameConfig game_cfg;
  std::string shm_name = "/werewolf_shm";
  bool show_help = false;
};

using ServerCommunicationFactory =
    std::function<std::unique_ptr<IServerCommunication>(const ServerOptions&)>;

void PrintServerUsage(std::string_view prog_name);

ServerOptions ParseServerArgs(int argc, char* argv[]);

int RunServer(const ServerOptions& options,
              const ServerCommunicationFactory& factory);

};  // namespace werewolf::frontends
