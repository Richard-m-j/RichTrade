#pragma once

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>
#include <websocketpp/endpoint.hpp>
#include "WebSocketConfig.h"
#include <set>
#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "../core/MatchingEngine.h"
#include "../core/OrderBook.h"
#include "../utils/Logger.h"
#include <unordered_map>
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

// Custom hash function for ConnectionHandle
struct ConnectionHandleHash {
    std::size_t operator()(const websocketpp::connection_hdl& hdl) const {
        return std::hash<void*>()(hdl.lock().get());
    }
};

// Custom equality function for ConnectionHandle
struct ConnectionHandleEqual {
    bool operator()(const websocketpp::connection_hdl& lhs, const websocketpp::connection_hdl& rhs) const {
        return lhs.lock().get() == rhs.lock().get();
    }
};

class WebSocketServer {
public:
    // Use the server type directly from websocketpp
    using WsServer = websocketpp::server<websocketpp::config::asio>;
    using ConnectionHandle = websocketpp::connection_hdl;
    using MessageHandler = std::function<void(ConnectionHandle, const std::string&)>;
    using MessagePtr = WsServer::message_ptr;
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    WebSocketServer(std::shared_ptr<MatchingEngine> engine, uint16_t port = 9002);
    ~WebSocketServer();

    // Start the WebSocket server
    void start();
    void stop();
    void setMessageHandler(MessageHandler handler);
    void broadcast(const std::string& message);

    // Market data and trade streaming
    void broadcastMarketData(const std::string& symbol);
    void broadcastTrade(const Trade& trade);
    void handleSubscription(ConnectionHandle hdl, const nlohmann::json& msg);
    void handleUnsubscription(ConnectionHandle hdl, const nlohmann::json& msg);

private:
    // Server instance and configuration
    WsServer server_;
    std::shared_ptr<MatchingEngine> engine_;
    uint16_t port_;
    MessageHandler message_handler_;
    std::atomic<bool> running_;

    // Connection and subscription management
    std::mutex connections_mutex_;
    std::unordered_map<ConnectionHandle, 
                      std::shared_ptr<Strand>, 
                      ConnectionHandleHash,
                      ConnectionHandleEqual> connections_;
    std::mutex subscriptions_mutex_;
    std::map<ConnectionHandle, 
             std::set<std::string>, 
             std::owner_less<ConnectionHandle>> subscriptions_;

    // WebSocket event handlers
    void onOpen(ConnectionHandle hdl);
    void onClose(ConnectionHandle hdl);
    void onMessage(ConnectionHandle hdl, MessagePtr msg);
    void onError(ConnectionHandle hdl);

    // Helper methods
    void setupServer();
    void registerHandlers();
    bool validateSubscriptionMessage(const nlohmann::json& msg, std::string& error);
    void sendError(ConnectionHandle hdl, const std::string& error_msg);
    void cleanupConnection(ConnectionHandle hdl);

    boost::asio::io_context io_context_;
    std::shared_ptr<Strand> strand_;
}; 