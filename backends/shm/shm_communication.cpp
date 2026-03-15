#include "shm_communication.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace werewolf {

static bool SlotValid(int slot, int num_slots) {
  return slot >= 0 && slot < num_slots;
}

std::size_t ShmHelper::region_size(int num_slots) {
  return sizeof(ShmHeader) +
         sizeof(ShmSlot) * static_cast<std::size_t>(num_slots);
}

ShmSlot* ShmHelper::slot_array() {
  // slots start right after the header
  auto* base = static_cast<char*>(mapped_);
  return reinterpret_cast<ShmSlot*>(base + sizeof(ShmHeader));
}

ShmHeader& ShmHelper::header() {
  auto* base = static_cast<ShmHeader*>(mapped_);
  return base[0];
}

void ShmHelper::channel_init(ShmChannel& ch) {
  ch.read_pos = 0;
  ch.write_pos = 0;
  std::memset(ch.buf, 0, ShmChannel::BUF_SIZE);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&ch.mutex, &attr);
  pthread_mutexattr_destroy(&attr);
}

void ShmHelper::channel_destroy(ShmChannel& ch) {
  pthread_mutex_destroy(&ch.mutex);
}

bool ShmHelper::channel_write(ShmChannel& ch, const std::string& msg) {
  const uint32_t len = static_cast<uint32_t>(msg.size());
  const std::size_t frame_size = sizeof(uint32_t) + len;

  pthread_mutex_lock(&ch.mutex);

  // available space in the ring (we never let write_pos wrap past read_pos)
  std::size_t used = ch.write_pos - ch.read_pos;
  std::size_t free_space = ShmChannel::BUF_SIZE - used;

  if (frame_size > free_space) {
    pthread_mutex_unlock(&ch.mutex);
    return false;  // ring full
  }

  // write length header byte-by-byte into the ring
  auto put = [&](const void* src, std::size_t n) {
    auto* s = static_cast<const char*>(src);
    for (std::size_t i = 0; i < n; ++i) {
      ch.buf[(ch.write_pos + i) % ShmChannel::BUF_SIZE] = s[i];
    }
    ch.write_pos += n;
  };

  put(&len, sizeof(uint32_t));
  put(msg.data(), len);

  pthread_mutex_unlock(&ch.mutex);
  return true;
}

std::optional<std::string> ShmHelper::channel_read(ShmChannel& ch) {
  pthread_mutex_lock(&ch.mutex);

  std::size_t avail = ch.write_pos - ch.read_pos;
  // nothing to read
  if (avail < sizeof(uint32_t)) {
    pthread_mutex_unlock(&ch.mutex);
    return std::nullopt;
  }

  // peek at the length header
  auto peek = [&](void* dst, std::size_t n, std::size_t pos) {
    auto* d = static_cast<char*>(dst);
    for (std::size_t i = 0; i < n; ++i) {
      d[i] = ch.buf[(pos + i) % ShmChannel::BUF_SIZE];
    }
  };

  uint32_t len = 0;
  peek(&len, sizeof(uint32_t), ch.read_pos);

  std::size_t frame_size = sizeof(uint32_t) + len;
  if (avail < frame_size) {
    // partial frame, not ready yet
    pthread_mutex_unlock(&ch.mutex);
    return std::nullopt;
  }

  std::string msg(len, '\0');
  std::size_t read_off = ch.read_pos + sizeof(uint32_t);
  peek(msg.data(), len, read_off);
  ch.read_pos += frame_size;

  // if the ring is empty, reset both pointers to avoid ever-growing indices
  if (ch.read_pos == ch.write_pos) {
    ch.read_pos = 0;
    ch.write_pos = 0;
  }

  pthread_mutex_unlock(&ch.mutex);
  return msg;
}

ShmHelper::ShmHelper(std::string shm_name, bool creator)
    : shm_name_(std::move(shm_name)), creator_(creator) {}

ShmHelper::~ShmHelper() { close(); }

bool ShmHelper::open(int num_slots) {
  num_slots_ = num_slots;
  mapped_size_ = region_size(num_slots);

  if (creator_) {
    // create (or recreate) the segment
    shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_RDWR, 0660);
  } else {
    shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0660);
  }

  if (shm_fd_ < 0) {
    std::cerr << "shm_open(" << shm_name_ << "): " << strerror(errno) << "\n";
    return false;
  }

  if (creator_) {
    if (ftruncate(shm_fd_, static_cast<off_t>(mapped_size_)) != 0) {
      std::cerr << "ftruncate: " << strerror(errno) << "\n";
      ::close(shm_fd_);
      shm_fd_ = -1;
      return false;
    }
  }

  mapped_ = mmap(nullptr, mapped_size_, PROT_READ | PROT_WRITE, MAP_SHARED,
                 shm_fd_, 0);
  if (mapped_ == MAP_FAILED) {
    std::cerr << "mmap: " << strerror(errno) << "\n";
    ::close(shm_fd_);
    shm_fd_ = -1;
    mapped_ = nullptr;
    return false;
  }

  if (creator_) {
    // zero the whole region, then initialise header + channels
    std::memset(mapped_, 0, mapped_size_);

    ShmHeader& hdr = header();
    hdr.magic = SHM_MAGIC;
    hdr.num_slots = num_slots_;

    ShmSlot* slots = slot_array();
    for (int i = 0; i < num_slots_; ++i) {
      channel_init(slots[i].s2p);
      channel_init(slots[i].p2s);
    }
  } else {
    // validate the header
    auto& hdr = header();
    if (hdr.magic != SHM_MAGIC) {
      std::cerr << "shm: bad magic number\n";
      std::cerr << "Expect: " << SHM_MAGIC << ", Got: " << hdr.magic << "\n";
      close();
      return false;
    }
  }

  alive_ = true;
  return true;
}

void ShmHelper::close() {
  if (!alive_) return;

  if (creator_ && mapped_) {
    ShmSlot* slots = slot_array();
    for (int i = 0; i < num_slots_; ++i) {
      channel_destroy(slots[i].s2p);
      channel_destroy(slots[i].p2s);
    }
  }

  if (mapped_ && mapped_ != MAP_FAILED) {
    munmap(mapped_, mapped_size_);
    mapped_ = nullptr;
  }

  if (shm_fd_ >= 0) {
    ::close(shm_fd_);
    shm_fd_ = -1;
  }

  if (creator_) {
    shm_unlink(shm_name_.c_str());
  }

  alive_ = false;
}

bool ShmHelper::write_s2p(int slot, const std::string& msg) {
  if (!alive_ || !SlotValid(slot, num_slots_)) return false;
  return channel_write(slot_array()[slot].s2p, msg);
}

bool ShmHelper::write_p2s(int slot, const std::string& msg) {
  if (!alive_ || !SlotValid(slot, num_slots_)) return false;
  return channel_write(slot_array()[slot].p2s, msg);
}

std::optional<std::string> ShmHelper::read_s2p(int slot) {
  if (!alive_ || !SlotValid(slot, num_slots_)) return std::nullopt;
  return channel_read(slot_array()[slot].s2p);
}

std::optional<std::string> ShmHelper::read_p2s(int slot) {
  if (!alive_ || !SlotValid(slot, num_slots_)) return std::nullopt;
  return channel_read(slot_array()[slot].p2s);
}

// server
ServerShmCommunication::ServerShmCommunication(bool creator,
                                               std::string shm_name)
    : helper_(std::move(shm_name), creator) {}

ServerShmCommunication::~ServerShmCommunication() { shutdown(); }

bool ServerShmCommunication::initialize(int num_slots) {
  return helper_.open(num_slots);
}

void ServerShmCommunication::shutdown() { helper_.close(); }

bool ServerShmCommunication::send(int slot, const std::string& msg) {
  return helper_.write_s2p(slot, msg);
}

std::optional<std::string> ServerShmCommunication::recv(int slot) {
  return helper_.read_p2s(slot);
}

// client
ClientShmCommunication::ClientShmCommunication(std::string shm_name)
    : helper_(std::move(shm_name), /*creator=*/false) {}

ClientShmCommunication::~ClientShmCommunication() { shutdown(); }

bool ClientShmCommunication::initialize(int slot_num) {
  player_id_ = slot_num;
  return helper_.open(slot_num + 1);
}

void ClientShmCommunication::shutdown() { helper_.close(); }

bool ClientShmCommunication::send(const std::string& msg) {
  return helper_.write_p2s(player_id_, msg);
}

std::optional<std::string> ClientShmCommunication::recv() {
  return helper_.read_s2p(player_id_);
}

// factory
std::unique_ptr<IServerCommunication> make_server_shm_communication(
    const std::string& shm_name, bool create_shm) {
  return std::make_unique<ServerShmCommunication>(create_shm, shm_name);
}

std::unique_ptr<IClientCommunication> make_client_shm_communication(
    const std::string& shm_name) {
  return std::make_unique<ClientShmCommunication>(shm_name);
}

}  // namespace werewolf
