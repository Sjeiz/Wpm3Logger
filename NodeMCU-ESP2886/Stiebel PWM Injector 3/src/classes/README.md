# Manager Classes Overview

This project uses several manager classes to organize and control different subsystems of the Stiebel PWM Injector 3. Each manager is responsible for a specific aspect of the system, ensuring modularity, maintainability, and clear separation of concerns.

## Manager Classes

### 1. StateManager

**Purpose:**  
Handles the overall state machine of the heat pump system. Tracks current state, manages transitions, and coordinates actions based on system status and sensor inputs.

**Key Responsibilities:**
- State transitions (e.g., idle, running, post-run)
- Timing and scheduling of state changes
- Notifying other managers of state updates

**Main Files:**  
- `src/classes/StateManager.cpp`  
- `src/classes/StateManager.h`

---

### 2. OutputManager

**Purpose:**  
Controls the PWM output signal to the pump. Manages frequency, duty cycle, and ensures correct output based on system state.

**Key Responsibilities:**
- Generating PWM signals
- Adjusting output parameters
- Enabling/disabling output as required

**Main Files:**  
- `src/classes/OutputManager.cpp`  
- `src/classes/OutputManager.h`

---

### 3. ModbusManager

**Purpose:**  
Manages Modbus TCP communication with the Stiebel ISG. Handles polling, error recovery, and data parsing.

**Key Responsibilities:**
- Polling ISG for operating status and flow rate
- Handling connection errors and automatic reinitialization
- Parsing and storing Modbus register values

**Main Files:**  
- `src/classes/ModbusManager.cpp`  
- `src/classes/ModbusManager.h`

---

### 4. LogManager

**Purpose:**  
Centralizes logging for the system. Coordinates different loggers (serial, telnet, web) and ensures relevant information is recorded and displayed.

**Key Responsibilities:**
- Managing log output to various interfaces
- Filtering and formatting log messages
- Storing log history for web display

**Main Files:**  
- `src/classes/LogManager.cpp`  
- `src/classes/LogManager.h`

---

### 5. WebServerManager

**Purpose:**  
Implements the web interface for configuration, status monitoring, and log viewing.

**Key Responsibilities:**
- Serving web pages and API endpoints
- Handling user interactions via browser
- Displaying real-time system status and logs

**Main Files:**  
- `src/classes/WebServerManager.cpp`  
- `src/classes/WebServerManager.h`

---

### 6. NetworkManager

**Purpose:**  
Handles WiFi connectivity, network configuration, and OTA updates.

**Key Responsibilities:**
- Connecting to WiFi networks
- Managing network timeouts and retries
- Handling OTA firmware updates

**Main Files:**  
- `src/classes/NetworkManager.cpp`  
- `src/classes/NetworkManager.h`

---

## Additional Notes

- Each manager interacts with others via well-defined interfaces and shared data structures (e.g., `StatusInfo`).
- The modular design allows for easy extension and maintenance.
- For more details, see the respective header and source files in `src/classes/`.

## Author

Sjeiz / Cheizoo
