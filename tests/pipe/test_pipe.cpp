#include <gtest/gtest.h>
#include "pipe_communication.h"
#include "../utils/temp_dir.h"
#include <thread>
#include <chrono>

// test init and close
TEST(PipeCommunicationTest, InitializeShutdown) {
    testutils::TempDir tmp;
    auto comm = werewolf::make_pipe_communication(tmp.path(), true);
    ASSERT_TRUE(comm->initialize(2));
    comm->shutdown();
}

// test player -> server
TEST(PipeCommunicationTest, P2S) {
    testutils::TempDir tmp;
    auto comm = werewolf::make_pipe_communication(tmp.path(), true);
    ASSERT_TRUE(comm->initialize(1));

    // slot 0
    std::thread player([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        comm->send_to_server(0, "hello");
    });

    std::optional<std::string> msg;
    // attempt for 10 times
    for(int i=0;i<10;i++) {
        msg = comm->recv_from_player(0);
        if(msg) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg.value(), "hello\n");

    player.join();
    comm->shutdown();
}

// test server -> player
TEST(PipeCommunicationTest, S2P) {
    testutils::TempDir tmp;
    auto comm = werewolf::make_pipe_communication(tmp.path(), true);
    ASSERT_TRUE(comm->initialize(1));

    std::thread server([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        comm->send_to_player(0, "vote");
    });

    std::optional<std::string> msg;
    for(int i=0;i<10;i++) {
        msg = comm->recv_from_server(0);
        if(msg) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg.value(), "vote\n");

    server.join();
    comm->shutdown();
}
