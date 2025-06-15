#include "RestServer.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../utils/Utils.h"

RestServer::RestServer(std::shared_ptr<MatchingEngine> engine, int port)
    : engine_(engine), port_(port), running_(false) {}

RestServer::~RestServer() {
    stop();
}

void RestServer::start() {
    if (!running_) {
        running_ = true;
        server_thread_ = std::thread([this]() {
            try {
                httplib::Server svr;

                // CORS preflight
                svr.Options("/orders", [](const httplib::Request& req, httplib::Response& res) {
                    res.set_header("Access-Control-Allow-Origin", "*");
                    res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                    res.set_header("Access-Control-Allow-Headers", "Content-Type");
                    res.status = 204;
                });

                svr.Post("/orders", [this](const httplib::Request& req, httplib::Response& res) {
                    auto set_cors = [&res]() {
                        res.set_header("Access-Control-Allow-Origin", "*");
                        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                        res.set_header("Access-Control-Allow-Headers", "Content-Type");
                    };
                    set_cors();
                    try {
                        auto j = nlohmann::json::parse(req.body);
                        // Validate required fields and types
                        if (!j.contains("symbol") || !j["symbol"].is_string()) {
                            res.status = 400;
                            res.set_content("{\"error\":\"Missing or invalid 'symbol' (string)\"}", "application/json");
                            Logger::err("Order rejected: missing/invalid symbol");
                            return;
                        }
                        if (!j.contains("order_type") || !j["order_type"].is_string()) {
                            res.status = 400;
                            res.set_content("{\"error\":\"Missing or invalid 'order_type' (string)\"}", "application/json");
                            Logger::err("Order rejected: missing/invalid order_type");
                            return;
                        }
                        if (!j.contains("side") || !j["side"].is_string()) {
                            res.status = 400;
                            res.set_content("{\"error\":\"Missing or invalid 'side' (string)\"}", "application/json");
                            Logger::err("Order rejected: missing/invalid side");
                            return;
                        }
                        if (!j.contains("quantity") || !(j["quantity"].is_string() || j["quantity"].is_number())) {
                            res.status = 400;
                            res.set_content("{\"error\":\"Missing or invalid 'quantity' (string or number)\"}", "application/json");
                            Logger::err("Order rejected: missing/invalid quantity");
                            return;
                        }

                        std::string symbol = j["symbol"].get<std::string>();
                        std::string order_type = Utils::toUpper(j["order_type"].get<std::string>());
                        std::string side = Utils::toUpper(j["side"].get<std::string>());
                        double quantity = 0.0;

                        try {
                            if (j["quantity"].is_string()) {
                                quantity = std::stod(j["quantity"].get<std::string>());
                            } else {
                                quantity = j["quantity"].get<double>();
                            }
                        } catch (const std::exception&) {
                            res.status = 400;
                            res.set_content("{\"error\":\"Invalid 'quantity' value\"}", "application/json");
                            Logger::err("Order rejected: invalid quantity value");
                            return;
                        }

                        if (quantity <= 0) {
                            res.status = 400;
                            res.set_content("{\"error\":\"'quantity' must be positive\"}", "application/json");
                            Logger::err("Order rejected: non-positive quantity");
                            return;
                        }

                        double price = 0.0;
                        if (j.contains("price")) {
                            try {
                                if (j["price"].is_string()) {
                                    price = std::stod(j["price"].get<std::string>());
                                } else {
                                    price = j["price"].get<double>();
                                }
                            } catch (const std::exception&) {
                                res.status = 400;
                                res.set_content("{\"error\":\"Invalid 'price' value\"}", "application/json");
                                Logger::err("Order rejected: invalid price value");
                                return;
                            }
                            if (price < 0) {
                                res.status = 400;
                                res.set_content("{\"error\":\"'price' must be non-negative\"}", "application/json");
                                Logger::err("Order rejected: negative price");
                                return;
                            }
                        }

                        Order::Type type;
                        if (order_type == "LIMIT") {
                            type = Order::Type::LIMIT;
                        } else if (order_type == "MARKET") {
                            type = Order::Type::MARKET;
                        } else if (order_type == "IOC") {
                            type = Order::Type::IOC;
                        } else if (order_type == "FOK") {
                            type = Order::Type::FOK;
                        } else {
                            res.status = 400;
                            res.set_content("{\"error\":\"Invalid 'order_type' (must be limit, market, ioc, fok)\"}", "application/json");
                            Logger::err("Order rejected: invalid order_type");
                            return;
                        }

                        Order::Side s;
                        if (side == "BUY") {
                            s = Order::Side::BUY;
                        } else if (side == "SELL") {
                            s = Order::Side::SELL;
                        } else {
                            res.status = 400;
                            res.set_content("{\"error\":\"Invalid 'side' (must be buy or sell)\"}", "application/json");
                            Logger::err("Order rejected: invalid side");
                            return;
                        }

                        std::string order_id = Utils::getCurrentTimestamp() + symbol + order_type + side; // Simple unique id
                        std::string timestamp = Utils::getCurrentTimestamp();
                        Order order(order_id, symbol, type, s, quantity, price, timestamp);

                        Logger::info("Order received: " + order_id + " " + symbol + " " + order_type + " " + side + 
                                    " qty=" + std::to_string(quantity) + " price=" + std::to_string(price));

                        auto trades = engine_->processOrder(order);
                        nlohmann::json resp;
                        resp["order_id"] = order_id;
                        resp["status"] = "success";
                        resp["message"] = "Order submitted successfully";
                        resp["executions"] = nlohmann::json::array();

                        for (const auto& t : trades) {
                            resp["executions"].push_back(nlohmann::json::parse(t.toJSON()));
                        }

                        res.status = 200;
                        res.set_content(resp.dump(), "application/json");

                    } catch (const std::exception& ex) {
                        res.status = 400;
                        std::string error_msg = "{\"error\":\"" + std::string(ex.what()) + "\"}";
                        res.set_content(error_msg, "application/json");
                        Logger::err("Order rejected: " + std::string(ex.what()));
                    }
                });

                Logger::info("REST server listening on port " + std::to_string(port_));
                if (!svr.listen("0.0.0.0", port_)) {
                    Logger::err("Failed to start REST server on port " + std::to_string(port_));
                    running_ = false;
                }
            } catch (const std::exception& e) {
                Logger::err("REST server error: " + std::string(e.what()));
                running_ = false;
            }
        });
    }
}

void RestServer::stop() {
    if (running_) {
        running_ = false;
        // The server will stop when the thread exits
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        Logger::info("REST server stopped");
    }
}

void RestServer::registerHandlers() {
    // Not used in this implementation
}