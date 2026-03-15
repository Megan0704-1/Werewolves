## Shared memory implementation of communication backend
Author: Megan Kuo
Design choice: Ring-buffer, POSIX mutex, Length-prefixed message framing

### Design Rationale

#### Ring-buffer
In shared memory IPC, three primary data structures are commonly evaluated: ring-buffers, state snapshots (seqlocks), and memory pooling. 
* **State snapshots** prioritize the most recent state and discard historical data, making them ideal for high-frequency sensor updates (e.g., route planning). 
* **Memory pooling** handles dynamically sized objects and complex topologies (like trees or graphs) by allocating variable-length blocks.
* **Ring-buffers**, acting as continuous FIFO queues, strictly preserve chronological order and historical integrity. 

In this Werewolf project, the data transmitted are sequential game messages. Since message history and chronological ordering are paramount (messages cannot be dropped or received out of order), the ring-buffer is the optimal architectural choice for our communication protocol.

#### POSIX `pthread_mutex` 
This represents a critical system-level engineering decision. The C++ standard library primitives, `std::mutex` and `std::lock_guard`, are strictly designed for thread synchronization *within a single process*. If instantiated in shared memory, their internal states remain process-private, leading to undefined behavior and complete loss of synchronization across process boundaries. 

To achieve true Inter-Process Communication (IPC), we bypass the standard library and utilize POSIX `pthread_mutex_t` initialized with the `PTHREAD_PROCESS_SHARED` attribute. This guarantees that threads from entirely distinct processes can safely contend for a single, unified lock residing in the mapped memory region.

#### Message Framing (Length-Prefixed)
To ensure reliable stream parsing over the ring-buffer, we enforce a strict length-prefixed framing protocol. Every message is serialized into the shared memory with the layout: `[uint32_t length][char[] payload]`.

This design explicitly prevents the "partial read" problem. By peeking at the `length` header first, the consumer can definitively verify if the entire payload has been fully committed to the buffer. If the available bytes are less than the frame size, the consumer aborts the read without advancing the internal pointer, ensuring data integrity across the asynchronous producer-consumer boundaries.
