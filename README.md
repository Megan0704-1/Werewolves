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

## Note
Make sure to pass the CI test before merging into main.
