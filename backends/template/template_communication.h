#pragma once

#include <memory>

#include "werewolf/client_communication.h"
#include "werewolf/server_communication.h"

namespace werewolf {

class TemplateServerCommunication : public IServerCommunication {
 public:
  bool initialize(int num_slots) override;
  void shutdown() override;
  bool send(int slot, const std::string& msg) override;
  std::optional<std::string> recv(int slot) override;
};

class TemplateClientCommunication : public IClientCommunication {
 public:
  bool initialize(int slot_num) override;
  void shutdown() override;
  bool send(const std::string& msg) override;
  std::optional<std::string> recv() override;

 private:
  int player_id = -1;
};

std::unique_ptr<IServerCommunication> make_server_template_communication();
std::unique_ptr<IClientCommunication> make_client_template_communication();

}  // namespace werewolf
