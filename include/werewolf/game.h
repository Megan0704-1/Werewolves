#pragma once

/// game.h

#include "werewolf/communication.h"
#include "werewolf/types.h"
#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace werewolf {

class Game {
public:
    explicit Game(std::unique_ptr<ICommunication> comm, GameConfig cfg = {});
    ~Game();

    void run();
    void request_stop();

private:
    // nested scope guard
    // This guard ensures proper shutdown when un-expected or abnormal returns before scope ends.
    struct CleanupGuard {
        Game& game;
        bool is_initialized = false;

        CleanupGuard(Game& game);
        ~CleanupGuard();
    };

    // player state
    struct Player {
        int slot = -1;
        std::string name;
        Role role = Role::Townperson;
        bool is_connected = false;
        bool is_alive = true;
        Special power = Special();
    };

    // members
    std::unique_ptr<ICommunication> comm_;
    GameConfig cfg_;
    std::atomic<bool> running_{false};
    int round_cnt = 0;

    std::ofstream game_log_;
    std::ofstream moderator_log_;
    std::mutex log_mutex_;

    // player members and methods
    std::vector<Player> players_;
    mutable std::mutex players_mutex_;
    void initialize_players(const std::vector<std::string>& names);

    // query
    std::optional<Player> get_player_info(int slot) const;
    std::optional<Player> get_player_info(const std::string& name) const;
    std::vector<int> alive_slots() const;
    std::vector<int> connected_slots() const;
    int connected_player_count() const;
    std::vector<int> alive_slots_with_role(Role r) const;

    // mutate
    bool mark_connected(int slot);
    void assign_roles();
    void notify_roles();
    bool kill_player(int slot);
    void handle_night_vote_result(const WitchAction& m, const VoteResult& r);
    void handle_day_vote_result(const VoteResult& r);
    void connect_lobby_players();

    // helper
    std::string role_name(Role r) const;
    VoteResult conduct_night_vote();
    VoteResult conduct_day_vote();
    WitchAction witch_magic_power(const VoteResult result);
    VoteResult choose_night_victim(const std::vector<int>& voters, const std::vector<int>& slots) ;
    VoteResult choose_day_target(const std::vector<int>& voters, const std::vector<int>& slots) ;
    VoteResult collect_votes(const std::vector<int>& voters, const std::vector<int>& cands, int duration);

    // phases
    std::vector<std::string> load_names() const;
    std::vector<std::string> load_default_names() const;
    void lobby_phase();
    void night_phase();
    void day_phase();
    void dead_phase(int slot);
    void chat_phase(const std::vector<int>& slots);

    // end rule
    Winner check_win() const;

    // private methods
    bool send_to_slot(int slot, const std::string& msg);
    void broadcast_to_slots(const std::string& msg, const std::vector<int>& slots);
    std::optional<std::string> recv_from_slot(int slot);
    void announce_death(int slot, const std::string& when);

    // validation
    bool valid_assign_config(std::vector<int>& slots) ;
    bool valid_player_numbers() ;

    // logging methods
    void log_assigned_slots(std::vector<int>& slots);
    void log_winner(Winner winner);
    void log_rounds();
    void log(const std::string& msg, bool to_stdout=true, bool to_game_log=true, bool to_moderator_log=true);
};

} // namespace werewolf
