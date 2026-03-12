This document defines the semantics for any backend implementing `werewolf::ICommunication`.

The game logic is depending on these semantics, it does not rely on communication-specific behavior from pipes, MPI, RPC, shared memory, or any other backend.

## Model
Communication is modeled as 2 logical per-slot message channels
- player -> server
- server -> player

Each slot can be viewed as independent logical mailbox.

## Semantics
### 1. Slots
Messages for one slot must not be mixed with messages for another slot.
E.g., `recv_from_player(3)` must not return a msg sent by slot(5).

### 2. Message boundary
Each call to `send_to_player(slot, msg)` or `send_to_server(slot, msg)` submit exactly 1 msg.
Each successfuly `recv_from_player(slot)` or `recv_from_server(slot)` return exactly 1 msg.

Backends must not merge multiple messages into 1 recv result, and must **not** split a single msg across multiple recv results.
E.g., `r_fifo` from pipe_communication.cpp identify 1 msg by splitting with char `\n`.

### 3. In-order delivery per slot and direction
For a fixed slot and direction (s2p/p2s), messages must be observed in send order.

E.g.,
- If a client (slot 4) sends `A` and then sends `B` to the server, the server must receive `A` and then `B` from `recv_from_player(4)`.
- If the server sends `A` and then sends `B` to slot 1, the client must not receive `B` before `A`.

No ordering is required across different slots.

### 4. NON-blocking recieve
`recv_from_player(slot)` and `recv_from_server(slot)` must b e non-blocking.
If no complete msg is currently available, they much return `std::nullopt`.
They must not sleep internally for long periods, and must not block waiting for future inputs.

The game is responsible for polling and timing.

### 5. Single logical producer / single logical consumer per direction

For each slot:
- player -> server:
  - producer: that player only
  - consumer: server only
- server -> player:
  - producer: server only
  - consumer: that player only

Backends may internally use more complex machinery, but externally they must present this mailbox-like behavior.

### 6. Valid slot range after initialization

After `initialize(num_slots)` succeeds, the backend must support logical slots:

- `0, 1, ..., num_slots - 1`

Using a slot outside this range is invalid.


### 7. Initialization semantics

`initialize(num_slots)` prepares all communication resources needed for slots `[0, num_slots)`.

- If it returns `true`, subsequent send/receive calls for valid slots must be usable.
- If it returns `false`, the backend is not usable.

### 8. Shutdown semantics

`shutdown()` releases backend resources.

After shutdown, send/receive behavior is no longer guaranteed.
The game logic must not depend on successful send/receive after shutdown.


## What this contract does NOT guarantee

This contract does not guarantee:

- buffering of out-of-phase messages at the `Game` layer
- automatic message routing by type
- retransmission or network fault recovery
- fairness across slots
- wall-clock synchronization between clients

These concerns are either:
- handled by protocol discipline, or
- left to backend-specific implementation choices
