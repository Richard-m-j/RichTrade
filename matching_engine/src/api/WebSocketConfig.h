#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

namespace websocketpp {
namespace config {

struct asio_legacy : public asio {
    typedef asio base;

    struct transport_config : public base::transport_config {
        typedef websocketpp::concurrency::basic concurrency_type;
        typedef boost::asio::io_context::strand strand_type;
        typedef boost::asio::io_context::executor_type executor_type;
        typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
    typedef transport_config::strand_type strand_type;
    typedef transport_config::executor_type executor_type;
};

} // namespace config
} // namespace websocketpp 