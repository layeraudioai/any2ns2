# Media Machine

Put supported media files in `content/` and run `node build.cjs` here. The build host needs Python with Pillow, FFmpeg, Node.js, and the repository shader compiler (`glslangValidator`). Files are packaged in case-insensitive filename order; the catalog accepts up to 31 entries.

## Video playback and build optimizations

MP4/M4V, MOV, MKV, WebM, AVI, 3GP, and TS clips are decoded at build time because ABI 1 does not expose a runtime video codec. FFmpeg scales with aspect ratio preserved and letterboxes to **160×90**, samples at **5 fps**, and writes **RGB565 little-endian** directly. This matches the fragment shader's packed-pixel reads and avoids a Python per-pixel conversion pass. The temporary decode is streamed into the package in bounded chunks. Clips loop; **A** starts the selected clip and **X** stops video playback.

Video frame data shares the app data asset with the font atlas. Packaging keeps the asset under the catalog's **7 MiB** cap (with a small safety margin), allocates frame capacity fairly across clips, and reclaims the budget when an earlier clip is shorter than its allowance. Each clip is capped at 100 frames (20 seconds at 5 fps); multiple long videos may be truncated by the shared budget. Change `frame_width`, `frame_height`, `frame_rate`, or the 100-frame cap in `assets.py` to trade image detail, smoothness, and clip length. RGB565 is retained rather than a compressed per-frame format to keep GPU decoding minimal.

Audio is converted to 48 kHz stereo signed 16-bit PCM for the packaged playback path. Video audio, when present, is packaged the same way. Supported audio containers: MP3, WAV, FLAC, OGG, AAC, M4A, WMA, Opus, and M4B. Select a file with Up/Down and press **A**; use **B/Minus** to exit.
