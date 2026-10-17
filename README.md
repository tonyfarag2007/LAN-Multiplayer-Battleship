# Battleship

A 2D Battleship game built in C++ with SFML, supporting both local two-player (hot-seat) play and LAN multiplayer over raw TCP sockets.

## Features

- **Drag-and-drop ship placement** with offset-based dragging, right-click rotation, overlap/bounds validation, and grid snapping
- **LAN multiplayer** using `sf::TcpListener` / `sf::TcpSocket`, with a host/join connection flow and a live "Hosting on: `<ip>`" display
- **Custom tagged-message protocol** over `sf::Packet` (`ATTACK`, `HIT_RESULT`, `TURN_DONE`, `SUNK_SHIP`, `GAME_END`) synchronizing turn state between host and client
- **Defender-authoritative hit resolution** where the receiving player's machine determines hit/miss/sunk, then reports the result back, avoiding desync between the two boards
- **Sunk-ship detection**, highlighting all cells of a destroyed ship in cyan on both the owner's board and the opponent's tracking board
- **Win detection and a "play again" flow** that resets both boards without restarting the connection
- **Disconnect handling** with a dedicated screen if the opponent drops

## Architecture

The game is driven by a single `screen` enum state machine (`CONNECTING → WAITING_FOR_CLIENT/CONNECTING_TO_HOST → PLAYER_ONE/PLAYER_TWO → BATTLESHIP → GAME_OVER/DISCONNECTED`), with per-frame logic branching on the current screen inside the main render loop.

**Core types:**
- `Ship` - length, board position, orientation, and per-ship hit count
- `Player` - five ships, a 10×10 board, a 10×10 tracking board (what you know about the opponent), and ship location lookup for hit resolution

**Networking model:**
- The host is always Player 1, the client is always Player 2
- Sockets are switched to non-blocking (`setBlocking(false)`) immediately after the connection handshake, so the render loop never stalls waiting on the network
- Each turn: the attacker sends an `ATTACK` packet with the target cell; the defender resolves the hit locally, updates its own board, and sends back a `HIT_RESULT` (and a follow-up `SUNK_SHIP` packet if that hit finished off a ship) before handing the turn back with `TURN_DONE`
- Measured sub-15ms round-trip latency between host and client on LAN

## Project Structure

| File | Purpose |
|---|---|
| `NetworkingTest.cpp` | **The actual game.** Full networked implementation - connection setup, gameplay, win/disconnect handling. Build and run this target to play. |
| `main.cpp` | Earlier local-only (hot-seat) prototype, kept for reference. Not networked. |
| `Network.cpp` / `Network.h` | TCP hosting/joining helpers (`startHosting`, `tryAccept`, `startJoining`, `tryConnect`) |
| `CMakeLists.txt` | Build configuration for both targets |

## Requirements

- SFML 3.1.0 (Window, Graphics, Audio, Network components)
- CMake ≥ 3.24
- A C++17 compiler (built and tested with GCC 14.2.0 / MinGW on Windows)

## Building

```bash
cmake -B build -DSFML_DIR=path/to/SFML/lib/cmake/SFML
cmake --build build
```

This produces two executables: `Battleship_GUI` (local prototype) and `NetworkingTest` (the networked game). Both link SFML statically along with the MinGW runtime, so the resulting `.exe` has no external DLL dependencies.

Important: NetworkingTest loads its font from a relative path (AsapSharp.ttf), not an absolute one. CMake copies AsapSharp.ttf into the build output directory automatically via configure_file(), so it will work right after building - but if you move or share the .exe on its own, you must copy AsapSharp.ttf into the same folder as the executable, or font loading will fail and the app will crash on startup.

## Running (LAN multiplayer)

1. One player launches `NetworkingTest` and chooses **Server**, the app displays its local IP ("Hosting on: `<ip>`")
2. The other player launches `NetworkingTest`, chooses **Client**, and enters the host's IP address
3. Once connected, both players place their five ships (drag to move, right-click to rotate) and battle begins
4. Players alternate turns clicking cells on the tracking board; hits, misses, and sunk ships update live for both sides
5. On game over, either player can choose "Play Again" to reset both boards and start a new round on the same connection

**Note:** LAN-only, this doesn't do NAT traversal, so both machines need to be on the same local network. Port 53000 needs to be allowed through any local firewall.

## Known Limitations

- LAN only, no internet play (no NAT traversal/relay server)
- No reconnection after a disconnect, the match ends
- `main.cpp`'s local mode currently loads a hardcoded Windows font path (`C:/Windows/Fonts/arial.ttf`). The networked build (`NetworkingTest.cpp`) instead bundles its own font file, which is the more portable approach

## Possible Future Work

- NAT traversal or a lightweight relay server for non-LAN play
- Reconnect-on-drop instead of ending the match
- Spectator mode / replay of a finished game
