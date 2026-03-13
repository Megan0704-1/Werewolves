#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "werewolf/game.h"
#include "../utils/temp_dir.h"
#include "../utils/files.h"
#include "fake_communication.h"

namespace werewolf::test {

using ::testing::HasSubstr;

TEST(UnitTest, RunInitializesAndShutsDownCommunication) {
    GameConfig cfg;
    cfg.max_players = 6;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    // raw observer
    auto* raw = fake.get();

    // comm ownership transfere to game obj.
    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->last_initialize_slots, 6);
    EXPECT_EQ(raw->shutdown_called, 1);
}

TEST(UnitTest, RunReturnsWhenInitializeFails) {
    GameConfig cfg;
    cfg.max_players = 4;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    fake->initialize_result = false; // this makes initialize returns false in fake_communication.h
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 0);
}

TEST(UnitTest, RunWithNullCommunicationDoesNotCrash) {
    GameConfig cfg;
    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    Game game(nullptr, cfg);
    EXPECT_NO_THROW(game.run());
}

TEST(UnitTest, RunLogsLobbyInitialization) {
    GameConfig cfg;
    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 1);

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby"));
}

TEST(UnitTest, RunLogsLobbyReadyBroadcast) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    raw->connect_automatically();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 1);

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby Ready"));
    EXPECT_THAT(log_content, HasSubstr("Lobby initialized with 6 players"));
}

TEST(UnitTest, RunLobbyLoadNames) {
    GameConfig cfg;
    cfg.max_players = 6;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";
    cfg.names_file = tmp_dir.path() + "/names.txt";

    std::ofstream names_fd(cfg.names_file, std::ios::app);
    names_fd << "alice" << std::endl;
    names_fd << "bob" << std::endl;
    names_fd << "cathy" << std::endl;
    names_fd.close();

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby initialized with 3 players"));
}

TEST(UnitTest, RoleAssignmentTest) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.wolf_count = 2;
    cfg.has_witch = true;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    raw->connect_automatically();

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("player 0 is Wolf"));
    EXPECT_THAT(log_content, HasSubstr("player 1 is Wolf"));
    EXPECT_THAT(log_content, HasSubstr("player 2 is Witch"));
    EXPECT_THAT(log_content, HasSubstr("player 3 is Townperson"));
    EXPECT_THAT(log_content, HasSubstr("player 4 is Townperson"));
    EXPECT_THAT(log_content, HasSubstr("player 5 is Townperson"));
}

TEST(UnitTest, RunDeterministic) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.wolf_count = 2;
    cfg.has_witch = true;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    raw->connect_automatically();

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("=== Round 0 ==="));
    EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Day: player0 was lynched."));
    EXPECT_THAT(log_content, HasSubstr("=== Round 1 ==="));
    EXPECT_THAT(log_content, HasSubstr("Night: player3 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Day: player1 was lynched."));
    EXPECT_THAT(log_content, HasSubstr("--- Village Win ---"));
}

TEST(UnitTest, TestLobbyConnected) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.wolf_count = 2;
    cfg.has_witch = true;
    cfg.deterministic_vote = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    raw->push_msg(0, "connect");
    raw->push_msg(1, "connect");
    raw->push_msg(3, "connect");
    raw->push_msg(4, "connect");

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("player0 is connected."));
    EXPECT_THAT(log_content, HasSubstr("player1 is connected."));
    EXPECT_THAT(log_content, HasSubstr("player3 is connected."));
    EXPECT_THAT(log_content, HasSubstr("player4 is connected."));
}

TEST(UnitTest, BroadcastDelegatesToTransport) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
    auto* raw = fake.get();

    raw->connect_automatically();

    Game game(std::move(fake), cfg);
    game.run();

    // Verify that broadcast was actually called on the transport, not just individual sends
    EXPECT_GT(raw->broadcast_called, 0);
}
} // namespace werewolf::test
