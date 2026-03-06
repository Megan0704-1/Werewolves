#include "pipe_communication.h"

#include <filesystem>
#include <string>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace werewolf {

// helpers for creating named-pipes
static bool SlotValid(int slot, int num_slots) {
    return slot >= 0 && slot < num_slots;
}

static std::string GetP2SFIFODir(const std::string& root, int slot) {
    return root + std::to_string(slot) + "ToServerD";
}

static std::string GetS2PFIFODir(const std::string& root, int slot) {
    return root + "ServerTo" + std::to_string(slot) + "D";
}

static std::string GetP2SFIFOPath(const std::string& root, int slot) {
    return GetP2SFIFODir(root, slot) + "/player_to_server_fifo";
}

static std::string GetS2PFIFOPath(const std::string& root, int slot) {
    return GetS2PFIFODir(root, slot) + "/server_to_player_fifo";
}

// ctor & dtor
PipeCommunication::PipeCommunication(std::string pipe_root_dir, bool create_fifos) : pipe_root_dir_(pipe_root_dir), create_fifos_(create_fifos) {
    // make sure root dir ends with back slash
    if(!pipe_root_dir_.empty() && pipe_root_dir_.back() != '/') {
        pipe_root_dir_ += '/';
    }
}

PipeCommunication::~PipeCommunication() {
    shutdown();
}

std::unique_ptr<ICommunication> make_pipe_communication(const std::string& pipe_root_dir, bool create_fifos) {
    return std::make_unique<PipeCommunication>(pipe_root_dir, create_fifos);
}
// private methods impl

/* w_fifo
 * behavior: lock mutex -> write fifo -> flush -> return  
 * ensure: 2 threads don't interleave writes.
 */
bool PipeCommunication::w_fifo(const int fd, const std::string& msg, std::mutex& mu) {
    if(!alive_) return false;
    std::lock_guard lock(mu); // handle mu, automatic unlock when scope exit.
    std::string payload = msg + '\n';
    ssize_t n = write(fd, payload.data(), payload.size());
    return n == static_cast<ssize_t>(payload.size());
}

/* r_fifo
 * behavior: open fifo -> read fifo to msg -> return msg 
 * note: no lock is needed for reads.
 */
std::optional<std::string> PipeCommunication::r_fifo(const int fd) {
    if(!alive_) return std::nullopt;
    char buf[1024];

    ssize_t n = read(fd, buf, sizeof(buf));
    if(n <= 0) return std::nullopt;

    return std::string(buf, n);
}

// public methods impl

bool PipeCommunication::initialize(int num_slots) {
    num_slots_ = num_slots;

    // resize states
    p2s_fifo_.resize(num_slots_);
    s2p_fifo_.resize(num_slots_);
    p2s_fd_.resize(num_slots_);
    s2p_fd_.resize(num_slots_);
    p2s_mutex_.resize(num_slots_);
    p2s_mutex_.resize(num_slots_);
    s2p_mutex_.resize(num_slots_);

    // loop through entries and instantiate objects
    for(int i=0; i<num_slots_; ++i) {
        p2s_fifo_[i] = GetP2SFIFOPath(pipe_root_dir_, i);
        s2p_fifo_[i] = GetS2PFIFOPath(pipe_root_dir_, i);
        p2s_mutex_[i] = std::make_unique<std::mutex>();
        s2p_mutex_[i] = std::make_unique<std::mutex>();
    }

    // mkfifo only if not already present.
    auto ensure_fifo = [&](const std::string &path) {
        // buf for stat syscall
        struct stat meta{};
        if(stat(path.c_str(), &meta) == 0 && S_ISFIFO(meta.st_mode)) {
            return true;
        }
        if(mkfifo(path.c_str(), 0660) != 0) {
            std::cerr << "mkfifo(" << path << "): " << strerror(errno) << "\n";
            return false;
        }
        return true;
    };

    if(create_fifos_) {
        for(int i=0; i<num_slots_; ++i) {
            std::error_code ec;
            fs::create_directories(GetP2SFIFODir(pipe_root_dir_, i), ec);
            fs::create_directories(GetS2PFIFODir(pipe_root_dir_, i), ec);
        }
    }

    // whether or not create_fifos flag is set, we check for file existence orelse make_fifo
    for(int i=0; i<num_slots_; ++i) {
        if(!ensure_fifo(p2s_fifo_[i])) return false;
        if(!ensure_fifo(s2p_fifo_[i])) return false;
        // open fifo once in the communication cycle to avoid blocking reads and writes.
        p2s_fd_[i] = open(p2s_fifo_[i].c_str(), O_RDWR);
        s2p_fd_[i] = open(s2p_fifo_[i].c_str(), O_RDWR);

        if(p2s_fd_[i] < 0) return false;
        if(s2p_fd_[i] < 0) return false;
    }

    alive_ = true;
    return alive_;
}

void PipeCommunication::shutdown() {
    for(int i=0; i<num_slots_; ++i) {
        std::error_code ec;
        fs::remove_all(GetP2SFIFODir(pipe_root_dir_, i), ec);
        fs::remove_all(GetS2PFIFODir(pipe_root_dir_, i), ec);
        close(p2s_fd_[i]);
        close(s2p_fd_[i]);
    }
    alive_ = false;
}

bool PipeCommunication::send_to_player(int slot, const std::string& msg) {
    if(!SlotValid(slot, num_slots_)) return false;
    return w_fifo(s2p_fd_[slot], msg, *s2p_mutex_[slot]);
}

std::optional<std::string> PipeCommunication::recv_from_player(int slot) {
    if(!SlotValid(slot, num_slots_)) return std::nullopt;
    return r_fifo(p2s_fd_[slot]);
}

bool PipeCommunication::send_to_server(int slot, const std::string& msg) {
    if(!SlotValid(slot, num_slots_)) return false;
    return w_fifo(p2s_fd_[slot], msg, *p2s_mutex_[slot]);
}

std::optional<std::string> PipeCommunication::recv_from_server(int slot) {
    if(!SlotValid(slot, num_slots_)) return std::nullopt;
    return r_fifo(s2p_fd_[slot]);
}

} // namspace werewolf
