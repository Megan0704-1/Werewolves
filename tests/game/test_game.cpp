#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "werewolf/game.h"
#include "../utils/temp_dir.h"
#include "../utils/files.h"
#include "fake_communication.h"

namespace werewolf::test {

using ::testing::HasSubstr;

TEST(GameTest, RunProcessesConnectAndVotes) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = false;
    cfg.lobby_wait_seconds = 1;
    cfg.vote_duration = 1;
    cfg.chat_duration = 0;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>(cfg.max_players);
    auto* raw = fake.get();

    // lobby connect
    raw->connect_automatically();

    // wolves, night vote
    raw->push_msg(0, "vote: player2");
    raw->push_msg(1, "vote: player2");

    // day vote
    raw->push_msg(0, "vote: player3");
    raw->push_msg(1, "vote: player3");
    raw->push_msg(2, "vote: player3");
    raw->push_msg(3, "vote: player0");
    raw->push_msg(4, "vote: player3");
    raw->push_msg(5, "vote: player3");

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Day: player3 was lynched."));
}

TEST(GameTest, TestDeadNotes) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = false;
    cfg.lobby_wait_seconds = 1;
    cfg.vote_duration = 1;
    cfg.chat_duration = 0;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>(cfg.max_players);
    auto* raw = fake.get();

    // lobby connect
    raw->connect_automatically();

    // wolves, night vote
    raw->push_msg(0, "vote: player2");
    raw->push_msg(1, "vote: player2");

    // day vote
    raw->push_msg(0, "vote: player3");
    raw->push_msg(1, "vote: player3");
    raw->push_msg(2, "vote: player3");
    raw->push_msg(3, "vote: player0");
    raw->push_msg(4, "vote: player3");
    raw->push_msg(5, "vote: player3");

    // final words from players
    raw->push_msg(2, "I am innocent.");
    raw->push_msg(3, "I am hungry.");

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Day: player3 was lynched."));
    EXPECT_THAT(log_content, HasSubstr("Final words from player2: I am innocent."));
    EXPECT_THAT(log_content, HasSubstr("Final words from player3: I am hungry."));
}

TEST(GameTest, TestChat) {
    GameConfig cfg;
    cfg.max_players = 6;
    cfg.deterministic_assign = true;
    cfg.deterministic_vote = false;
    cfg.lobby_wait_seconds = 1;
    cfg.vote_duration = 1;
    cfg.chat_delay_ms = 1000;

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>(cfg.max_players);
    auto* raw = fake.get();

    // lobby connect
    raw->connect_automatically();

    // wolves, night chat
    raw->push_msg(0, "chat: who to vote?");
    raw->push_msg(1, "chat: player2");

    // wolves, night vote
    raw->push_msg(0, "vote: player2");
    raw->push_msg(1, "vote: player2");
    raw->push_msg(4, "chat: I tried to chat.");

    // death note
    raw->push_msg(2, "happy.");

    // day chat
    raw->push_msg(0, "chat: hey.");
    raw->push_msg(5, "chat: who is wolf?");

    // day vote
    raw->push_msg(0, "vote: player3");
    raw->push_msg(1, "vote: player3");
    raw->push_msg(3, "vote: player0");
    raw->push_msg(4, "vote: player3");
    raw->push_msg(5, "vote: player3");

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("player0: who to vote?"));
    EXPECT_THAT(log_content, HasSubstr("player1: player2"));
    EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Final words from player2: happy."));
    EXPECT_THAT(log_content, HasSubstr("player0: hey."));
    EXPECT_THAT(log_content, HasSubstr("player4: I tried to chat."));
    EXPECT_THAT(log_content, HasSubstr("player5: who is wolf?"));
}

} // namespace werewolf::test
