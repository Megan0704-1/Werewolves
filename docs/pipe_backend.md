## Named-pipes implementation of communication backend
Purpose: mimicking werewolves python project.

Invaiants:
- Each slow owns 2 FIFOs:
    1. player -> server (p2s_fifo_[slot])
    2. server -> player (s2p_fifo_[slot])
- One writer per FIFO:
    1. p2s_fifo_[slot] is only written by player[slot]
    2. s2p_fifo_[slot] is only written by server (single thread)
    Therefore, concurrent writes to the same FIFO **never** occur.
- The server opens all FIFOs during `initialize()` with O_RDWR to avoid blocking.
- The file descriptors remain open for the lifetime of the object.

Tests: tests/pipe/test_pipe.cpp

Caveat:
- we don't actually need mutex lock, since we have conclusion that concurrent writes never occurs to the same FIFO, yet we keep those referenced, for defensive programming purpose.
