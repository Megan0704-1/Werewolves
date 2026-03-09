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

// initialize_players:
// instantiate player object by @p names, and store to players_ list.
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

// alive_slots:
// return list of alive slots.
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

// mark_connected:
// mark the given slot as connected.
bool Game::mark_connected(int slot) {
    std::lock_guard<std::mutex> lock(players_mutex_);

    if(slot < 0 || slot >= static_cast<int>(players_.size())) {
        return false;
    }
    if(!players_[slot].is_alive) {
        return false;
    }
    players_[slot].is_connected=true;
    return true;
}

// connected_slots:
// returns a list of connected slots.
std::vector<int> Game::connected_slots() const {
    std::lock_guard<std::mutex> lock(players_mutex_);

    std::vector<int> slots;
    slots.reserve(players_.size());
    for(const Player& p : players_) {
        if(p.is_connected && p.slot >= 0) {
            slots.push_back(p.slot);
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

std::string Game::role_name(Role r) const {
    switch(r) {
        case Role::Townperson: return "Townperson";
        case Role::Witch: return "Witch";
        case Role::Wolf: return "Wolf";
    }
    return "Unknown";
}

// assign_roles:
// if deterministic flag is set in the config, we apply the following
// roles to the role assignment:
// - assign # of wolves
// - assign witch
// rest are townperson
// if stochastic: all roles are assigned randomly.
void Game::assign_roles() {
    std::vector<int> slots = connected_slots();
    if(slots.empty()) return;
    if(!validate_assign_config(slots)) return;

    {
        std::lock_guard<std::mutex> lock(players_mutex_); // connected slot also acquire lock
        if(cfg_.deterministic) {
            // clear all role assignments
            for(auto s : slots) {
                players_[s].role = Role::Townperson;
            }
            // assign wolves
            int i=0;
            for(; i<cfg_.wolf_count; ++i) {
                players_[slots[i]].role = Role::Wolf;
            }
            // assign witch
            if(cfg_.has_witch) {
                players_[slots[i]].role = Role::Witch;
            }
        }
    }

    log_assigned_slots(slots);
}

// load_names:
// If cfg_.names_file is readable, load non-empty trimmed lines from it.
// The returned list is not padded to cfg_.max_players.
// If the file cannot be opened, fall back to load_default_names().
std::vector<std::string> Game::load_names() const {
    std::vector<std::string> names;
    names.reserve(cfg_.max_players);
    std::ifstream fd(cfg_.names_file);
    if(!fd) {
        return load_default_names();
    }

    std::string line;
    while(getline(fd, line)) {
        // trim trailing whitespaces
        while(!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if(!line.empty()) names.push_back(line);
    }

    return names;
}

// load_default_names:
// The returne names list has size cfg_.max_players
std::vector<std::string> Game::load_default_names() const {
    std::vector<std::string> names;
    names.reserve(cfg_.max_players);

    for(int i=0; i<cfg_.max_players; ++i) {
        names.push_back("player" + std::to_string(i));
    }
    return names;
}

// private methods
bool Game::send_to_slot(int slot, const std::string& msg) {
    if(!comm_) {
        log("send_to_slot failed: no communication backend", true, true, true);
        return false;
    }

    if(!comm_->send_to_player(slot, msg)) {
        log("send_to_slot failed for slot " + std::to_string(slot), true, true, true);
        return false;
    }
    log("Game sent message to player " + std::to_string(slot) + ": " + msg);
    return true;
}

void Game::broadcast_to_slots(const std::string& msg, const std::vector<int>& slots) {
    for(int slot : slots) {
        send_to_slot(slot, msg);
    }
}

// phase: players waiting in the lobby
void Game::lobby_phase() {
    auto names = load_names();
    initialize_players(names);
    log("Lobby initialized with " + std::to_string(names.size()) + " players.");

    // tmp
    for(int i=0; i<players_.size(); ++i) {
        mark_connected(i);
    }
    broadcast_to_slots("Lobby Ready", connected_slots());
    assign_roles();
}

// methods
bool Game::validate_assign_config(std::vector<int>& slots) {
    if(slots.empty()) return false;
    int n = static_cast<int>(slots.size());
    if(n < cfg_.wolf_count) {
        log("Not enough players to play wolves.");
        return false;
    }
    if(n < cfg_.wolf_count + int(cfg_.has_witch)) {
        log("Not enough players to play witch.");
        return false;
    }
    return true;
}

void Game::log_assigned_slots(std::vector<int>& slots) {
    // log
    for(int s : slots) {
        Player p = players_[s];
        log("player " + std::to_string(p.slot) + " is " + role_name(p.role));
    }
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


} // namespace werewolf
