#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "werewolf/frontends/shm/server_types.h"
#include "werewolf/game.h"

namespace werewolf::frontends {

namespace {
werewolf::Game* game_ref = nullptr;

void SignalHandler(int) {
  if (game_ref) {
    game_ref->request_stop();
  }
}
};  // namespace

void PrintServerUsage(std::string_view bin) {
  std::cerr << "Usage: " << bin << " [OPTIONS]\n"
            << "\n"
            << "Game options:\n"
            << "  --players N        Max player slots\n"
            << "  --wolves N         Number of wolves\n"
            << "  --lobby SEC        Lobby wait time in seconds\n"
            << "  --vote SEC         Vote duration in seconds\n"
            << "  --chat SEC         Chat duration in seconds\n"
            << "  --speech SEC       Death speech duration in seconds\n"
            << "  --witch SEC        Witch decision duration in seconds\n"
            << "  --names FILE       Names file\n"
            << "  --no-randomize     Disable name randomization\n"
            << "  --no-witch         Disable witch role\n"
            << "  --det-assign       Deterministic role assignment\n"
            << "  --det-vote         Deterministic voting\n"
            << "  --game-log         Game log file\n"
            << "  --moderator-log    Moderator log file\n"
            << "\n"
            << "Shared memory backend config:\n"
            << "  --shm-name NAME    shm name\n"
            << "\n"
            << "Misc:\n"
            << "  -h, --help         Show this message\n";
}

ServerOptions ParseServerArgs(int argc, char* argv[]) {
  ServerOptions options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 < argc) return argv[++i];
      std::cerr << "Missing value for " << arg << "\n";
      std::exit(1);
    };

    if (arg == "--players") {
      options.game_cfg.max_players = std::stoi(next());
    } else if (arg == "--wolves") {
      options.game_cfg.wolf_count = std::stoi(next());
    } else if (arg == "--lobby") {
      options.game_cfg.lobby_wait_seconds = std::stoi(next());
    } else if (arg == "--vote") {
      options.game_cfg.vote_duration = std::stoi(next());
    } else if (arg == "--chat") {
      options.game_cfg.chat_duration = std::stoi(next());
    } else if (arg == "--speech") {
      options.game_cfg.death_speech_seconds = std::stoi(next());
    } else if (arg == "--witch") {
      options.game_cfg.witch_decide_seconds = std::stoi(next());
    } else if (arg == "--names") {
      options.game_cfg.names_file = next();
    } else if (arg == "--no-randomize") {
      options.game_cfg.randomize_names = false;
    } else if (arg == "--no-witch") {
      options.game_cfg.has_witch = false;
    } else if (arg == "--det-assign") {
      options.game_cfg.deterministic_assign = true;
    } else if (arg == "--det-vote") {
      options.game_cfg.deterministic_vote = true;
    } else if (arg == "--game-log") {
      options.game_cfg.game_log = next();
    } else if (arg == "--moderator-log") {
      options.game_cfg.moderator_log = next();
    } else if (arg == "--shm-name") {
      options.shm_name = next();
    } else if (arg == "-h" || arg == "--help") {
      options.show_help = true;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      PrintServerUsage(argv[0]);
      std::exit(1);
    }
  }
  return options;
}

int RunServer(const ServerOptions& options,
              const ServerCommunicationFactory& factory) {
  if (options.show_help) {
    return 0;
  }

  auto comm = factory(options);
  if (!comm) {
    std::cerr << "Failed to create communication backends for server: shm "
                 "(shared memory)\n";
    return 1;
  }

  werewolf::Game game(std::move(comm), options.game_cfg);
  game_ref = &game;

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  std::cout << ">>> Werewolf Server Starts <<<" << std::endl;
  std::cout << "backend: shm" << std::endl;

  game.run();
  game_ref = nullptr;
  return 0;
}

};  // namespace werewolf::frontends
