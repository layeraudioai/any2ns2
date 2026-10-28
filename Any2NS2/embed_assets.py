#!/usr/bin/env python3
"""Generate compile-time embedded runtime assets for the Any2NS2 executable."""
import base64
import json
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2])
tool_root = Path(__file__).resolve().parent
if sys.platform == "win32":
    suffix = ".exe"
else:
    suffix = ""

assets = {
    "sdk/build.cjs": (root / "BrendanLauncher/sdk/build.cjs").read_text(),
    "sdk/include/state.glsl": (root / "BrendanLauncher/sdk/include/state.glsl").read_text(),
    "sdk/include/hb.glsl": (root / "BrendanLauncher/sdk/include/hb.glsl").read_text(),
    "sdk/include/draw.glsl": (root / "BrendanLauncher/sdk/include/draw.glsl").read_text(),
    "scripts/toolchain.cjs": (root / "BrendanLauncher/scripts/toolchain.cjs").read_text(),
    "apps/hello-world/assets/text.bin.base64": base64.b64encode(
        (root / "BrendanLauncher/apps/hello-world/assets/text.bin").read_bytes()
    ).decode("ascii"),
}

# The platform-specific executables are required inputs to `make` and are
# embedded into Any2NS2 so generated projects carry their own shader toolchain.
for tool in ("glslangValidator", "spirv-val"):
    source = tool_root / f"{tool}{suffix}"
    if not source.is_file():
        raise SystemExit(
            f"missing required shader tool: {source.name} (place it in the Any2NS2 source directory)"
        )
    assets[f"tools/{tool}{suffix}.base64"] = base64.b64encode(source.read_bytes()).decode("ascii")

lines = ["#pragma once", "#include <map>", "#include <string>", "namespace any2ns2_embedded {", "inline const std::map<std::string, std::string>& assets() {", "    static const std::map<std::string, std::string> value = {"]
for name, content in assets.items():
    lines.append(f"        {{{json.dumps(name)}, {json.dumps(content)}}},")
lines += ["    };", "    return value;", "}", "}"]
out.write_text("\n".join(lines) + "\n")
