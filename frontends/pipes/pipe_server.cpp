#include "pipe_communication.h"
#include "werewolf/server_communication.h"
#include "werewolf/frontends/pipes/server_types.h"

#include <memory>

namespace {
werewolf::frontends::ServerCommunicationFactory makeServerPipeFactory() {
    return [](const werewolf::frontends::ServerOptions& options) -> std::unique_ptr<werewolf::IServerCommunication> {
        if(options.pipe_root == "") {
            return werewolf::make_server_pipe_communication(options.create_fifos);
        }
        return werewolf::make_server_pipe_communication(options.create_fifos, options.pipe_root);
    };
}
}; // namespace
 

int main(int argc, char* argv[]) {
    auto options = werewolf::frontends::ParseServerArgs(argc, argv);
    if(options.show_help) {
        werewolf::frontends::PrintServerUsage(argv[0]);
        return 0;
    }

    return werewolf::frontends::RunServer(options, makeServerPipeFactory());
}
