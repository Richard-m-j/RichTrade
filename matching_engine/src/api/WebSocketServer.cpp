#include "WebSocketServer.h"
#include <nlohmann/json.hpp>
#include <sstream> // Required for std::ostringstream
#include <iostream>
#include <thread>
#include "../utils/Logger.h"

using json = nlohmann::json;
using namespace std::placeholders;

WebSocketServer::WebSocketServer(std::shared_ptr<MatchingEngine> engine, uint16_t port)
    : engine_(engine)
    , port_(port)
    , running_(false)
    , strand_(std::make_shared<Strand>(io_context_.get_executor()))
{
    setupServer();
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::setupServer() {
    try {
        // Initialize ASIO
        server_.init_asio();

        // Register handlers
        registerHandlers();

        // Listen on port
        server_.listen(port_);
        server_.start_accept();
    } catch (const std::exception& e) {
        // Manually format the string for Logger::err
        std::ostringstream oss;
        oss << "Error setting up WebSocket server: " << e.what();
        Logger::err(oss.str());
        throw;
    }
}

void WebSocketServer::registerHandlers() {
    server_.set_open_handler(
        [this](ConnectionHandle hdl) { onOpen(hdl); }
    );
    
    server_.set_close_handler(
        [this](ConnectionHandle hdl) { onClose(hdl); }
    );
    
    server_.set_message_handler(
        [this](ConnectionHandle hdl, MessagePtr msg) { onMessage(hdl, msg); }
    );
    
    server_.set_fail_handler(
        [this](ConnectionHandle hdl) { onError(hdl); }
    );
}

void WebSocketServer::start() {
    if (!running_) {
        running_ = true;
        try {
            // Start the server in a separate thread
            std::thread server_thread([this]() {
                try {
                    server_.run();
                } catch (const std::exception& e) {
                    // Manually format the string for Logger::err
                    std::ostringstream oss;
                    oss << "Server error: " << e.what();
                    Logger::err(oss.str());
                    running_ = false;
                }
            });
            server_thread.detach();

            // Manually format the string for Logger::info
            std::ostringstream oss;
            oss << "WebSocket server started on port " << port_;
            Logger::info(oss.str());
        } catch (const std::exception& e) {
            // Manually format the string for Logger::err
            std::ostringstream oss;
            oss << "Error starting WebSocket server: " << e.what();
            Logger::err(oss.str());
            running_ = false;
            throw;
        }
    }
}

void WebSocketServer::stop() {
    if (running_) {
        running_ = false;
        server_.stop_listening();
        server_.stop();
        Logger::info("WebSocket server stopped"); // This one already passes a single string
    }
}

void WebSocketServer::setMessageHandler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& conn : connections_) {
        try {
            server_.send(conn.first, message, websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            // Manually format the string for Logger::err
            std::ostringstream oss;
            oss << "Error broadcasting message: " << e.what();
            Logger::err(oss.str());
        }
    }
}

void WebSocketServer::onOpen(ConnectionHandle hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_[hdl] = std::make_shared<Strand>(io_context_.get_executor());
    Logger::info("New WebSocket connection established"); // This one already passes a single string
}

void WebSocketServer::onClose(ConnectionHandle hdl) {
    cleanupConnection(hdl);
    Logger::info("WebSocket connection closed"); // This one already passes a single string
}

void WebSocketServer::onMessage(ConnectionHandle hdl, MessagePtr msg) {
    if (message_handler_) {
        try {
            message_handler_(hdl, msg->get_payload());
        } catch (const std::exception& e) {
            // Manually format the string for Logger::err
            std::ostringstream oss;
            oss << "Error handling message: " << e.what();
            Logger::err(oss.str());
            sendError(hdl, "Error processing message");
        }
    }
}

void WebSocketServer::onError(ConnectionHandle hdl) {
    Logger::err("WebSocket error occurred"); // This one already passes a single string
    cleanupConnection(hdl);
}

void WebSocketServer::cleanupConnection(ConnectionHandle hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(hdl);
    
    std::lock_guard<std::mutex> sub_lock(subscriptions_mutex_);
    subscriptions_.erase(hdl);
}

void WebSocketServer::sendError(ConnectionHandle hdl, const std::string& error_msg) {
    try {
        json error = {
            {"type", "error"},
            {"message", error_msg}
        };
        server_.send(hdl, error.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        // Manually format the string for Logger::err
        std::ostringstream oss;
        oss << "Error sending error message: " << e.what();
        Logger::err(oss.str());
    }
}

// ... rest of the implementation ...