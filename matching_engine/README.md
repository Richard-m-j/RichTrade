# Cryptocurrency Matching Engine

A high-performance, REG NMS-compliant cryptocurrency matching engine written in C++.

## Features
- Price-time priority matching
- Support for MARKET, LIMIT, IOC, FOK orders
- REST API for order submission (cpp-httplib)
- WebSocket market data streaming (websocketpp)
- Thread-safe, high-throughput design
- Structured logging and audit trail
- Comprehensive unit tests (Google Test)

## Directory Structure
```
matching_engine/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── core/
│   ├── api/
│   ├── utils/
│   └── main.cpp
├── tests/
└── external/
```

## Build Instructions
1. Clone the repository and install dependencies:
   - nlohmann/json
   - cpp-httplib
   - websocketpp
   - Google Test
2. Build with CMake:
   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```
3. Run the application:
   ```sh
   ./matching_engine
   ```
4. Run tests:
   ```sh
   ./tests/test_matching_engine
   ```

## Usage Example
Submit an order via REST:
```
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{"symbol":"BTC-USDT","order_type":"limit","side":"buy","quantity":"1.5","price":"50000.00"}'
```

## License
MIT 