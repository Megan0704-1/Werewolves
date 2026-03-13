#include "werewolf/frontends/pipes/client_types.h"
#include <atomic>
#include <mutex>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <poll.h>
#include <unistd.h>

namespace werewolf::frontends {

namespace {
std::atomic<bool> running_ref{true};
std::mutex io_mutex;

void SignalHandler(int) {
    running_ref.store(false);
}
} // namespace

void PrintClientUsage(std::string_view bin) {
    std::lock_guard<std::mutex> lock(io_mutex);
    std::cerr
        << "Usage: " << bin << " <slot-number> [OPTIONS]\n"
        << "\n"
        << "Options:\n"
        << "  --pipe-root DIR    FIFO root directory\n"
        << "  -h, --help         Show this message\n";
}

ClientOptions ParseClientArgs(int argc, char* argv[]) {
    ClientOptions options;

    if (argc < 2) {
        PrintClientUsage(argv[0]);
        std::exit(1);
    }

    options.slot = std::stoi(argv[1]);

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 < argc) return argv[++i];
            std::cerr << "Missing value for " << arg << "\n";
            std::exit(1);
        };

        if (arg == "--pipe-root") {
            options.pipe_root = next();
        } else if (arg == "-h" || arg == "--help") {
            options.show_help = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintClientUsage(argv[0]);
            std::exit(1);
        }
    }

    return options;
}

// client runs
// 1. First sent connect request to the server.
// 2. (if successfully connected) launch a listener thread, keep pooling msg from server. (shutdown communication if game closed.)
// 3. another thread getline from the client, send to server if any.
int RunClient(const ClientOptions& options, const ClientCommunicationFactory& factory) {
    running_ref.store(true);
    if(options.show_help) return 0;

    if(options.slot < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Invalid slot number\n";
        return 1;
    }

    auto comm = factory(options);
    if(!comm) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Failed to create communication backend for client: pipe\n";
        return 1;
    }

    if(!comm->initialize(options.slot)) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Failed to initialize communication\n";
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::mutex send_mutex, recv_mutex;

    {
        const std::string connect_req = "connect";
        std::lock_guard<std::mutex> lock(send_mutex);
        comm->send(connect_req);
    }

    std::thread listener([&]() {
            while(running_ref.load()) {
                std::optional<std::string> msg;
                {
                    std::lock_guard<std::mutex> lock(recv_mutex);
                    msg = comm->recv();
                } // end if

                if(!msg) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                } // end if

                if(*msg == "close") {
                    {
                        std::lock_guard<std::mutex> lock(io_mutex);
                        std::cout << "Connection closed" << std::endl;
                    }
                    running_ref.store(false);
                    return;
                } // end if
               
                {
                    std::lock_guard<std::mutex> lock(io_mutex);
                    std::cout << *msg << std::endl;
                } 
            } // end while
    });

    std::string line;
    while(running_ref.load()) {
        struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
        // waiting for keyboard input for 100ms
        int ret = poll(&pfd, 1, 100);

        if(ret < 0 && errno != EINTR) {
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                std::cerr << "poll(stdin) failed\n";
            }
            break;
        }

        if(ret > 0 && (pfd.revents & (POLLIN | POLLHUP))) {
            if(!std::getline(std::cin, line)) break;
            if(!running_ref.load()) break;
            {
                std::lock_guard<std::mutex> lock(send_mutex);
                comm->send(line);
            }
        }
        
    }

    running_ref.store(false);
    if(listener.joinable()) listener.join();
    comm->shutdown();
    return 0;
}

} // namespace werewolf::frontends
