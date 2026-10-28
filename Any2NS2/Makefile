CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
PYTHON ?= python3
EMBEDDED := embedded_assets.hpp
HBLAUNCHER := ..
ifeq ($(OS),Windows_NT)
SHADER_TOOLS := glslangValidator.exe spirv-val.exe
else
SHADER_TOOLS := glslangValidator spirv-val
endif
ifeq ($(shell uname -s 2>/dev/null),Linux)
LDFLAGS += -static
endif
ifeq ($(OS),Windows_NT)
LDFLAGS += -static
TARGET := any2ns2.exe
else
TARGET := any2ns2
endif

.PHONY: all clean test
all: $(TARGET)

test: $(TARGET)
	$(PYTHON) tests/test_model_analysis.py

$(EMBEDDED): embed_assets.py ../BrendanLauncher/sdk/build.cjs ../BrendanLauncher/sdk/include/state.glsl ../BrendanLauncher/sdk/include/hb.glsl ../BrendanLauncher/sdk/include/draw.glsl ../BrendanLauncher/scripts/toolchain.cjs ../BrendanLauncher/apps/hello-world/assets/text.bin $(addprefix ./,$(SHADER_TOOLS))
	$(PYTHON) embed_assets.py $(HBLAUNCHER) $@

$(TARGET): main.cpp model_analyzer.hpp pipeline_analyzer.hpp $(EMBEDDED)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ main.cpp

clean:
	rm -f $(TARGET) $(TARGET).exe $(EMBEDDED)
