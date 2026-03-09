#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "werewolf/game.h"
#include "../utils/temp_dir.h"
#include "../utils/files.h"
#include "fake_communication.h"

namespace werewolf::test {

using ::testing::HasSubstr;

TEST(GameTest, RunInitializesAndShutsDownCommunication) {
    GameConfig cfg;
    cfg.max_players = 6;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>();
    // raw observer
    auto* raw = fake.get();

    // comm ownership transfere to game obj.
    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->last_initialize_slots, 6);
    EXPECT_EQ(raw->shutdown_called, 1);
}

TEST(GameTest, RunReturnsWhenInitializeFails) {
    GameConfig cfg;
    cfg.max_players = 4;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>();
    fake->initialize_result = false; // this makes initialize returns false in fake_communication.h
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 0);
}

TEST(GameTest, RunWithNullCommunicationDoesNotCrash) {
    GameConfig cfg;
    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    Game game(nullptr, cfg);
    EXPECT_NO_THROW(game.run());
}

TEST(GameTest, RunLogsLobbyInitialization) {
    GameConfig cfg;
    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>();
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 1);

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby"));
}

TEST(GameTest, RunLogsLobbyReadyBroadcast) {
    GameConfig cfg;
    cfg.max_players = 6;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>();
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->shutdown_called, 1);

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby Ready"));
    EXPECT_THAT(log_content, HasSubstr("Lobby initialized with 6 players"));
}

TEST(GameTest, RunLobbyLoadNames) {
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

    auto fake = std::make_unique<FakeCommunication>();
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Lobby initialized with 3 players"));
}

TEST(GameTest, RoleAssignmentTest) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.wolf_count = 2;
    cfg.has_witch = true;
    cfg.deterministic = true;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>();
    auto* raw = fake.get();

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



} // namespace werewolf::test
