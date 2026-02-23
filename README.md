# SHADOWGRID

*"Move like a ghost. Think like a machine. Strike like a knife in the dark."*

A turn-based cyberpunk roguelike with deep stealth, hacking, and mission-based progression. ASCII terminal interface, permadeath, systemic depth. Think Invisible Inc meets Shadowrun meets CDDA.

---

## Features

- **Stealth-first gameplay** — guards have awareness states (unaware → suspicious → alert → hunting); getting caught is usually fatal
- **Hacking system** — hack terminals to unlock doors, trigger alarms, or erase guard awareness
- **Three skill trees** — Shadow (stealth/infiltration), Netrunner (hacking/tech), Chrome (combat/cyberware)
- **Procedural maps** — facility layout, guard positions, and loot differ every run
- **Elite guards** — high-value missions spawn elite guards with more HP, wider detection range, and harder hits
- **Projectile combat** — flechette gun with 3-dart spread; animate projectile flight
- **Mission → debrief loop** — safe house hub between missions for upgrades, gear, and skill investment

## Build

**Dependencies:** `libncurses-dev`

```bash
sudo apt-get install libncurses-dev
```

```bash
mkdir build && cmake -S . -B build
make -C build
./build/shadowgrid
```

## Controls

| Key | Action |
|-----|--------|
| `hjkl` / arrow keys | Move |
| `yubn` | Diagonal move |
| `c` | Toggle crouch |
| `x` | Hack terminal (stand on `[`) |
| `f` | Fire flechette gun |
| `i` | Inventory |
| `m` | Use med kit |
| `v` | Enter/exit vent |
| `>` | Extract (stand on `>`) |
| `t` | Bullet Time (Chrome skill) |
| `g` | Ghost Protocol (Shadow skill) |
| `o` | Overclock (Netrunner skill) |
| `p` | Place trap (Netrunner skill) |
| `d` | Daemon (Netrunner skill) |
| `e` | EMP (Netrunner skill) |
| `z` | Spoof (Netrunner skill) |
| `G` | GOD MODE (Netrunner capstone) |
| `q` | Quit to hub |

## Map Symbols

| Symbol | Meaning |
|--------|---------|
| `@` | Player |
| `G` | Guard (gray = unaware, yellow = suspicious, red = alert/hunting) |
| `E` | Elite guard (steel blue when unaware) |
| `#` | Wall |
| `.` | Floor |
| `+` | Security door |
| `[` | Hackable terminal |
| `=` | Vent |
| `>` | Extraction stairs |
| `^` | Netrunner trap |

## Skill Trees

### Shadow — Stealth & Infiltration
Ghost Protocol capstone: 5 turns of complete invisibility, once per mission.

### Netrunner — Hacking & Tech
GOD MODE capstone: full facility network control for 3 turns, once per mission.

### Chrome — Combat & Cyberware
REAPER capstone: every kill grants a bonus action, chainable, once per mission.

## Source Layout

```
src/
├── main.c          game loop, input dispatch, state machine
├── map.h/c         tile grid, procedural room+corridor generation
├── fov.h/c         recursive shadowcasting FOV
├── entity.h/c      player struct, stats, inventory, skill trees
├── guard.h/c       guard AI — patrol, awareness FSM, LOS, spawning
├── render.h/c      ncurses draw calls, color pairs
├── items.h/c       item types, rarity tiers, loot tables
├── skilltree.h/c   skill tree definitions and purchase logic
├── hub.c           safe house, shop, mission select, debrief screens
├── projectile.h/c  Bresenham projectile movement, flechette spread
└── netrunner.h     per-mission Netrunner ability state
```

## License

MIT
