#pragma once

#include "werewolf/communication.h"
#include <memory>
#include <vector>
#include <mutex>

namespace werewolf {

class PipeCommunication : public ICommunication {
public:
    explicit PipeCommunication(std::string pipe_root_dir="/home/moderator/pipes", bool create_fifos = false);

    ~PipeCommunication() override;

    bool initialize(int num_slots) override;
    void shutdown() override;
    bool send_to_player(int slot, const std::string& msg) override;
    std::optional<std::string> recv_from_player(int slot) override;
    bool send_to_server(int slot, const std::string& msg) override;
    std::optional<std::string> recv_from_server(int slot) override;

private:
    std::string pipe_root_dir_;
    bool create_fifos_;
    int num_slots_ = 0;
    bool alive_ = false;
    
    // states
    std::vector<std::string> p2s_fifo_;
    std::vector<std::string> s2p_fifo_;
    std::vector<int> p2s_fd_;
    std::vector<int> s2p_fd_;
    std::vector<std::unique_ptr<std::mutex>> p2s_mutex_;
    std::vector<std::unique_ptr<std::mutex>> s2p_mutex_;

    // fifo operations
    bool w_fifo(const int fd, const std::string& msg, std::mutex& mu);
    std::optional<std::string> r_fifo(const int fd);

};

std::unique_ptr<ICommunication> make_pipe_communication(const std::string& pipe_root_dir="/home/moderator/pipes/", bool create_fifos=false);

} // namespace werewolf
