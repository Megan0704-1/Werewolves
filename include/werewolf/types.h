#pragma once

#include <string>

namespace werewolf {

enum class Role { Townperson, Wolf, Witch };
enum class Winner { Village, Wolf, TBD };
enum class VoteStatus { Decided, Tie, NoDecision };

struct VoteResult {
    VoteStatus status = VoteStatus::NoDecision;
    int target = -1;
};

struct GameConfig {
    int  max_players = 16;
    int  lobby_wait_seconds = 1;
    int  vote_duration = 1;
    int  death_speech_seconds = 1;
    int  wolf_count = 2;
    bool has_witch = true;
    bool has_seer = true;
    bool deterministic_assign = false;
    bool deterministic_vote = false;
    bool silent_votes = true;
    bool randomize_names = true;
    std::string names_file = "names.txt";
    std::string game_log = "game.log";
    std::string moderator_log = "moderator.log";
    std::string vote_prefix="vote: ";
    std::string chat_prefix="chat: ";
};

} // namespace werewolf
