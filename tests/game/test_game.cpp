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

    testutils::TempDir tmp_dir;
    cfg.game_log = tmp_dir.path() + "/game.log";
    cfg.moderator_log = tmp_dir.path() + "/moderator.log";

    auto fake = std::make_unique<FakeCommunication>(cfg.max_players);
    auto* raw = fake.get();

    // lobby connect
    raw->connect_automatically();

    // night vote
    raw->push_msg(0, "vote: player2");
    raw->push_msg(1, "vote: player2");

    // day vote
    raw->push_msg(0, "vote: player3");
    raw->push_msg(1, "vote: player3");
    raw->push_msg(2, "vote: player3");
    raw->push_msg(3, "vote: player0");

    Game game(std::move(fake), cfg);
    game.run();

    std::string log_content = testutils::ReadFileContents(cfg.game_log);
    EXPECT_THAT(log_content, HasSubstr("Night: player 2 was killed."));
    EXPECT_THAT(log_content, HasSubstr("Day: player 3 was lynched."));
}

} // namespace werewolf::test
