#pragma once

#include <optional>
#include <string>
#include <vector>

namespace werewolf {

class IServerCommunication {
 public:
  virtual ~IServerCommunication() = default;

  // called once before any send and recv
  virtual bool initialize(int num_slots) = 0;

  virtual void shutdown() = 0;

  // send @p msg from server to player at @p slot.
  virtual bool send(int slot, const std::string& msg) = 0;

  // recv one msg from player at @p slot
  virtual std::optional<std::string> recv(int slot) = 0;

  // broadcast @p msg to all give @p slots
  // default impl loops over send()
  // backend may override with native broadcast.
  virtual void broadcast(const std::string& msg,
                         const std::vector<int>& slots) {
    for (int slot : slots) {
      send(slot, msg);
    }
  }
};

}  // namespace werewolf
