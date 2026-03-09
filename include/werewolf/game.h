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
    };

    // members
    std::unique_ptr<ICommunication> comm_;
    GameConfig cfg_;
    std::atomic<bool> running_{false};

    std::ofstream game_log_;
    std::ofstream moderator_log_;
    std::mutex log_mutex_;

    // player members and methods
    std::vector<Player> players_;
    mutable std::mutex players_mutex_;
    void initialize_players(const std::vector<std::string>& names);
    std::vector<int> alive_slots() const;
    bool mark_connected(int slot);
    std::vector<int> connected_slots() const;
    int connected_player_count() const;
    std::string role_name(Role r) const;
    void assign_roles();

    // phases
    std::vector<std::string> load_names() const;
    std::vector<std::string> load_default_names() const;
    void lobby_phase();

    // private methods
    bool send_to_slot(int slot, const std::string& msg);
    void broadcast_to_slots(const std::string& msg, const std::vector<int>& slots);

    // validation
    bool validate_assign_config(std::vector<int>& slots) ;

    // logging methods
    void log_assigned_slots(std::vector<int>& slots);
    void log(const std::string& msg, bool to_stdout=true, bool to_game_log=true, bool to_moderator_log=true);
};

} // namespace werewolf
