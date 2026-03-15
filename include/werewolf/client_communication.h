#pragma once

#include <optional>
#include <string>

namespace werewolf {

class IClientCommunication {
 public:
  virtual ~IClientCommunication() = default;

  // called once before any send/recv
  // @p num_slots is the number of players for the session.
  virtual bool initialize(int slot_num) = 0;

  // graceful shutdown, release any hold resources.
  virtual void shutdown() = 0;

  // send @p msg from the player at @p slot to the game server.
  virtual bool send(const std::string& msg) = 0;

  // recv one msg from the server destined for player at @p slot
  virtual std::optional<std::string> recv() = 0;
};

}  // namespace werewolf
