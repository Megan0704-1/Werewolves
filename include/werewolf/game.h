#pragma once

/// game.h
///

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

    // members
    std::unique_ptr<ICommunication> comm_;
    GameConfig cfg_;
    std::atomic<bool> running_{false};

    std::ofstream game_log_;
    std::ofstream moderator_log_;
    std::mutex log_mutex_;

    void log(const std::string& msg, bool to_stdout=true, bool to_game_log=true, bool to_moderator_log=true);

};

} // namespace werewolf
