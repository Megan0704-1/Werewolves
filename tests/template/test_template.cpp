#include <gtest/gtest.h>

#include "template_communication.h"

// suite name, test name
TEST(TemplateCommunication, ServerBasicLifeCycle) {
  auto comm = werewolf::make_server_template_communication();
  ASSERT_TRUE(comm->initialize(4));
  EXPECT_TRUE(comm->send(0, "hello player"));
  comm->shutdown();
}

TEST(TemplateCommunication, ClientBasicLifeCycle) {
  auto comm = werewolf::make_client_template_communication();
  ASSERT_TRUE(comm->initialize(0));
  EXPECT_TRUE(comm->send("hello server"));
  comm->shutdown();
}

TEST(TemplateCommunication, ServerBroadcast) {
  auto comm = werewolf::make_server_template_communication();
  ASSERT_TRUE(comm->initialize(4));
  comm->broadcast("hello all", {0, 1, 2, 3});
  comm->shutdown();
}
