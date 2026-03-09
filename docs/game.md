## Werewolf Game logic implementation

Destroy(/scope exit) Invaiants:
- `running_` state being false.
- if `comm_` is initialized, shutdown, otherwise, no-op.
- flush every occupied resources (log files).

Policy:
- deterministic:
    - In role assignment, the game pick smallest n_wolves to be wolves, next available slot to be witch, others are townperson. (e.g., available and alive slots are [0,1,2,3,4,5], then slot 0 and slot 1 is picked to be wolves, slot 2 is picked to be witch.)
    - In night phase, the game default pick the smallest victim that is not wolves to killed. (e.g., available and alive slots are [0,1,2,3,4,5], and slot 0, slot 1 are wolves, we choose 2 as victim.)
    - In day phase, the game default to vote out smallest participant. (e.g., available and alive slots are [0,1,2,3,4,5], and slot 0, slot 1 are wolves, we choose 0 as victim.)

Win rule:
1. when wolves are eliminated: wolves_remaining == 0, village wins.
2. when wolevs_remaining > village_remaining, wolves win.
3. otherwise, game continues.

Tests: tests/game/test_game.cpp
