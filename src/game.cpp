#include "werewolf/game.h"

#include <ctime>
#include <iostream>
#include <utility>

namespace werewolf {

Game::Game(std::unique_ptr<ICommunication> comm, GameConfig cfg): comm_(std::move(comm)), cfg_(std::move(cfg)) {}

Game::~Game() {
    request_stop();
}

void Game::request_stop() {
    running_.store(false);
}

void Game::log(const std::string& msg, bool to_stdout, bool to_all, bool to_moderator) {
    const auto now = std::time(nullptr);
    const std::string stamped = "(" + std::to_string(now) + "): " +  msg;

    std::lock_guard<std::mutex> lock(log_mutex_);

    if(to_stdout) {
        std::cout << stamped << std::endl;
    }
    if(to_all && game_log_.is_open()) {
        game_log_ << stamped << '\n';
        game_log_.flush();
    }
    if(to_moderator && moderator_log_.is_open()) {
        moderator_log_ << stamped << '\n';
        moderator_log_.flush();
    }
}

// responsible for acquire and release comm object.
void Game::run() {
    running_.store(true);

    game_log_.open(cfg_.game_log, std::ios::app);
    moderator_log_.open(cfg_.moderator_log, std::ios::app);

    if(!comm_) {
        std::cerr << "Game has no communication backend.\n";
        running_.store(false);
        return;
    }

    if(!comm_->initialize(cfg_.max_players)) {
        std::cerr << "Failed to initialize communication.\n";
        running_.store(false);
        return;
    }

    log(">>> Werewolf game starting <<<");

    comm_->shutdown();
    log(">>> Werewolf game ended <<<");
    running_.store(false);
}

} // namespace werewolf
