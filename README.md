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
- CPP compiler
- Clang-format
- Pre-commit

## Tests
We use googletest to manage the project.
```bash
# make a new build or clear the current build dir
mkdir build && cd build

# build
cmake ..
make

# run tests
# all tests
ctest
# individual tests
./tests/test_template
```

## Play
To play with this game, you'll need to build the project first (follow instruction in tests).
```bash
# cd to project root

# launch server
./build/frontends/pipes/werewolf_pipe_server --pipe-root /tmp/werewolves --create-fifos --players 4 --wolves 1 --lobby 10 --chat 10 --vote 10 --speech 10 --witch 1 --det-assign

# launch clients
./build/frontends/pipes/werewolf_pipe_client 1 --pipe-root /tmp/werewolves
```

## Development
```bash
pre-commit install # this enforce clang-format before commiting
```

## Note
Make sure to pass the CI test before merging into main.

## TODO
chat_delay is a temporary fix for chat_phase taking msg from the fake_communication queue.
