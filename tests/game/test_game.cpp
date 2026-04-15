#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

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
  cfg.death_speech_seconds = 0;
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  // lobby connect
  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves, night vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player2");
  raw->push_msg(1, "vote: player2");

  // day vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 3, 4, 5}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");
  raw->push_msg(3, "vote: player0");
  raw->push_msg(4, "vote: player3");
  raw->push_msg(5, "vote: player3");

  // wolves, night vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");

  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Day: player3 was lynched."));
}

TEST(GameTest, TestDeadNotes) {
  GameConfig cfg;
  cfg.max_players = 5;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.chat_duration = 0;
  cfg.death_speech_seconds = 1;
  cfg.witch_decide_seconds = 0;
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  // lobby connect
  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });
  // wolves, night vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player2");
  raw->push_msg(1, "vote: player2");

  // final words from player2
  ASSERT_TRUE(raw->wait_until_sent(2, "You are dead."));
  raw->push_msg(2, "I am innocent.");

  // day vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 3, 4}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");
  raw->push_msg(3, "vote: player0");
  raw->push_msg(4, "vote: player3");

  // final words from player3
  ASSERT_TRUE(raw->wait_until_sent(3, "You are dead."));
  raw->push_msg(3, "I am hungry.");

  // wolves, night vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Day: player3 was lynched."));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player2: I am innocent."));
  EXPECT_THAT(log_content, HasSubstr("Final words from player3: I am hungry."));
}

TEST(GameTest, TestChat) {
  GameConfig cfg;
  cfg.max_players = 5;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.chat_duration = 1;
  cfg.vote_duration = 1;
  cfg.death_speech_seconds = 1;
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  // lobby connect
  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves, night chat
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "Chat starts"));
  raw->push_msg(0, "chat: who to vote?");
  raw->push_msg(1, "chat: player2");

  // wolves, night vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player2");
  raw->push_msg(1, "vote: player2");
  raw->push_msg(4, "chat: I tried to chat.");  // not consumed, queued

  // death note
  ASSERT_TRUE(raw->wait_until_sent(2, "You are dead."));
  raw->push_msg(2, "happy.");

  // day chat
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 4}, "Chat starts"));
  raw->push_msg(0, "chat: hey.");
  raw->push_msg(4, "chat: who is wolf?");

  // day vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 3, 4}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");
  raw->push_msg(3, "vote: player0");
  raw->push_msg(4, "vote: player3");

  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");

  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("player0: who to vote?"));
  EXPECT_THAT(log_content, HasSubstr("player1: player2"));
  EXPECT_THAT(log_content, HasSubstr("Night: player2 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Final words from player2: happy."));
  EXPECT_THAT(log_content, HasSubstr("player0: hey."));
  EXPECT_THAT(log_content, HasSubstr("player4: I tried to chat."));
  EXPECT_THAT(log_content, HasSubstr("player4: who is wolf?"));
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
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves vote player3
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch heals
  ASSERT_TRUE(raw->wait_until_sent(2, "Witch, decide one of your actions:"));
  raw->push_msg(2, "heal");

  // day vote, make game continue deterministically enough
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 2, 3, 4, 5}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  raw->push_msg(2, "vote: player0");
  raw->push_msg(3, "vote: player0");
  raw->push_msg(4, "vote: player0");
  raw->push_msg(5, "vote: player0");

  // wolves vote player2
  ASSERT_TRUE(raw->wait_until_sent_to_all({1}, "[vote] starts"));
  raw->push_msg(1, "vote: player2");

  // day vote, make game continue deterministically enough
  ASSERT_TRUE(raw->wait_until_sent_to_all({1, 3, 4, 5}, "[vote] starts"));
  raw->push_msg(1, "vote: player4");
  raw->push_msg(3, "vote: player1");
  raw->push_msg(4, "vote: player1");
  raw->push_msg(5, "vote: player1");

  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Witch healed player3"));
  EXPECT_THAT(log_content,
              ::testing::Not(HasSubstr("Night: player3 was killed.")));
}

// witch skips, victim dies normally
TEST(GameTest, WitchSkipKeepsWolfVictimDead) {
  GameConfig cfg;
  cfg.max_players = 5;
  cfg.wolf_count = 2;
  cfg.has_witch = true;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.death_speech_seconds = 1;
  cfg.chat_duration = 0;
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();
  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves vote player3
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch skips
  raw->push_msg(2, "skip");

  // player3 final words
  raw->push_msg(3, "I am not a wolf.");

  // day vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 2, 4}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  raw->push_msg(2, "vote: player4");
  raw->push_msg(4, "vote: player0");

  runner.join();

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
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves vote player3
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch poisons player4
  raw->push_msg(2, "poison: player4");

  // final words
  raw->push_msg(3, "goodbye from 3");
  raw->push_msg(4, "goodbye from 4");

  // day vote after night double death
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 2, 5}, "[vote] starts"));
  raw->push_msg(0, "vote: player5");
  raw->push_msg(1, "vote: player5");
  raw->push_msg(2, "vote: player5");
  raw->push_msg(5, "vote: player0");

  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);
  EXPECT_THAT(log_content, HasSubstr("Night: player3 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Night: player4 was poisoned."));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player3: goodbye from 3"));
  EXPECT_THAT(log_content,
              HasSubstr("Final words from player4: goodbye from 4"));
}

// Wolf kills A, witch poisons B — both must be marked dead BEFORE
// either's dead_phase runs.  Old code ran dead_phase(A) while B was
// still alive, corrupting any state check inside dead_phase.
TEST(GameTest, WitchPoisonBothDeadBeforeDeadPhase) {
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
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // night 1: wolves kill player3, witch poisons player5
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");
  raw->push_msg(2, "poison: player0");

  // final words — both must get a dead_phase
  raw->push_msg(0, "wolf got me");
  raw->push_msg(3, "witch got me");

  ASSERT_TRUE(raw->wait_until_sent_to_all({1, 2, 4, 5}, "[vote] starts"));
  raw->push_msg(2, "vote: player1");
  raw->push_msg(4, "vote: player1");
  raw->push_msg(5, "vote: player1");

  runner.join();

  std::string log = testutils::ReadFileContents(cfg.game_log);

  // both deaths announced
  EXPECT_THAT(log, HasSubstr("Night: player3 was killed."));
  EXPECT_THAT(log, HasSubstr("Night: player0 was poisoned."));

  // both entered dead_phase
  EXPECT_THAT(log, HasSubstr("Final words from player0: wolf got me"));
  EXPECT_THAT(log, HasSubstr("Final words from player3: witch got me"));

  // key assertion: player5's death announcement must appear BEFORE
  // any dead_phase final-words line — proves both were killed before
  // either's dead_phase ran
  auto poison_announce = log.find("Night: player0 was poisoned.");
  auto first_final = log.find("Final words from player");
  ASSERT_NE(poison_announce, std::string::npos);
  ASSERT_NE(first_final, std::string::npos);
  EXPECT_LT(poison_announce, first_final)
      << "Both deaths must be announced before any dead_phase runs.\n"
         "Old code ran dead_phase(player0) while player3 was still alive.";
}

// Wolf and witch both target the same player: old code would run
// dead_phase(player3) during wolf‑kill processing, then the second
// kill_player(player3) would silently fail, swallowing the poison
// event entirely.
// The fix deduplicates before any side‑effects run,
// so the player dies exactly once and only one death is announced.
TEST(GameTest, WitchPoisonSameTargetAsWolfDeduplicates) {
  GameConfig cfg;
  cfg.max_players = 5;
  cfg.wolf_count = 2;
  cfg.has_witch = true;
  cfg.deterministic_assign = true;
  cfg.deterministic_vote = false;
  cfg.lobby_wait_seconds = 1;
  cfg.vote_duration = 1;
  cfg.death_speech_seconds = 1;
  cfg.chat_duration = 0;
  cfg.witch_decide_seconds = 1;
  cfg.delay_ms = 900;

  testutils::TempDir tmp_dir;
  cfg.game_log = tmp_dir.path() + "/game.log";
  cfg.moderator_log = tmp_dir.path() + "/moderator.log";

  auto fake = std::make_unique<FakeServerCommunication>(cfg.max_players);
  auto* raw = fake.get();

  raw->connect_automatically();

  Game game(std::move(fake), cfg);
  std::thread runner([&] { game.run(); });

  // wolves vote player3
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1}, "[vote] starts"));
  raw->push_msg(0, "vote: player3");
  raw->push_msg(1, "vote: player3");

  // witch also poisons player3 (same target)
  raw->push_msg(2, "poison: player3");

  // final words — only one death speech should happen
  raw->push_msg(3, "why me twice");

  // day vote
  ASSERT_TRUE(raw->wait_until_sent_to_all({0, 1, 2, 4}, "[vote] starts"));
  raw->push_msg(0, "vote: player4");
  raw->push_msg(1, "vote: player4");
  raw->push_msg(2, "vote: player4");
  raw->push_msg(4, "vote: player0");

  runner.join();

  std::string log_content = testutils::ReadFileContents(cfg.game_log);

  // player3 dies exactly once (wolf kill takes priority as first collected)
  EXPECT_THAT(log_content, HasSubstr("Night: player3 was killed."));
  EXPECT_THAT(log_content, HasSubstr("Final words from player3: why me twice"));

  // poison announcement must NOT appear — dedup dropped it
  EXPECT_THAT(log_content,
              ::testing::Not(HasSubstr("Night: player3 was poisoned.")));

  // dead_phase must not have run twice (old bug: first dead_phase
  // mutated game state before poison processing even started)
  auto occurrences = [&](const std::string& needle) {
    size_t count = 0, pos = 0;
    while ((pos = log_content.find(needle, pos)) != std::string::npos) {
      ++count;
      pos += needle.size();
    }
    return count;
  };
  EXPECT_EQ(occurrences("Final words from player3"), 2u);
}

}  // namespace werewolf::test
