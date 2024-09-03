// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#include <iso15118/io/sdp_client.hpp>

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>

#include <cbv2g/exi_v2gtp.h>

#include <iso15118/detail/helper.hpp>

namespace iso15118::io {

static constexpr auto LINK_LOCAL_MULTICAST = "ff02::1";
static constexpr auto SDP_SERVER_PORT = 15118;

static void log_peer_hostname(const struct sockaddr_in6& address) {
    char hostname[128];
    socklen_t hostname_len = sizeof(hostname);

    const auto get_if_name_result = getnameinfo(reinterpret_cast<const struct sockaddr*>(&address), sizeof(address),
                                                hostname, hostname_len, nullptr, 0, NI_NUMERICHOST);

    if (0 == get_if_name_result) {
        logf("Got SDP response from %s\n", hostname);
    } else {
        logf("Got SDP response, but failed to get the address\n");
    }

    logf("SDP ScopeID: %u, SDP Flowinfo: %u", address.sin6_scope_id, address.sin6_flowinfo);
}

SdpClient::~SdpClient() {

    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

void SdpClient::init() {
    fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (fd == -1) {
        log_and_throw("Failed to open socket!");
    }

    // find source port between 49152-65535
    auto could_bind = false;
    auto source_port = 49152;
    for (; source_port < 65535; source_port++) {
        sockaddr_in6 source_address = {AF_INET6, htons(source_port)};
        if (bind(fd, reinterpret_cast<sockaddr*>(&source_address), sizeof(sockaddr_in6)) == 0) {
            could_bind = true;
            break;
        }
    }

    if (!could_bind) {
        const auto error_msg = adding_err_msg("Could not bind");
        log_and_throw(error_msg.c_str());
    }

    int enable{1};
    const auto set_reuseaddr = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (set_reuseaddr == -1) {
        log_and_throw("setsockopt(SO_REUSEADDR) failed");
    }

    const auto index = if_nametoindex(interface_name.c_str());
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, sizeof(index)) < 0) {
        const auto error_msg = adding_err_msg("Could not set interface name:" + std::string(interface_name));
        log_and_throw(error_msg.c_str());
    }
}

void SdpClient::send_request() {

    uint8_t v2g_packet[V2GTP_HEADER_LENGTH + 2];
    uint8_t* sdp_request = v2g_packet + V2GTP_HEADER_LENGTH;

    sdp_request[0] = 0x10; // FIXME(sl): Config - right now set to tcp
    sdp_request[1] = 0x00; // always TCP(0x00)

    V2GTP20_WriteHeader(v2g_packet, 2, V2GTP20_SDP_REQUEST_PAYLOAD_ID);

    sockaddr_in6 sdp_server_address = {AF_INET6, htons(SDP_SERVER_PORT)};
    if (inet_pton(AF_INET6, LINK_LOCAL_MULTICAST, &sdp_server_address.sin6_addr) <= 0) {
        const auto error_msg = adding_err_msg("Failed to setup server address, reset key_log_fd");
        log_and_throw(error_msg.c_str());
    }

    // TODO(sl): Check result
    sendto(fd, v2g_packet, sizeof(v2g_packet), 0, reinterpret_cast<const sockaddr*>(&sdp_server_address),
           sizeof(sdp_server_address));
}

SdpResponse SdpClient::get_sdp_response() {
    decltype(SdpResponse::address) peer_address;
    socklen_t peer_addr_len = sizeof(peer_address);

    const auto read_result = recvfrom(fd, udp_buffer, sizeof(udp_buffer), 0,
                                      reinterpret_cast<struct sockaddr*>(&peer_address), &peer_addr_len);
    if (read_result <= 0) {
        log_and_throw("Read on sdp client socket failed");
    }

    if (peer_addr_len > sizeof(peer_address)) {
        log_and_throw("Unexpected address length during read on sdp client socket");
    }

    log_peer_hostname(peer_address);

    if (read_result == sizeof(udp_buffer)) {
        logf("Read on sdp client socket succeeded, but message is to big for the buffer");
        return SdpResponse{false};
    }

    uint32_t sdp_payload_len;
    const auto parse_sdp_result = V2GTP20_ReadHeader(udp_buffer, &sdp_payload_len, V2GTP20_SDP_RESPONSE_PAYLOAD_ID);

    if (parse_sdp_result != V2GTP_ERROR__NO_ERROR) {
        // FIXME (aw): we should not die here immediately
        logf("Sdp client received an unexpected payload");
        return SdpResponse{false};
    }

    uint8_t* sdp_buffer = udp_buffer + V2GTP_HEADER_LENGTH;

    struct sockaddr_in6 server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin6_family = AF_INET6;
    
    memcpy(&server_address.sin6_addr, sdp_buffer, sizeof(server_address.sin6_addr));

    const uint16_t port = (static_cast<uint16_t>(sdp_buffer[16]) << 8) | sdp_buffer[17];
    server_address.sin6_port = htobe16(port);

    // server_address.sin6_scope_id = 2;

    const auto security = static_cast<v2gtp::Security>(sdp_buffer[18]);
    const auto transport = static_cast<v2gtp::TransportProtocol>(sdp_buffer[19]);

    SdpResponse sdp_response{true, server_address, security, transport};
    return sdp_response;
}

} // namespace iso15118::io
