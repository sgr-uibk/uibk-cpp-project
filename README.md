# Game Specification

We built a simple real-time networked multiplayer game.

We have an isometric view of the world, players are tanks, they have health and can shoot projectiles.

There are obstacles on the map that can be damaged and destroyed. Collisions with obstackles deals damage to players.
The maps themselves are predefined, stored as json files.

Game flow is a classic Lobby - Battle loop:
There is a dedicated server hosting a lobby, where players can join, the game starts when all players are ready.
In the match, players can move around the map and shoot each other until the last one survives, who is the winner.

An emphasis was made on a proper Client-Server model that makes many forms of cheating impossible by design (e.g.
noclip, teleport / movement speed hacks, health and damage cheats, etc.).
The Server is always authoritative, clients do prediction and reconciliation of their local state to ensure a smooth
experience for the player.
In case of a discrepancy between client and server, the clients will accept the server state to ensure consistency and
prevent the above mentioned ways of cheating.

The Server enforces UDP packet ordering - it drops reordered and duplicated packets to prevent cheating on the network
layer.
Passive cheats are still possible to some degree: The client does interpolation, so with intentional modification of
their client, cheating players can exploit this system to show all interpolation data, thus see where enemies are moving
a few frames ahead of time.

:
- Nier Automata's Hacking Minigame ![](https://www.confreaksandgeeks.com/wp-content/uploads/2017/03/NieR_Automata_20170315171011.jpg)
- Tank 1990
- Tank Trouble
- ![Reference Picture](images/reference_game1.gif)
- ![Reference Picture](images/reference_game2.jpg)
---

## Goals (11 Points)

- (1) Matchmaking Lobby [Maksym]
    - Players connect to a lobby hosted by a player, by entering an ip:port.
    - Up to 4 players, per match
  - The Map is chosen at random by the server in the lobby, before the match starts.
  - Players mark themselves as ready, when all players are ready, the server starts the game.
- (1) Battle logic [Simon]
    - Players have colors to distinguish them (blue tank, red tank, ...) and have names.
  - Players can move around, aim at other players and shoot at them
    - When being hit, the players health decreases, player dies when it reaches 0.
  - Match ends once there is only one surviving player left, after the battle, back to lobby and repeat this cycle.
- (1) Isometric Map [Maksym]
  - Premade selection of maps with player/item spawn points, json map parser
  - Player and item sprites for 8 isometric perspectives
- (2) Networking [Simon]
  - Design and implementation of separate State and Client classes, TCP and UDP protocols
  - Client prediction, enemy interpolation, server reconciliation
- (2) Items / Powerups / Projectiles [Maksym]
    - Player can pick up items that are randomly distributed on the map,
    - Items grant the player a temporary effect: speed bonus healing or similar.
  - Items can be used by a player by pressing a key, this consumes the item, cooldown effects.
  - Players have limited ammunition which regenerates over time
- (2) Resource manager, Audio, Logging, CI pipelines, Readme.md [Simon]
  - Templated lazy-loading ResourceManager for Textures, Audio, etc.
  - Audio, logging helpers
  - Github CI pipelines for windows+linux
- (1) Main menu [Maksym]
    - Join Lobby
    - Host Lobby
    - Set Name
    - Exit
- (1) Server commands (end,quit,map), spectator mode WIP, disconnect detection WIP [Julian]


## How to Build

### Build for Windows
First, download and install [Git](https://git-scm.com/install/windows) and [Git-LFS](https://git-lfs.com/). 

Then, `git clone` this repository and open a shell inside its directory.
Run:
```powershell
git lfs install # (only once)
git lfs checkout # to be safe
# The following fetch and build external libraries, so can take a while at first
cmake -S . -B build
cmake --build build --config Release
dir build\Release
```

### Build for Linux (Ubtuntu in this example)
```shell
sudo apt update && sudo apt install -y libsfml-dev cmake build-essential git git-lfs libnlohmann-json3-dev

# If that fails, try explicitly specifying the dependencies for libsfml-dev
sudo apt install -y \
  libfreetype6-dev libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
  libudev-dev libgl1-mesa-dev libflac-dev libogg-dev libvorbis-dev \
  libvorbisenc2 libvorbisfile3 libpthread-stubs0-dev git-lfs 
```

Then, `git clone` this repository and open a shell inside its directory.
```shell
git lfs install # (only once)
git lfs checkout # to be safe
cmake -S . -B build
cmake --build build --config Release
ls build
```

# Network setup

There are a `Server` and a `TankGame` (client) executable.
Per default, the server listens at ports 25106/TCP and 3074/UDP, the values can be changed in `src/Networking.h` if
necessary.

The Client connects by specifying IPv4 address and TCP port in the menu.

# How to run

```bash
uibk-cpp-project/build $ ./Server
uibk-cpp-project/build $ ./TankGame
uibk-cpp-project/build $ ./TankGame
...
```

- Select `Join Lobby`
- Enter IP address and port if neccessary.
- Select `Connect` and `Ready`
  - Do this for all clients, once every client is ready, the server starts the game.

- The server CLI supports these commands:
  - `end` Ends the battle with no winner, sends clients back to lobby
  - `map n`, where `n` is the map index, currently 2 maps are implemented `0` and `1`.
  - `quit` Shuts down the server.

- Client keybinds can be seen when pressing `ESC`, then selecting `Settings`.
- There are multiple powerups that gradually spawn in the map, to collect them, move the tank to the item,
- To shoot, press `Spacebar` or `Mouse Left Click`, note you have limited ammo which regenerates over time.
- To use an item:
  - Select the item's hotbar slot (e.g. by pressing a number key or scrolling with the mouse)
  - press `Q`. The powerup is now active for a specific time.
