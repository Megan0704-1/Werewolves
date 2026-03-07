## Werewolf Game logic implementation

Destroy(/scope exit) Invaiants:
- `running_` state being false.
- if `comm_` is initialized, shutdown, otherwise, no-op.
- flush every occupied resources (log files).

Tests: tests/game/test_game.cpp
