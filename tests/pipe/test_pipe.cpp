#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "../utils/temp_dir.h"
#include "pipe_communication.h"

// test init and close
TEST(PipeCommunicationTest, InitializeShutdown) {
  testutils::TempDir tmp;
  auto server = werewolf::make_server_pipe_communication(true, tmp.path());
  ASSERT_TRUE(server->initialize(2));
  server->shutdown();
}

// test player -> server (client sends, server receives)
TEST(PipeCommunicationTest, ClientToServer) {
  testutils::TempDir tmp;
  auto server = werewolf::make_server_pipe_communication(true, tmp.path());
  auto client = werewolf::make_client_pipe_communication(tmp.path());

  ASSERT_TRUE(server->initialize(1));
  ASSERT_TRUE(client->initialize(0));

  std::thread player([&]() {
    client->send("hello");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  });

  std::optional<std::string> msg;
  for (int i = 0; i < 10; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    msg = server->recv(0);
    if (msg) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg.value(), "hello");

  player.join();
  client->shutdown();
  server->shutdown();
}

// test server -> player (server sends, client receives)
TEST(PipeCommunicationTest, ServerToClient) {
  testutils::TempDir tmp;
  auto server = werewolf::make_server_pipe_communication(true, tmp.path());
  auto client = werewolf::make_client_pipe_communication(tmp.path());

  ASSERT_TRUE(server->initialize(1));
  ASSERT_TRUE(client->initialize(0));

  std::thread sender([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->send(0, "vote");
  });

  std::optional<std::string> msg;
  for (int i = 0; i < 10; i++) {
    msg = client->recv();
    if (msg) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg.value(), "vote");

  sender.join();
  client->shutdown();
  server->shutdown();
}

// multiple messages
TEST(PipeCommunicationTest, MultipleMessages) {
  testutils::TempDir tmp;
  auto server = werewolf::make_server_pipe_communication(true, tmp.path());
  auto client = werewolf::make_client_pipe_communication(tmp.path());

  ASSERT_TRUE(server->initialize(1));
  ASSERT_TRUE(client->initialize(0));

  std::thread sender([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->send(0, "hello2");
    server->send(0, "hello1");
    server->send(0, "hello0");
  });

  std::optional<std::string> msg;
  for (int i = 0; i < 10; i++) {
    msg = client->recv();
    if (msg) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg.value(), "hello2");

  msg = client->recv();
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg.value(), "hello1");

  msg = client->recv();
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg.value(), "hello0");

  sender.join();
  client->shutdown();
  server->shutdown();
}

// test broadcast
TEST(PipeCommunicationTest, BroadcastSendsToAllSlots) {
  testutils::TempDir tmp;
  auto server = werewolf::make_server_pipe_communication(true, tmp.path());
  auto client0 = werewolf::make_client_pipe_communication(tmp.path());
  auto client1 = werewolf::make_client_pipe_communication(tmp.path());

  ASSERT_TRUE(server->initialize(2));
  ASSERT_TRUE(client0->initialize(0));
  ASSERT_TRUE(client1->initialize(1));

  server->broadcast("hello all", {0, 1});

  std::optional<std::string> msg0, msg1;
  for (int i = 0; i < 10; i++) {
    if (!msg0) msg0 = client0->recv();
    if (!msg1) msg1 = client1->recv();
    if (msg0 && msg1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  ASSERT_TRUE(msg0.has_value());
  EXPECT_EQ(msg0.value(), "hello all");

  ASSERT_TRUE(msg1.has_value());
  EXPECT_EQ(msg1.value(), "hello all");

  client0->shutdown();
  client1->shutdown();
  server->shutdown();
}
