# Any2NS2

A C++17 command-line **porting scaffold generator** for HBLauncher GPU apps. It detects a few project/framework markers (including C# + MonoGame), preserves source files, and creates an app layout accepted by the repository's shader SDK.

> **Important:** this is not an automatic MonoGame/C#/Unity/Godot runtime or source translator. The generated shaders are a small demo, not the source application's rendering or behavior. Exact 1:1 visual, input, model, audio, or functional parity cannot be promised. Read the generated `PORTING.md` and implement/verify each source feature explicitly. Retail Switch 2 hardware parity requires access to supported hardware/toolchains and comparative testing.

## Build the tool

From this directory, on a system with a C++17 compiler and GNU Make:

```sh
make
```

On Windows use GNU Make with a C++17 MinGW toolchain; the output is `any2ns2.exe`. `make clean` removes the executable and generated embedded-assets header. Linux and MinGW builds use static runtime linking; the Linux executable has no dynamic library dependencies. The executable embeds the SDK build files, template asset, and both shader tools, so it can be copied and run from any working directory; it does not need the HBLauncher checkout or shader tools on `PATH` at runtime. `make` requires `glslangValidator` and `spirv-val` in this Any2NS2 source directory (use `glslangValidator.exe` and `spirv-val.exe` on Windows; Unix filenames have no extension). These platform-matching executables are embedded into Any2NS2 and extracted into generated ports' `tools/` directory for the optional shader-build step. It is OS/architecture-specific (and Windows still relies on Windows system DLLs). Python is only needed to generate the embedded header while building Any2NS2. Node.js is needed only when requesting a shader build.

## Use

```sh
./any2ns2 <sourcedir> <portdir> <iconpath> <apptitle> [appdescription] [build]
```

`portdir` must be new or empty and must not be inside `sourcedir`. `iconpath` must point to a PNG file; Any2NS2 copies it to `assets/icon.png` instead of relying on an SDK template icon path. The supplied title and optional description are used in `app.json`; quote paths and text containing spaces. The optional final literal `build` runs `node <portdir>/build.cjs` after generation (it may also be supplied without an app description). The generated port directory includes the SDK files and template data needed by its `build.cjs`; neither generation nor shader building needs the original HBLauncher directory. The build compiles shaders, not the preserved source project.

Generated output includes `app.json`, `shaders/`, `assets/`, `content/`, `source/` (preserved project files), `source-inventory.json` (relative paths, coarse roles, byte sizes, and FNV-1a checksums), `model.json`, `model-graph.json`, `port-analysis.json`, `build.cjs`, and `PORTING.md`. `port-analysis.json` records lexical hints for common rendering, controller, and audio APIs with file/line evidence. It is intentionally analysis-only and may include false positives; it does not translate those APIs. The target notes distinguish the requested 1920x1080@60 rendering target from a runtime performance guarantee, and distinguish PCM audio output from unverified audio capture. For C# input, `model-graph.json` adds a conservative source-level graph of declared types and resolvable inheritance/member-reference edges (including collection element references); ambiguous references are left unresolved. It is a static reference graph, not runtime ownership or reachability. For detected C# projects, `model.json` is a best-effort static inventory of declared types, nested types, base type names, fields/properties (including syntactically captured initializer expressions), enum values, and recognizable `new` expressions with source locations. It is an analysis artifact only: it does not instantiate a live model, fully resolve inheritance, or translate constructors, initializers, serialization, reflection, dependency injection, runtime-generated data, asset semantics, or framework behavior. Original files are preserved for later stages. Unsupported languages receive an explicit status in `model.json`. Detection is heuristic and informational only. No source files are executed by the tool.

## Current limitations

- The model inventory is not a behavior-preserving model port; no C# compiler/CLR execution, constructor evaluation, complete semantic/type resolution, MonoGame/XNA API, sprite/font/content pipeline, arbitrary application CPU runtime, project build, source-language translation, or binary conversion is provided.
- Generated compute example reads button state and updates demo values only. It is not a generic controller adapter.
- The HBLauncher ABI exposes only documented/implemented services. It does not provide arbitrary live microphone/headset/USB-C/aux input; do not report these as supported by a port.
- Framework-specific rendering, controller semantics, persistence/backend schemas, networking, audio capture, and platform APIs need explicit ports and tests. See `PORTING.md` in generated output.
