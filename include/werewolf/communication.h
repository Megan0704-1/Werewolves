#pragma once

/// @file communication.h
/// This is the abstract class for communication interface.
///
/// Derived class of ICommunication (interface comm) must implement server-side and client-side comm:
/// + send_to_player
/// + recv_from_player
/// + send_to_server
/// + recv_from_server
///
/// Note. 
/// + implementation must be able to handle concurrent access for send & recv from different slots.
/// + recv may block.

#include <optional>
#include <string>
#include <functional>
#include <memory>

namespace werewolf {

class ICommunication {
public:
    virtual ~ICommunication() = default;

    /* handle lifecycle of derived classes */
    // called initialize once before any send/recv.
    // @p num_slots is the number of players(slots) that will be used throughout the game.
    virtual bool initialize(int num_slots) = 0;
    virtual void shutdown() = 0;

    /* server-side interface */
    // sned @p msg from server to player at @p slot.
    virtual bool send_to_player(int slot, const std::string &msg) = 0;
    // recv one message from player at @p slot.
    virtual std::optional<std::string> recv_from_player(int slot) = 0;

    /* client-side interface */
    // sned @p msg from player at @p slot to server.
    virtual bool send_to_server(int slot, const std::string& msg) = 0;
    // recv one message from servew to player at @p slot.
    virtual std::optional<std::string> recv_from_server(int slot) = 0;
};

// API for game, switchable comm backend
using CommunicationFactory = std::function<std::unique_ptr<ICommunication>()>;

} // namespace werewolf
