#pragma once

#include <string>

namespace werewolf {

enum class Role { Townperson, Wolf, Witch };
enum class Winner { Village, Wolf, TBD };
enum class VoteStatus { Decided, Tie, NoDecision };

struct WitchAction {
  enum class Magic { Healed, Poisoned, Skip };
  Magic magic = Magic::Skip;
  int poison_target = -1;
  WitchAction() = default;
};

struct VoteResult {
  VoteStatus status = VoteStatus::NoDecision;
  int target = -1;
};

struct Special {
  int heal_power = 0;
  int poison_power = 0;
  Special() = default;
  Special(int heal, int poison) : heal_power(heal), poison_power(poison) {}
};

struct GameConfig {
  int max_players = 16;
  int lobby_wait_seconds = 1;
  int vote_duration = 1;
  int chat_duration = 1;
  int death_speech_seconds = 1;
  int wolf_count = 2;
  bool has_witch = true;
  int witch_decide_seconds = 0;
  int heal_power = 1;
  int poison_power = 1;

  bool has_seer = true;
  bool deterministic_assign = false;
  bool deterministic_vote = false;
  bool silent_votes = true;
  bool randomize_names = true;
  std::string names_file = "names.txt";
  std::string game_log = "game.log";
  std::string moderator_log = "moderator.log";
  std::string vote_prefix = "vote: ";
  std::string chat_prefix = "chat: ";
  std::string poison_prefix = "poison: ";

  // Current implementation assumes phase-separated input:
  // chat messages arrive during chat_phase, vote messages during vote
  // collection. Out-of-phase messages are not buffered.
  int delay_ms = 100;
};

}  // namespace werewolf
