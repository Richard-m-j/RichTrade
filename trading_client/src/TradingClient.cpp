#include "TradingClient.h"
#include <iostream>
#include <sstream>

TradingClient::TradingClient(const std::string& uri)
    : uri_(uri), connected_(false), running_(false) {
    setupClient();
}

TradingClient::~TradingClient() {
    disconnect();
}

void TradingClient::setupClient() {
    try {
        // Set logging to be pretty verbose (everything except message payloads)
        client_.set_access_channels(websocketpp::log::alevel::all);
        client_.clear_access_channels(websocketpp::log::alevel::frame_payload);
        client_.set_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        client_.init_asio();

        // Register handlers
        client_.set_open_handler([this](ConnectionHandle hdl) { onOpen(hdl); });
        client_.set_close_handler([this](ConnectionHandle hdl) { onClose(hdl); });
        client_.set_message_handler([this](ConnectionHandle hdl, MessagePtr msg) { onMessage(hdl, msg); });
        client_.set_fail_handler([this](ConnectionHandle hdl) { onError(hdl); });
    } catch (const std::exception& e) {
        std::cerr << "Error setting up client: " << e.what() << std::endl;
        throw;
    }
}

bool TradingClient::connect() {
    if (connected_) return true;

    try {
        websocketpp::lib::error_code ec;
        WsClient::connection_ptr con = client_.get_connection(uri_, ec);
        if (ec) {
            std::cerr << "Could not create connection: " << ec.message() << std::endl;
            return false;
        }

        client_.connect(con);
        
        // Start the ASIO io_service run loop
        running_ = true;
        std::thread client_thread([this]() {
            try {
                while (running_) {
                    client_.run();
                }
            } catch (const std::exception& e) {
                std::cerr << "Client run error: " << e.what() << std::endl;
                running_ = false;
                connected_ = false;
                if (connection_status_handler_) {
                    connection_status_handler_(false);
                }
            }
        });
        client_thread.detach();

        // Start message processing thread
        message_thread_ = std::thread([this]() { processMessageQueue(); });

        // Wait for connection
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, std::chrono::seconds(5), [this]() { return connected_.load(); })) {
            std::cerr << "Connection timeout" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return false;
    }
}

void TradingClient::disconnect() {
    if (!connected_) return;

    try {
        running_ = false;
        if (message_thread_.joinable()) {
            message_thread_.join();
        }

        websocketpp::lib::error_code ec;
        client_.close(connection_, websocketpp::close::status::normal, "Client disconnecting", ec);
        if (ec) {
            std::cerr << "Error closing connection: " << ec.message() << std::endl;
        }

        connected_ = false;
        if (connection_status_handler_) {
            connection_status_handler_(false);
        }
    } catch (const std::exception& e) {
        std::cerr << "Disconnect error: " << e.what() << std::endl;
    }
}

bool TradingClient::isConnected() const {
    return connected_;
}

void TradingClient::setMessageHandler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void TradingClient::setConnectionStatusHandler(ConnectionStatusHandler handler) {
    connection_status_handler_ = std::move(handler);
}

void TradingClient::onOpen(ConnectionHandle hdl) {
    connection_ = hdl;
    connected_ = true;
    if (connection_status_handler_) {
        connection_status_handler_(true);
    }
    cv_.notify_all();
}

void TradingClient::onClose(ConnectionHandle hdl) {
    connected_ = false;
    if (connection_status_handler_) {
        connection_status_handler_(false);
    }
}

void TradingClient::onMessage(ConnectionHandle hdl, MessagePtr msg) {
    if (message_handler_) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_queue_.push(msg->get_payload());
    }
}

void TradingClient::onError(ConnectionHandle hdl) {
    connected_ = false;
    if (connection_status_handler_) {
        connection_status_handler_(false);
    }
}

void TradingClient::processMessageQueue() {
    while (running_) {
        std::string message;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!message_queue_.empty()) {
                message = message_queue_.front();
                message_queue_.pop();
            }
        }

        if (!message.empty() && message_handler_) {
            message_handler_(message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TradingClient::sendMessage(const std::string& message) {
    if (!connected_) {
        throw std::runtime_error("Not connected to server");
    }

    try {
        websocketpp::lib::error_code ec;
        client_.send(connection_, message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            throw std::runtime_error("Error sending message: " + ec.message());
        }
    } catch (const std::exception& e) {
        std::cerr << "Send error: " << e.what() << std::endl;
        throw;
    }
}

bool TradingClient::placeOrder(const std::string& symbol,
                             const std::string& orderType,
                             const std::string& side,
                             double quantity,
                             double price) {
    try {
        nlohmann::json order = {
            {"type", "order"},
            {"symbol", symbol},
            {"order_type", orderType},
            {"side", side},
            {"quantity", quantity}
        };

        if (price > 0) {
            order["price"] = price;
        }

        sendMessage(order.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error placing order: " << e.what() << std::endl;
        return false;
    }
}

bool TradingClient::subscribeToMarketData(const std::string& symbol) {
    try {
        nlohmann::json sub = {
            {"type", "subscribe"},
            {"symbol", symbol}
        };
        sendMessage(sub.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error subscribing to market data: " << e.what() << std::endl;
        return false;
    }
}

bool TradingClient::unsubscribeFromMarketData(const std::string& symbol) {
    try {
        nlohmann::json unsub = {
            {"type", "unsubscribe"},
            {"symbol", symbol}
        };
        sendMessage(unsub.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error unsubscribing from market data: " << e.what() << std::endl;
        return false;
    }
} 