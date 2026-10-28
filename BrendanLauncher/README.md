# BrendanLauncher

Unified Switch 2 Hybrid Launcher merging **HBLauncher**, **IZ2S Platform / IZLauncher**, and **SPHAIRA** high-performance immersive rendering architectures.

## Supported Frameworks & Pre-compiled Apps

- **Hello World** (HBLauncher GPU App)
- **Sphaira** (Immersive High-Performance Shader & Audio Rendering Engine)
- **Media Machine** (Switch 2 Media & YouTube Streaming Engine)
- **BlitBun** (PC to NS2 Streaming Module - `.izm`)
- **VacooTube** (NS2 YouTube Client Module - `.izm`)
- **iz-exchange** (IZ2S File Exchange & FTP Module - `.izm`)

## Any2NS2 Porting Engine & Framework Selector

BrendanLauncher integrates full Any2NS2 porting abilities for all known languages and frameworks (C#, MonoGame, XNA, Godot, Unity, C/C++, Python, Java).

### Usage:
```cmd
node any2ns2.js <sourcedir> <portdir> <iconpath> <apptitle> [appdescription] [--framework hblauncher|izlauncher|sphaira] [--build]
```

- `--framework hblauncher` (default): Generates an HBLauncher GPU app scaffold (`app.json`, shaders, assets).
- `--framework izlauncher`: Generates an IZLauncher / IZ2S-Platform module scaffold (`module.json`, panel metadata, `.izm` packaging).
- `--framework sphaira`: Generates a SPHAIRA immersive shader/audio rendering app scaffold.

## Getting Started (BrendanLauncher.bat)

Double-click or run **`BrendanLauncher.bat`** from the project root. It will:
1. Display the SmileBASIC Direct mode commands (`LOAD "TXT:NS2"`, `RUN`, `REMOVE NSR,&60000000,&30000000`).
2. Prompt for your PC IP address.
3. Start the unified BrendanLauncher server and output the generated `V=START(...)` command to paste into your NS2JB SmileBasic4 terminal session.
