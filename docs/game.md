# Werewolf Game logic implementation

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

# Werewolf Game Protocol

This paragraph defines the application-level protocol between the server and clients.

The current `Game` implementation assumes protocol discipline:
- clients send only phase-appropriate messages after receiving the corresponding server prompt.

Out-of-phase messages are not buffered by `Game`.

## General rules

1. The server drives all phases.
2. The server sends an explicit prompt at the start of each phase.
3. Clients should only send the message type expected for that phase.
4. If a client sends a message for the wrong phase, current behavior is
   unspecified and may result in the message being ignored or consumed.


## Slot identity

Each client is assigned a fixed slot number externally.
The server internally associates each slot with a display name.

## Message classes

### 1. Lobby connect
Client -> server:

```text
connect
```

Used only during the lobby phase.

2. Chat

Client -> server:

```text
chat: <text>
```

Examples:

chat: hello
chat: I suspect player3

Used only during chat/discussion phases.

3. Vote

Client -> server:

```text
vote: <player_name>
```

Examples:

vote: player3
vote: alice

Used only during vote collection phases.

4. Witch action

Client -> server during witch phase:

```text
skip
heal
poison: <player_name>
```

Examples:

skip
heal
poison: player4

**Rules:**
- heal is valid only if wolves produced an actual victim that night
- poison: <name> is valid if the witch still has poison power remaining

current implementation assumes at most one witch action per night

5. Final words

Client -> server during death speech:

Current implementation may accept plain text directly.

chat: <final words>

The server should explicitly prompt each phase before expecting corresponding messages.

### Example workflow:

#### Lobby
- Server -> player:
Greeting from the game. You are connected, your slot is: <slot>

- Server -> connected players:
Lobby Ready


#### Role notification

- Server -> player:
Your role is <Role>

- For wolves:

Your role is Wolf. Fellow wolves are: <names...>
Night start

- Server -> alive players:

Night starts

#### Wolf chat phase

- Server -> wolves:
Wolves may chat now.

- wolves chat:

chat: ...

#### Wolf vote phase

- Server -> wolves:

Wolves, choose your victim.

- wolves vote:
vote: <player_name>

#### Witch phase

- Server -> witch: (If wolves selected a victim)

The wolves chose <victim_name> tonight.
Witch, decide one of your actions:
+ skip
+ heal
+ poison: <name>

- Server -> witch: (If wolves did not select a victim)

The wolves did not kill anyone tonight.
Witch, decide one of your actions:
+ skip
+ poison: <name>

- witch -> server:

skip
heal
poison: <player_name>


#### Day start

- Server -> alive players:
Day starts

#### Day chat phase

- Server -> alive players:

Discuss now.

- players chat:
chat: ...

#### Day vote phase

- Server -> alive players:

Vote now.

- players vote:

vote: <player_name>

#### Death speech

- Server -> dead player / alive players:

<player_name> may give final words.

- dead player speech:

plain text or chat: <text> depending on implementation choice

#### Game end

- Server -> connected players:

close

Clients should terminate gracefully after receiving close.

### Current protocol discipline assumption

- The current Game implementation assumes:
- chat messages arrive only during chat phases
- vote messages arrive only during vote collection
- witch messages arrive only during witch phase
- final words arrive only during death speech
- Game does not currently buffer or defer out-of-phase messages.
