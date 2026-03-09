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

// public APIs

void Game::request_stop() {
    running_.store(false);
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

    lobby_phase();

    log(">>> Werewolf game ended <<<");
}

// player methods
void Game::initialize_players(const std::vector<std::string>& names) {
    // TODO: role assignment, connection status, liveness.
    std::lock_guard<std::mutex> lock(players_mutex_);

    players_.clear();
    players_.reserve(names.size());

    for(int i=0; i<static_cast<int>(names.size()); ++i) {
        Player p;
        p.slot = i;
        p.name = names[i];
        players_.push_back(std::move(p));
    }
}

std::vector<int> Game::alive_slots() const {
    std::lock_guard<std::mutex> lock(players_mutex_);
    std::vector<int> slots;
    for(const Player& player : players_) {
        if(player.is_connected && player.is_alive && player.slot >=0) {
            slots.push_back(player.slot);
        }
    }
    return slots;
}

int Game::connected_player_count() const {
    std::lock_guard<std::mutex> lock(players_mutex_);

    int cnt = 0;
    for(const Player& p : players_) {
        if(p.is_connected) cnt++;
    }
    return cnt;
}

std::vector<std::string> Game::load_default_names() const {
    std::vector<std::string> names;
    names.reserve(cfg_.max_players);

    for(int i=0; i<cfg_.max_players; ++i) {
        names.push_back("player" + std::to_string(i));
    }
    return names;
}

// phase: players waiting in the lobby
void Game::lobby_phase() {
    auto names = load_default_names();
    initialize_players(names);
    log("Lobby initialized with " + std::to_string(names.size()) + " players.");
}

// methods
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


} // namespace werewolf
