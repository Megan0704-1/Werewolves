#pragma once

#include "werewolf/server_communication.h"
#include "werewolf/client_communication.h"
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>
#include <pthread.h>

namespace werewolf {

// shared memory datastructure
// per-direction, per-slot resides in shm region.
// msgs are framed as [len][payload], len is 4 bytes.
struct ShmChannel {
    static constexpr std::size_t BUF_SIZE = 65536; // 64 KiB per channel
    pthread_mutex_t mutex;
    std::size_t read_pos;
    std::size_t write_pos;
    char buf[BUF_SIZE];
};

// A pair of channels for one slot (server-to-player + player-to-server).
struct ShmSlot {
    ShmChannel s2p; 
    ShmChannel p2s;
};

// Top-level header stored at the beginning of the shared-memory region.
struct ShmHeader {
    uint32_t magic; 
    int num_slots;
};

static constexpr uint32_t SHM_MAGIC = 0x574F4C46; 


// helper
class ShmHelper {
public:
    explicit ShmHelper(std::string shm_name = "/werewolf_shm",
                       bool creator = false);

    ~ShmHelper();

    bool open(int num_slots);
    void close();

    bool write_s2p(int slot, const std::string& msg);
    std::optional<std::string> read_s2p(int slot);

    bool write_p2s(int slot, const std::string& msg);
    std::optional<std::string> read_p2s(int slot);

private:
    std::string shm_name_;
    bool creator_;
    int num_slots_ = 0;
    bool alive_ = false;
    int shm_fd_ = -1;
    void* mapped_ = nullptr;
    std::size_t mapped_size_ = 0;

    ShmSlot* slot_array();
    ShmHeader& header();

    static bool channel_write(ShmChannel& ch, const std::string& msg);
    static std::optional<std::string> channel_read(ShmChannel& ch);
    static void channel_init(ShmChannel& ch);
    static void channel_destroy(ShmChannel& ch);
    static std::size_t region_size(int num_slots);
};


// server
class ServerShmCommunication : public IServerCommunication {
public:
    explicit ServerShmCommunication(bool creator = false, std::string shm_name = "/werewolf_shm");
    ~ServerShmCommunication() override;

    bool initialize(int num_slots) override;
    void shutdown() override;
    bool send(int slot, const std::string& msg) override;
    std::optional<std::string> recv(int slot) override;

private:
    ShmHelper helper_;
};


// client
class ClientShmCommunication : public IClientCommunication {
public:
    explicit ClientShmCommunication(std::string shm_name = "/werewolf_shm");
    ~ClientShmCommunication() override;

    bool initialize(int slot_num) override;
    void shutdown() override;
    bool send(const std::string& msg) override;
    std::optional<std::string> recv() override;

private:
    int player_id_ = -1;
    ShmHelper helper_;
};


// factory
std::unique_ptr<IServerCommunication> make_server_shm_communication(
    const std::string& shm_name = "/werewolf_shm", bool create_shm = false);

std::unique_ptr<IClientCommunication> make_client_shm_communication(
    const std::string& shm_name = "/werewolf_shm");

} // namespace werewolf

