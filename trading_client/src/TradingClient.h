#pragma once

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>
#include <websocketpp/endpoint.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

class TradingClient {
public:
    using WsClient = websocketpp::client<websocketpp::config::asio_client>;
    using ConnectionHandle = websocketpp::connection_hdl;
    using MessageHandler = std::function<void(const std::string&)>;
    using ConnectionStatusHandler = std::function<void(bool)>;
    using MessagePtr = WsClient::message_ptr;

    TradingClient(const std::string& uri = "ws://localhost:9002");
    ~TradingClient();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Message handlers
    void setMessageHandler(MessageHandler handler);
    void setConnectionStatusHandler(ConnectionStatusHandler handler);

    // Trading operations
    bool placeOrder(const std::string& symbol, 
                   const std::string& orderType,
                   const std::string& side,
                   double quantity,
                   double price = 0.0);

    bool subscribeToMarketData(const std::string& symbol);
    bool unsubscribeFromMarketData(const std::string& symbol);

private:
    // WebSocket client instance
    WsClient client_;
    std::string uri_;
    ConnectionHandle connection_;
    std::atomic<bool> connected_;
    std::mutex mutex_;
    std::condition_variable cv_;

    // Message handling
    MessageHandler message_handler_;
    ConnectionStatusHandler connection_status_handler_;
    std::queue<std::string> message_queue_;
    std::thread message_thread_;
    std::atomic<bool> running_;

    // Internal methods
    void setupClient();
    void onOpen(ConnectionHandle hdl);
    void onClose(ConnectionHandle hdl);
    void onMessage(ConnectionHandle hdl, WsClient::message_ptr msg);
    void onError(ConnectionHandle hdl);
    void processMessageQueue();
    void sendMessage(const std::string& message);
}; 