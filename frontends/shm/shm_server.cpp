#include <memory>

#include "shm_communication.h"
#include "werewolf/frontends/shm/server_types.h"
#include "werewolf/server_communication.h"

namespace {
werewolf::frontends::ServerCommunicationFactory makeServerShmFactory() {
  return [](const werewolf::frontends::ServerOptions& options)
             -> std::unique_ptr<werewolf::IServerCommunication> {
    if (options.shm_name == "") {
      return werewolf::make_server_shm_communication();
    }
    return werewolf::make_server_shm_communication(options.shm_name, true);
  };
}
};  // namespace

int main(int argc, char* argv[]) {
  auto options = werewolf::frontends::ParseServerArgs(argc, argv);
  if (options.show_help) {
    werewolf::frontends::PrintServerUsage(argv[0]);
    return 0;
  }

  return werewolf::frontends::RunServer(options, makeServerShmFactory());
}
