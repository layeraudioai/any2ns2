# BrendanLauncher

Unified Switch 2 Homebrew Launcher merging **HBLauncher** and **IZ2S Platform / IZLauncher** architectures.

## Supported Frameworks & Pre-compiled Apps

- **Hello World** (HBLauncher GPU App)
- **Media Machine** (Switch 2 Media & YouTube Streaming Engine)
- **BlitBun** (PC to NS2 Streaming Module)
- **VacooTube** (NS2 YouTube Client with Touch & Controller Navigation)
- **iz-exchange** (IZ2S File Exchange & FTP Module)

## Any2NS2 Porting Engine & Framework Selector

BrendanLauncher integrates full Any2NS2 porting abilities for all known languages and frameworks (C#, MonoGame, XNA, Godot, Unity, C/C++, Python, Java).

### Usage:
```cmd
node any2ns2.js <sourcedir> <portdir> <iconpath> <apptitle> [appdescription] [--framework hblauncher|izlauncher]
```

- `--framework hblauncher` (default): Generates an HBLauncher GPU app scaffold (`app.json`, shaders, assets).
- `--framework izlauncher`: Generates an IZLauncher / IZ2S-Platform module scaffold (`module.json`, panel metadata).

## Getting Started

Start the unified server:
```cmd
node server.js
```

Follow the console connection instructions for SmileBASIC on Switch 2.
