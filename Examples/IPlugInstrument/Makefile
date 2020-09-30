
BUILD_DIR := build-linux
BUILD_CONFIG := Debug

CMAKE_BUILD_CMD := cmake --build $(BUILD_DIR) --config $(BUILD_CONFIG) --target

app: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-app

app/run: app
	./$(BUILD_DIR)/IPlugInstrument-app/IPlugInstrument

vst2: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-vst2

vst3: cmake_setup
	$(CMAKE_BUILD_CMD) IPlugInstrument-vst3

cmake_setup:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

