IZLauncher Windows Footage:


https://github.com/user-attachments/assets/5eb2cc0a-e529-4bbe-b355-3cb88d918ba0




any2ns2
-
Usage: any2ns2 {sourcedir} {portdir} {iconpath} {apptitle} [appdescription] [build] [--framework IZLauncher]
-
Use a PNG icon. Quote arguments containing spaces.
---
Any2NS2 standalone project bundle
---
Run Any2NS2/any2ns2 directly, or build/test from source:
  cd Any2NS2
  make
  make test
--
The bundled glslangValidator and spirv-val are Linux x86_64 and Windows x86_64 tools. 
Node.js is required only when requesting the generated shader build with the optional `build` argument. 
This is needed to build the actual Nintendo Switch 2 Homebrew Launcher port's soruce.
--
The Makefile expects Python 3 and a C++17 compiler. 
The small BrendanLauncher/sdk, scripts and hello-world asset folders are included because the Any2NS2 source 
Makefile uses them to regenerate its embedded assets; runtime use does not require them.
--
ns2jb and switch1scanmodswitch2s are being used also.
IZLauncher also
--
https://github.com/mhzsajan/ns2jb
https://github.com/byizmysh/IZ2S-Platform
https://github.com/ravensclawx/HBLauncher
