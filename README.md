# Werewolves
Terminal-based implementation of the Werewolf party game with pluggable communication backends.

## Project structures
- include/: public API.
- src/: private implementation.
- backends/: communication libraries.
- apps/: applications that uses the APIs.

## Requirements & packages
- Linux 
- CMake

## Tests
```bash
mkdir build && cd build

# build
cmake ..
make

# run tests
./tests/test_template
```
