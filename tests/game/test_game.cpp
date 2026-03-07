#include <gtest/gtest.h>

#include "werewolf/game.h"
#include "../utils/temp_dir.h"
#include "fake_communication.h"

namespace werewolf::test {

TEST(GameTest, RunInitializesAndShutsDownCommunication) {
    GameConfig cfg;
    cfg.max_players = 6;

    testutils::TempDir game_log_tmp, moderator_log_tmp;
    cfg.game_log = game_log_tmp.path();
    cfg.moderator_log = moderator_log_tmp.path();

    auto fake = std::make_unique<FakeCommunication>();
    auto* raw = fake.get();

    Game game(std::move(fake), cfg);
    game.run();

    EXPECT_EQ(raw->initialize_called, 1);
    EXPECT_EQ(raw->last_initialize_slots, 6);
    EXPECT_EQ(raw->shutdown_called, 1);
}

} // namespace werewolf::test
