# RichTrade

A C++ trading system implementation with a matching engine component.

## Project Structure

- `matching_engine/`: Contains the core matching engine implementation
- `.vscode/`: VS Code configuration files

## Getting Started

More details about setup and usage will be added as the project develops.

## Development

This project uses C++. Make sure you have a C++ compiler and CMake installed on your system.

### Prerequisites
- C++ compiler (gcc/g++ or MSVC)
- CMake (version 3.10 or higher)
- Build tools (Make, Ninja, or Visual Studio)

### Building the Project
1. Clone the repository
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Configure with CMake:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   cmake --build .
   ```

### Trading Client Setup and Usage

The trading client is a separate component that interacts with the matching engine. Here's how to set it up:

1. Navigate to the trading client directory:
   ```bash
   cd trading_client
   ```

2. Create a build directory for the client:
   ```bash
   mkdir build && cd build
   ```

3. Configure the client with CMake:
   ```bash
   cmake ..
   ```

4. Build the trading client:
   ```bash
   cmake --build .
   ```

5. Run the trading client:
   ```bash
   ./TradingClient
   ```

Note: Make sure the matching engine is running before starting the trading client. The client will attempt to connect to the matching engine on the default port.

## License
// ... existing code ...
