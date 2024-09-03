// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include "connection_abstract.hpp"

// #include <iso15118/config.hpp>
#include <iso15118/io/poll_manager.hpp>

#include <string>
#include <netinet/in.h>

namespace iso15118::io {

class ConnectionPlainClient : public IConnection {
public:
    ConnectionPlainClient(PollManager&, sockaddr_in6&, const std::string&);
    ~ConnectionPlainClient() = default;

    void set_event_callback(const ConnectionEventCallback&) final;
    Ipv6EndPoint get_public_endpoint() const final;

    void write(const uint8_t* buf, size_t len) final;
    ReadResult read(uint8_t* buf, size_t len) final;

    void close() final;

private:
    PollManager& poll_manager;

    int fd{-1};

    bool connection_open{false};

    ConnectionEventCallback event_callback{nullptr};

    void handle_data();
};

} // namespace iso15118::io
