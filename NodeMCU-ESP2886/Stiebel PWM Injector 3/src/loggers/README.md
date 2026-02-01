# Logger Classes Overview

This project includes several logger classes to provide flexible and robust logging across different interfaces. Each logger is designed to handle specific output channels, making it easy to monitor, debug, and analyze the system both locally and remotely.

## Logger Classes

### 1. SerialLogger

**Purpose:**  
Outputs log messages to the serial console for direct monitoring and debugging via USB or UART.

**Key Responsibilities:**
- Printing log messages to the serial port
- Formatting messages for readability
- Useful during development and troubleshooting

**Main Files:**  
- `src/loggers/SerialLogger.cpp`  
- `src/loggers/SerialLogger.h`

---

### 2. TelnetLogger

**Purpose:**  
Provides remote logging over Telnet, allowing users to view logs from any device on the network.

**Key Responsibilities:**
- Managing Telnet connections
- Sending log messages to connected clients
- Handling network errors and reconnections

**Main Files:**  
- `src/loggers/TelnetLogger.cpp`  
- `src/loggers/TelnetLogger.h`

---

### 3. WebLogger

**Purpose:**  
Displays log messages on the web interface, with advanced filtering to show only relevant updates and status changes.

**Key Responsibilities:**
- Storing log history for web display
- Filtering detail lines based on state changes, sensor deltas, and intervals
- Providing real-time log updates in the browser

**Main Files:**  
- `src/loggers/WebLogger.cpp`  
- `src/loggers/WebLogger.h`

---

### 4. TelnetBridge

**Purpose:**  
Acts as a bridge for Telnet communication, supporting multiple clients and forwarding log messages.

**Key Responsibilities:**
- Managing multiple Telnet sessions
- Relaying log messages between system and clients

**Main Files:**  
- `src/loggers/TelnetBridge.cpp`  
- `src/loggers/TelnetBridge.h`

---

### 5. Logger Base Class

**Purpose:**  
Defines the common interface and base functionality for all logger types.

**Key Responsibilities:**
- Standardizing log message formatting
- Providing a unified API for logging across different outputs

**Main Files:**  
- `src/loggers/Logger.h`

---

## Additional Notes

- Loggers are coordinated by the `LogManager` class, which routes messages to the appropriate output channels.
- The modular logger design allows for easy extension (e.g., adding new log targets).
- Filtering logic in `WebLogger` ensures only relevant information is shown to users, reducing noise and improving clarity.

## Author

Sjeiz / Cheizoo
