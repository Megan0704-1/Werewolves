#include <gtest/gtest.h>
#include "template_communication.h"

// suite name, test name
TEST(TemplateCommunication, BasicLifeCycle) {
    auto comm = werewolf::make_template_communication();
    ASSERT_TRUE(comm->initialize(4));
    EXPECT_TRUE(comm->send_to_player(0, "hello player"));
    EXPECT_TRUE(comm->send_to_server(0, "hello server"));

    comm->shutdown();
}
