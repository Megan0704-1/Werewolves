#include "template_communication.h"

int main() {

    auto comm = werewolf::make_template_communication();
    comm->initialize(4);
    comm->send_to_player(0, "hello player");
    comm->send_to_server(0, "hello server");
    comm->shutdown();

    return 0;
}
