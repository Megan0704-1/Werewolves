#include <gtest/gtest.h>
#include "shm_communication.h"
#include <thread>
#include <chrono>
#include <atomic>

// Unique shm name per test to avoid collisions with parallel runs.
static std::atomic<int> shm_counter{0};
static std::string unique_shm_name() {
    return "/ww_test_shm_" + std::to_string(getpid()) + "_" + std::to_string(shm_counter++);
}


TEST(ShmCommunicationTest, InitializeShutdown) {
    auto name = unique_shm_name();
    bool create_shm = true;
    auto server = werewolf::make_server_shm_communication(name, create_shm);
    ASSERT_TRUE(server->initialize(2));
    server->shutdown();
}


TEST(ShmCommunicationTest, ClientToServer) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    auto client = werewolf::make_client_shm_communication(name);

    ASSERT_TRUE(server->initialize(1));
    ASSERT_TRUE(client->initialize(0));

    ASSERT_TRUE(client->send("hello"));

    std::optional<std::string> msg;
    for (int i = 0; i < 20; ++i) {
        msg = server->recv(0);
        if (msg) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg.value(), "hello");

    client->shutdown();
    server->shutdown();
}


TEST(ShmCommunicationTest, ServerToClient) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    auto client = werewolf::make_client_shm_communication(name);

    ASSERT_TRUE(server->initialize(1));
    ASSERT_TRUE(client->initialize(0));

    ASSERT_TRUE(server->send(0, "vote"));

    std::optional<std::string> msg;
    for (int i = 0; i < 20; ++i) {
        msg = client->recv();
        if (msg) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg.value(), "vote");

    client->shutdown();
    server->shutdown();
}


TEST(ShmCommunicationTest, MultipleMessages) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    auto client = werewolf::make_client_shm_communication(name);

    ASSERT_TRUE(server->initialize(1));
    ASSERT_TRUE(client->initialize(0));

    ASSERT_TRUE(server->send(0, "msg_a"));
    ASSERT_TRUE(server->send(0, "msg_b"));
    ASSERT_TRUE(server->send(0, "msg_c"));

    auto poll = [&]() -> std::optional<std::string> {
        for (int i = 0; i < 20; ++i) {
            auto m = client->recv();
            if (m) return m;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return std::nullopt;
    };

    auto m1 = poll();
    ASSERT_TRUE(m1.has_value());
    EXPECT_EQ(*m1, "msg_a");

    auto m2 = poll();
    ASSERT_TRUE(m2.has_value());
    EXPECT_EQ(*m2, "msg_b");

    auto m3 = poll();
    ASSERT_TRUE(m3.has_value());
    EXPECT_EQ(*m3, "msg_c");

    client->shutdown();
    server->shutdown();
}


TEST(ShmCommunicationTest, BroadcastSendsToAllSlots) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    auto client0 = werewolf::make_client_shm_communication(name);
    auto client1 = werewolf::make_client_shm_communication(name);

    ASSERT_TRUE(server->initialize(2));
    ASSERT_TRUE(client0->initialize(0));
    ASSERT_TRUE(client1->initialize(1));

    server->broadcast("hello all", {0, 1});

    std::optional<std::string> msg0, msg1;
    for (int i = 0; i < 20; ++i) {
        if (!msg0) msg0 = client0->recv();
        if (!msg1) msg1 = client1->recv();
        if (msg0 && msg1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(msg0.has_value());
    EXPECT_EQ(*msg0, "hello all");

    ASSERT_TRUE(msg1.has_value());
    EXPECT_EQ(*msg1, "hello all");

    client0->shutdown();
    client1->shutdown();
    server->shutdown();
}


TEST(ShmCommunicationTest, RecvReturnsNulloptWhenEmpty) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    auto client = werewolf::make_client_shm_communication(name);

    ASSERT_TRUE(server->initialize(1));
    ASSERT_TRUE(client->initialize(0));

    EXPECT_FALSE(server->recv(0).has_value());
    EXPECT_FALSE(client->recv().has_value());

    client->shutdown();
    server->shutdown();
}


TEST(ShmCommunicationTest, CrossProcessFork) {
    auto name = unique_shm_name();
    auto server = werewolf::make_server_shm_communication(name, true);
    ASSERT_TRUE(server->initialize(1));

    pid_t pid = fork();
    ASSERT_NE(pid, -1);

    if (pid == 0) {
        // child acts as client
        auto client = werewolf::make_client_shm_communication(name);
        if (!client->initialize(0)) _exit(1);
        if (!client->send("from_child")) _exit(2);
        client->shutdown();
        _exit(0);
    }

    // parent is server
    std::optional<std::string> msg;
    for (int i = 0; i < 100; ++i) {
        msg = server->recv(0);
        if (msg) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    int status = 0;
    waitpid(pid, &status, 0);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(*msg, "from_child");

    server->shutdown();
}

