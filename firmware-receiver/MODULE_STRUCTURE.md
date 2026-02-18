# Gate Receiver - Modular Code Structure

This ESP32 gate receiver project has been reorganized into distinct modules for better maintainability and code clarity.

## Module Structure

### Core Modules

#### 1. **ring_buffer** (`ring_buffer.h`, `ring_buffer.c`)
- **Purpose**: Generic ring buffer implementation for signal debouncing
- **Key Functions**:
  - `ringbuf_add_sample()` - Add a new sample to the buffer
  - `ringbuf_count_high()` - Count high samples in buffer
  - `ringbuf_is_majority_high()` - Check if majority of samples are high
- **Used For**: GPIO debouncing for gate status and OTA button

#### 2. **gpio_config** (`gpio_config.h`, `gpio_config.c`)
- **Purpose**: GPIO pin configuration and setup
- **Key Functions**:
  - `gpio_setup()` - Configure all GPIO pins (input/output, pullups, etc.)
- **Pin Definitions**:
  - `GATE_CMD_PIN_OUT` (GPIO2) - Gate command output
  - `GATE_STATUS_PIN_INPUT` (GPIO4) - Gate status input
  - `OTA_BUTTON_PIN_INPUT` (GPIO0) - OTA button input

#### 3. **state_machine** (`state_machine.h`, `state_machine.c`)
- **Purpose**: Main application state management
- **States**:
  - `STATE_IDLE` - Waiting for events
  - `STATE_OPEN` - Opening gate sequence
  - `STATE_TOGGLE` - Toggling gate state
  - `STATE_OTA_UPDATING` - OTA update mode
- **Key Functions**:
  - `run_state_machine()` - Execute current state logic
  - `set_state()` - Change system state
  - `get_current_state()` - Get current state

#### 4. **event_processing** (`event_processing.h`, `event_processing.c`)
- **Purpose**: Handle received ESP-NOW packets and signal analysis
- **Key Functions**:
  - `process_event()` - Main event processor
  - `update_rssi_history()` - Track signal strength over time
  - `is_getting_closer()` - Detect if transmitter is approaching
- **Features**:
  - Rolling code validation
  - Proximity detection based on RSSI trends
  - Command processing (ping, force open)

#### 5. **espnow_config** (`espnow_config.h`, `espnow_config.c`)
- **Purpose**: ESP-NOW communication setup and packet handling
- **Key Functions**:
  - `espnow_setup()` - Initialize ESP-NOW and event queue
  - `receive_cb()` - ESP-NOW receive callback
- **Packet Format**: Version, rolling code, command

#### 6. **nvs_config** (`nvs_config.h`, `nvs_config.c`)
- **Purpose**: Non-volatile storage management for rolling codes
- **Key Functions**:
  - `load_expected_rolling_code()` - Load rolling code from NVS on boot
  - `save_expected_rolling_code()` - Save rolling code to NVS
- **Storage**: Handles replay protection by persisting expected rolling codes

#### 7. **ota_module** (`ota_module.h`, `ota_module.c`)
- **Purpose**: Over-the-air firmware update functionality
- **Key Functions**:
  - `ota_setup()` - Switch to AP mode and start HTTP server
  - `ota_teardown()` - Return to normal operation mode
  - `ota_task()` - HTTP server task for firmware uploads
- **Features**:
  - Web-based firmware upload interface
  - Automatic mode switching based on button press

### Main Application (`main.c`, `main.h`)
- **Purpose**: Application entry point and main loop coordination
- **Responsibilities**:
  - System initialization
  - Module coordination
  - Main execution loop
  - Shared variable definitions

## Key Features

### Signal Processing
- **Debounced GPIO Reading**: Uses ring buffers for reliable signal detection
- **RSSI-Based Proximity Detection**: Analyzes signal strength trends to detect approaching transmitters
- **Rolling Code Security**: Implements replay protection using monotonic counters

### State Management  
- **Clean State Transitions**: Well-defined states with clear transition logic
- **Timing Controls**: Cooldown periods prevent unwanted rapid state changes
- **Event-Driven Architecture**: Responds to ESP-NOW packets and GPIO events

### Over-the-Air Updates
- **Button-Activated OTA**: Hold button to enter update mode
- **Web Interface**: Simple HTML upload form
- **Safe Mode Switching**: Cleanly transitions between normal and OTA modes

## Build Configuration

The `CMakeLists.txt` includes all module source files:
```cmake
idf_component_register(
    SRCS "espnow_config.c" "nvs_config.c" "gpio_config.c" 
         "state_machine.c" "event_processing.c" "ring_buffer.c" 
         "ota_module.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash
)
```

## Usage

1. **Normal Operation**: Device waits for ESP-NOW packets and processes them
2. **Proximity Detection**: Automatically opens gate when transmitter approaches
3. **Manual Control**: Force open command via ESP-NOW
4. **OTA Updates**: Hold GPIO0 button to enter update mode at 192.168.4.1

## Benefits of Modular Structure

1. **Maintainability**: Each module has a single, well-defined responsibility
2. **Testability**: Modules can be tested independently
3. **Reusability**: Generic modules like ring_buffer can be reused
4. **Readability**: Code is organized logically with clear interfaces
5. **Debugging**: Issues can be isolated to specific modules
6. **Collaboration**: Different developers can work on different modules