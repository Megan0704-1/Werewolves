#include "pipe_communication.h"
#include "werewolf/communication.h"
#include "werewolf/frontends/pipes/server_types.h"

#include <memory>

namespace {
werewolf::frontends::CommunicationFactory makePipeFactory() {
    return [](const werewolf::frontends::ServerOptions& options) -> std::unique_ptr<werewolf::ICommunication> {
        if(options.pipe_root == "") {
            return werewolf::make_pipe_communication(options.create_fifos);
        }
        return werewolf::make_pipe_communication(options.create_fifos, options.pipe_root);
    };
}
}; // namespace
 

int main(int argc, char* argv[]) {
    auto options = werewolf::frontends::ParseServerArgs(argc, argv);
    if(options.show_help) {
        werewolf::frontends::PrintServerUsage(argv[0]);
        return 0;
    }

    return werewolf::frontends::RunServer(options, makePipeFactory());
}
