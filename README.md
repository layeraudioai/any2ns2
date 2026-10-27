Any2NS2 standalone project bundle (Linux x86_64)

Run HBLauncher/Any2NS2/any2ns2 directly, or build/test from source:
  cd HBLauncher/Any2NS2
  make
  make test

The bundled glslangValidator and spirv-val are Linux x86_64 tools. Node.js is
required only when requesting the optional generated shader build. The Makefile
expects Python 3 and a C++17 compiler. The small HBLauncher/sdk, scripts and
hello-world asset folders are included because the Any2NS2 source Makefile
uses them to regenerate its embedded assets; runtime use does not require them.
