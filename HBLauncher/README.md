# HelloWorldS2

A simplified, cross-platform host project for running **Hello World** on **SmileBASIC / Switch 2**, without requiring the Raspberry Pi Pico keyboard server.

## Overview

- **Host PC (Windows, macOS, or Linux):** Hosts the RPC bridge server, handles console qualification, performs module inspections, and runs the Hello World GPU app.
- **Console (Switch 2):** You manually run the REMOVE command and the generated START command in SmileBASIC.

---

## Prerequisites & Dependency Explanations

1. **Node.js (v22+)**
   - *Why:* Serves as the core host runtime environment. It executes `server.js`, manages the TCP connection listener (`node:net`), hosts the local bridge HTTP server (`node:http`), and orchestrates the inspection and pipeline scripts.

2. **Python 3**
   - *Why:* Used to execute the asset rasterization script (`catalog-assets.py`) during menu preparation.

3. **Pillow (`PIL` - Python Imaging Library)**
   - *Why:* Used by `catalog-assets.py` to process homebrew application assets. It opens app icons, resizes them (`ImageOps.contain`), rasterizes application titles and descriptions into text bitmap tiles (`ImageDraw.Draw`), and packs them into a fixed GPU texture atlas (`catalog.bin`) so the Switch 2 console can correctly render app thumbnails and menu labels in the homebrew interface. *(Note: `server.js` now automatically checks for Pillow and installs it via `pip` if missing).*

4. **Network Access**
   - *Why:* Inbound TCP port **39380** must be open/allowed through your firewall so the Switch 2 console can connect to the host PC's RPC server.

---

## Getting Started

1. Start the RPC server:
   ```cmd
   node server.js
   ```
   *(Optional: Use `node server.js --verbose` to enable low-level RPC and native exchange logs).*

2. The script will display your computer's local IP address and the exact `START(...)` command for SmileBASIC.

3. On your Switch 2 console running SmileBASIC Direct mode:
   - Run the manual `REMOVE` command.
   - Run the generated `START(...)` command provided by the server.

4. The host will automatically detect the console connection, qualify the native session, run inspection steps, prepare the **Hello World** app, and render it on your console!
