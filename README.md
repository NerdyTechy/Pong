# Pong (C/SDL2)

A tiny two-player Pong game written in C using SDL2.

## Controls
- Press space bar to start the game
- Left paddle can be controlled with W/S
- Right paddle can be controlled with up/down arrow keys.

## Build (macOS)
1. Install SDL2 (Homebrew):

```bash
brew install sdl2
```

2. Build:

```bash
make
```

3. Run:

```bash
./pong
```

## Notes
- The game consists entirely of rectangles and therefore no external assets are required
- If compilation fails, ensure `sdl2-config` is in your PATH (Homebrew installs it to /usr/local/bin or /opt/homebrew/bin)
- This is a small demo meant to be extended (levels, menus, sound)
