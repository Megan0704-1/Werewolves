#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../utils/files.h"
#include "../utils/temp_dir.h"
#include "fake_communication.h"
#include "werewolf/game.h"

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

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
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
  cfg.witch_decide_seconds = 0;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  // lobby connect
  raw->connect_automatically();

  // wolves, night vote
  raw->push_msg(0, "vote: player2");
  raw->push_msg(1, "vote: player2");

  // day vote
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");
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
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player2: I am innocent."));
  EXPECT_THAT(log_content, HasSubstr("Final words from player3: I am hungry."));
}

TEST(GameTest, TestChat) {
  GameConfig cfg;
  cfg.max_players = 6;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.delay_ms = 1000;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
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

// witch heals
TEST(GameTest, WitchCanHealNightVictim) {
  GameConfig cfg;
  cfg.max_players = 6;
  cfg.wolf_count = 2;
  cfg.has_witch = true;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.chat_duration = 0;
  cfg.death_speech_seconds = 1;
  cfg.witch_decide_seconds = 1;
  cfg.delay_ms = 1000;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  // wolves vote player3
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch heals
  raw->push_msg(2, "heal");

  // day vote, make game continue deterministically enough
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  raw->push_msg(2, "vote: player4");
  raw->push_msg(3, "vote: player0");
  raw->push_msg(4, "vote: player0");
  raw->push_msg(5, "vote: player4");

  // wolves vote player3
  raw->push_msg(0, "vote: player2");
  raw->push_msg(1, "vote: player2");

  Game game(std::move(fake), cfg);
  game.run();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Witch healed player3"));
  EXPECT_THAT(log_content,
              ::testing::Not(HasSubstr("Night: player3 was killed.")));
}

// witch skips, victim dies normally
TEST(GameTest, WitchSkipKeepsWolfVictimDead) {
  GameConfig cfg;
  cfg.max_players = 6;
  cfg.wolf_count = 2;
  cfg.has_witch = true;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.death_speech_seconds = 1;
  cfg.chat_duration = 0;
  cfg.delay_ms = 1000;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  // wolves vote player3
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch skips
  raw->push_msg(2, "skip");

  // player3 final words
  raw->push_msg(3, "I am not a wolf.");

  // day vote
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  raw->push_msg(2, "vote: player4");
  raw->push_msg(4, "vote: player0");
  raw->push_msg(5, "vote: player4");

  Game game(std::move(fake), cfg);
  game.run();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Night: player3 was killed."));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player3: I am not a wolf."));
}

// witch poisons someone, original victim also dies
TEST(GameTest, WitchPoisonKillsPoisonTargetAndKeepsWolfVictimDead) {
  GameConfig cfg;
  cfg.max_players = 6;
  cfg.wolf_count = 2;
  cfg.has_witch = true;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.death_speech_seconds = 1;
  cfg.chat_duration = 0;
  cfg.witch_decide_seconds = 1;
  cfg.delay_ms = 1000;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  // wolves vote player3
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch poisons player4
  raw->push_msg(2, "poison: player4");

  // final words
  raw->push_msg(3, "goodbye from 3");
  raw->push_msg(4, "goodbye from 4");

  // day vote after night double death
  raw->push_msg(0, "vote: player5");
  raw->push_msg(1, "vote: player5");
  raw->push_msg(2, "vote: player5");
  raw->push_msg(5, "vote: player0");

  Game game(std::move(fake), cfg);
  game.run();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Night: player3 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Night: player4 was poisoned."));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player3: goodbye from 3"));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player4: goodbye from 4"));
}

}  // namespace werewolf::test
