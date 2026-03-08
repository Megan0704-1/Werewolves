#include "werewolf/game.h"

#include <ctime>
#include <iostream>
#include <utility>

namespace werewolf {

// cleanup scope guard
Game::CleanupGuard::CleanupGuard(Game& game): game(game) {}  

Game::CleanupGuard::~CleanupGuard() {
    game.running_.store(false);
    if(is_initialized && game.comm_) {
        game.comm_->shutdown();
    }
    if(game.game_log_.is_open()) {
        game.game_log_.flush();
    }
    if(game.moderator_log_.is_open()) {
        game.moderator_log_.flush();
    }
}

// ctor & dtor
Game::Game(std::unique_ptr<ICommunication> comm, GameConfig cfg): comm_(std::move(comm)), cfg_(std::move(cfg)) {}

Game::~Game() {
    request_stop();
}

// methods
void Game::request_stop() {
    running_.store(false);
}

void Game::log(const std::string& msg, bool to_stdout, bool to_game_log, bool to_moderator_log) {
    const auto now = std::time(nullptr);
    const std::string stamped = "(" + std::to_string(now) + "): " +  msg;

    std::lock_guard<std::mutex> lock(log_mutex_);

    if(to_stdout) {
        std::cout << stamped << std::endl;
    }
    if(to_game_log && game_log_.is_open()) {
        game_log_ << stamped << '\n';
        game_log_.flush();
    }
    if(to_moderator_log && moderator_log_.is_open()) {
        moderator_log_ << stamped << '\n';
        moderator_log_.flush();
    }
}

// responsible for acquire and release comm object.
void Game::run() {
    running_.store(true);

    // handle game state and communication object lifecycle automatically
    CleanupGuard scope_guard(*this);

    game_log_.open(cfg_.game_log, std::ios::app);
    moderator_log_.open(cfg_.moderator_log, std::ios::app);

    if(!comm_) {
        std::cerr << "Game has no communication backend.\n";
        return;
    }

    if(!comm_->initialize(cfg_.max_players)) {
        std::cerr << "Failed to initialize communication.\n";
        return;
    }
    scope_guard.is_initialized = true;

    log(">>> Werewolf game starting <<<");

    log(">>> Werewolf game ended <<<");
}

} // namespace werewolf
