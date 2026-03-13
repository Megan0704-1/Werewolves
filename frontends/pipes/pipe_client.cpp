#include "pipe_communication.h"
#include "werewolf/client_communication.h"
#include "werewolf/frontends/pipes/client_types.h"

#include <memory>
#include <string>

namespace {
werewolf::frontends::ClientCommunicationFactory makeClientPipeFactory() {
    return [](const werewolf::frontends::ClientOptions& options) -> std::unique_ptr<werewolf::IClientCommunication> {
        return werewolf::make_client_pipe_communication(options.pipe_root);
    };
}
} // namespace

int main(int argc, char* argv[]) {
    auto options = werewolf::frontends::ParseClientArgs(argc, argv);
    if(options.show_help) {
        werewolf::frontends::PrintClientUsage(argv[0]);
        return 0;
    }
    return werewolf::frontends::RunClient(options, makeClientPipeFactory());
}

