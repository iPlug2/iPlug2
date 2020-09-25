
BUILD_DIR := build-linux
BUILD_CONFIG := Debug

VST3_SRC := $(BUILD_DIR)/IPlugInstrument.vst3
VST3_DST := $$HOME/.vst3/IPlugInstrument.vst3
VST2_SRC := $(BUILD_DIR)/IPlugInstrument-vst2
VST2_DST := $$HOME/.vst/IPlugInstrument-vst2

CMAKE_BUILD_CMD := cmake --build $(BUILD_DIR) --config $(BUILD_CONFIG) --target

app: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-app

app/run: app
	./$(BUILD_DIR)/IPlugInstrument-app/IPlugInstrument

vst2: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-vst2

vst2/install: vst2
	-rm -r $(VST2_DST)
	cp -r $(VST2_SRC) $(VST2_DST)

vst3: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-vst3

vst3/install: vst3
	-rm -r $(VST3_DST)
	cp -r $(VST3_SRC) $(VST3_DST)

cmake_setup:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

