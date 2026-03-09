#pragma once

#include <string>

namespace werewolf {
enum class Role { Townperson, Wolf, Witch };

struct GameConfig {
    int  max_players = 16;
    int  lobby_wait_seconds = 30;
    int  vote_duration = 30;
    int  death_speech_seconds = 15;
    int  wolf_count = 2;
    bool has_witch = true;
    bool has_seer = true;
    bool deterministic = false;
    bool silent_votes = true;
    bool randomize_names = true;
    std::string names_file = "names.txt";
    std::string game_log = "game.log";
    std::string moderator_log = "moderator.log";
};

} // namespace werewolf
