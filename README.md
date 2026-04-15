# Werewolf -- Terminal-Based Party Game

A terminal-based implementation of the Werewolf (Mafia) party game written in C++17, featuring a pluggable communication backend architecture. 
Each backend implements the same abstract interface so the game logic is completely decoupled from the IPC mechanism.

## Group Members & Primitives

| Member      | Concurrency Primitive | Communication Library File                     |
|-------------|-----------------------|------------------------------------------------|
| Megan Kuo  | Named Pipes (FIFO)    | `backends/pipe/pipe_communication.cpp`         |
| Megan Kuo   | **Shared Memory**     | `backends/shm/shm_communication.cpp`           |
|  Timo Lin  |  RPC  | |
| Gene Wang | Async IO     | |
| Shu-Wen Yeh | Message Queue | |

> **My individual contribution:** Game logic, Communication Abstractions, and the shared-memory communication backend (`backends/shm/shm_communication.cpp` and `backends/shm/shm_communication.h`).

## Project Structure

```
.
├── include/werewolf/           # Public API headers
│   ├── server_communication.h  #   IServerCommunication interface
│   ├── client_communication.h  #   IClientCommunication interface
│   ├── game.h                  #   Game engine header
│   └── types.h                 #   Shared types (Role, GameConfig, etc.)
│
├── src/
│   └── game.cpp                # Game engine implementation
│
├── backends/                   # Communication libraries (one per group member)
│   ├── shm/                    #   ★ Shared memory backend (MY WORK)
│   │   ├── shm_communication.h
│   │   └── shm_communication.cpp
│   ├── pipe/                   #   Named-pipe (FIFO) backend
│   │   ├── pipe_communication.h
│   │   └── pipe_communication.cpp
│   └── template/               #   Skeleton for additional backends
│
├── frontends/                  # Executables that wire a backend to the game
│   ├── shm/                    #   Server + client using shared memory
│   └── pipes/                  #   Server + client using named pipes
│
├── tests/                      # Unit tests & end-to-end scripts
│   ├── shm/                    #   Shared memory backend tests
│   ├── pipe/                   #   Pipe backend tests
│   └── game/                   #   Game logic tests
│
├── docs/                       # Design documentation
│   ├── communication_contract.md
│   ├── game.md
│   └── shm_backend.md
│
├── CMakeLists.txt
└── README.md                   # ← You are here
```

## Communication Library API

Every backend implements two abstract interfaces defined in `include/werewolf/`:

### `IServerCommunication` (server side)

```cpp
bool initialize(int num_slots);                       // Prepare resources for N player slots
void shutdown();                                      // Release all resources
bool send(int slot, const std::string& msg);          // Send to one player
std::optional<std::string> recv(int slot);             // Non-blocking receive from one player
void broadcast(const std::string& msg,
               const std::vector<int>& slots);        // Send to multiple players
```

### `IClientCommunication` (client side)

```cpp
bool initialize(int slot_num);            // Attach to the game at a given slot
void shutdown();                          // Release resources
bool send(const std::string& msg);       // Send a message to the server
std::optional<std::string> recv();        // Non-blocking receive from the server
```

Key contract points (full specification in `docs/communication_contract.md`):
- Per-slot isolation: messages for slot N never leak to slot M.
- Message boundary preservation: one `send()` ↔ one `recv()`.
- In-order delivery within each (slot, direction) pair.
- `recv()` is always **non-blocking** -- returns `std::nullopt` when nothing is available.

## Requirements

- Linux (tested on Ubuntu 22.04 / 24.04)
- CMake >= 3.14
- A C++17-capable compiler (GCC >= 9 or Clang >= 10)
- POSIX shared-memory support (`/dev/shm`)

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Run (Shared Memory Backend)

Open **one terminal** for the server and **one terminal per player** for the clients.

### Server

```bash
./build/frontends/shm/werewolf_shm_server \
    --players 5 \
    --wolves 1 \
    --lobby 30 \
    --chat 30 \
    --vote 30 \
    --speech 15 \
    --witch 15 \
    --det-assign
```

Server options:
- `--players N` -- Maximum player slots (default 16)
- `--wolves N` -- Number of wolves (default 2)
- `--lobby SEC` -- Lobby wait time
- `--chat SEC` -- Chat phase duration
- `--vote SEC` -- Vote phase duration
- `--speech SEC` -- Death speech duration
- `--witch SEC` -- Witch decision duration
- `--det-assign` -- Deterministic role assignment (for testing)
- `--shm-name NAME` --Custom shared memory object name (default `/werewolf_shm`)

### Clients

Each client needs a unique **slot number** starting from 0:

```bash
# Terminal 1
./build/frontends/shm/werewolf_shm_client 0

# Terminal 2
./build/frontends/shm/werewolf_shm_client 1

# Terminal 3
./build/frontends/shm/werewolf_shm_client 2

# ... and so on
```
Client options:
- `--id ID` -- player ID
- `--shm-name NAME` -- Should follow the same shm-name declared in server configuration.

### In-Game Commands

Once connected, players type commands depending on the current game phase:

| Phase         | Command                   | Example                |
|---------------|---------------------------|------------------------|
| Chat          | `chat: <message>`         | `chat: I suspect Bob`  |
| Vote          | `vote: <player_name>`     | `vote: player3`        |
| Witch (heal)  | `heal`                    | `heal`                 |
| Witch (poison)| `poison: <player_name>`   | `poison: player2`      |
| Witch (skip)  | `skip`                    | `skip`                 |

## Tests

```bash
cd build
ctest                                    # Run all tests
./tests/test_shm                         # Shared memory backend unit tests
bash ../tests/shm/e2e/test_shm_e2e.sh   # End-to-end integration test
```

## Game Rules

Werewolf is a social deduction game with the following phases each round:

1. **Night Phase** -- Wolves privately vote to kill a villager; the Witch may heal or poison.
2. **Day Phase** -- All living players discuss and vote to lynch a suspect.
3. **Win Conditions** -- Village wins when all wolves are eliminated; Wolves win when they equal or outnumber the villagers.

Roles: Townperson, Wolf, Witch (with heal and poison powers).
