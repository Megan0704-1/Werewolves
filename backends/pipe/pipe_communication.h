#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "werewolf/client_communication.h"
#include "werewolf/server_communication.h"

namespace werewolf {

class PipeFifoHelper {
 public:
  explicit PipeFifoHelper(bool create_fifos = false,
                          std::string pipe_root_dir = "/home/moderator/pipes");

  ~PipeFifoHelper();

  bool open_pipes(int num_slots);
  void close_pipes();

  bool write_s2p(int slot, const std::string& msg);
  std::optional<std::string> read_s2p(int slot);

  bool write_p2s(int slot, const std::string& msg);
  std::optional<std::string> read_p2s(int slot);

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
  std::vector<std::string> p2s_rbuf_;
  std::vector<std::string> s2p_rbuf_;

  // fifo operations
  bool w_fifo(const int fd, const std::string& msg, std::mutex& mu);
  std::optional<std::string> r_fifo(const int fd, std::string& rbuf);
};

// pipe server
class ServerPipeCommunication : public IServerCommunication {
 public:
  explicit ServerPipeCommunication(
      bool create_fifos = false,
      std::string pipe_root_dir = "/home/moderator/pipes");

  ~ServerPipeCommunication() override;

  bool initialize(int num_slots) override;
  void shutdown() override;
  bool send(int slot, const std::string& msg) override;
  std::optional<std::string> recv(int slot) override;

 private:
  PipeFifoHelper helper_;
};

// pipe client
class ClientPipeCommunication : public IClientCommunication {
 public:
  explicit ClientPipeCommunication(
      std::string pipe_root_dir = "/home/moderator/pipes");

  ~ClientPipeCommunication() override;

  bool initialize(int slot_num) override;
  void shutdown() override;
  bool send(const std::string& msg) override;
  std::optional<std::string> recv() override;

 private:
  int player_id = -1;
  PipeFifoHelper helper_;
};

std::unique_ptr<IServerCommunication> make_server_pipe_communication(
    bool create_fifos = false,
    const std::string& pipe_root_dir = "/home/moderator/pipes/");

std::unique_ptr<IClientCommunication> make_client_pipe_communication(
    const std::string& pipe_root_dir = "/home/moderator/pipes/");

}  // namespace werewolf
