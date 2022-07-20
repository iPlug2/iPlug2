
BUILD_DIR := build-linux
BUILD_CONFIG := Debug

CMAKE_BUILD_CMD := cmake --build $(BUILD_DIR) --config $(BUILD_CONFIG) --target

app: cmake_setup
	$(CMAKE_BUILD_CMD) App

app/run: app
	./$(BUILD_DIR)/IPlugControls-app/IPlugControls

vst2: cmake_setup
	$(CMAKE_BUILD_CMD) VST2

vst3: cmake_setup
	$(CMAKE_BUILD_CMD) VST3

cmake_setup:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

