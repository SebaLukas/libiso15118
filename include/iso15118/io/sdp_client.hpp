// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>

#include "sdp.hpp"

namespace iso15118::io {

static constexpr auto UDP_BUFFER_SIZE = 2048;

struct SdpResponse {
    explicit SdpResponse(bool valid_) : valid(valid_) {};
    explicit SdpResponse(bool valid_, struct sockaddr_in6 address_, v2gtp::Security security_,
                         v2gtp::TransportProtocol transport_protocol_) :
        valid(valid_), address(address_), security(security_), transport_protocol(transport_protocol_) {
    }
    struct sockaddr_in6 address;
    v2gtp::Security security;
    v2gtp::TransportProtocol transport_protocol;

    operator bool() const {
        return valid;
    }

private:
    const bool valid;
};

class SdpClient {
public:
    SdpClient() = delete;
    SdpClient(std::string interface_name_) : interface_name(interface_name_) {};
    ~SdpClient();

    void send_request();
    SdpResponse get_sdp_response();

    void init();

    auto get_fd() const {
        return fd;
    }

private:
    int fd{-1};
    std::string interface_name;
    std::uint8_t udp_buffer[UDP_BUFFER_SIZE];
};

} // namespace iso15118::io
