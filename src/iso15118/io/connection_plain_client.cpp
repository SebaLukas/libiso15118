// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#include <iso15118/io/connection_plain_client.hpp>

#include <arpa/inet.h>
#include <cassert>
#include <net/if.h>
#include <unistd.h>

#include <iso15118/detail/helper.hpp>

namespace iso15118::io {

ConnectionPlainClient::ConnectionPlainClient(PollManager& poll_manager_, sockaddr_in6& host_address,
                                             const std::string& interface_name) :
    poll_manager(poll_manager_) {

    fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd == -1) {
        log_and_throw("Failed to create an ipv6 socket");
    }

    const auto index = if_nametoindex(interface_name.c_str());
    logf("Index: %u\n", index);
    host_address.sin6_scope_id = index; // I guess dirty?

    logf("TCP ScopeID: %u, TCP Flowinfo: %u\n", host_address.sin6_scope_id, host_address.sin6_flowinfo);

    const auto connect_result =
        connect(fd, reinterpret_cast<const struct sockaddr*>(&host_address), sizeof(host_address));
    if (connect_result == -1) {
        log_and_throw("Failed to connect to host address!");
    }

    connection_open = true;

    logf("test 5\n");

    poll_manager.register_fd(fd, [this]() { this->handle_data(); });

    logf("test 6\n");
}

void ConnectionPlainClient::set_event_callback(const ConnectionEventCallback& callback) {
    this->event_callback = callback;
}

Ipv6EndPoint ConnectionPlainClient::get_public_endpoint() const {
    return Ipv6EndPoint{};
}

void ConnectionPlainClient::write(const uint8_t* buf, size_t len) {
    assert(connection_open);

    const auto write_result = ::write(fd, buf, len);

    if (write_result == -1) {
        log_and_throw("Failed to write()");
    } else if (write_result != len) {
        log_and_throw("Could not complete write");
    }
}

ReadResult ConnectionPlainClient::read(uint8_t* buf, size_t len) {
    assert(connection_open);

    const auto read_result = ::read(fd, buf, len);
    const auto did_block = (len > 0) and (read_result != len);

    if (read_result >= 0) {
        return {did_block, static_cast<size_t>(read_result)};
    }

    // should be an error
    if (errno != EAGAIN) {
        // in case the error is not due to blocking, log it
        logf("ConnectionPlain::read failed with error code: %d", errno);
    }

    return {did_block, 0};
}

void ConnectionPlainClient::handle_data() {
    assert(connection_open);

    call_if_available(event_callback, ConnectionEvent::NEW_DATA);
}

void ConnectionPlainClient::close() {

    logf("Closing TCP Connection");

    const auto close_result = ::close(fd);

    if (close_result == -1) {
        log_and_throw("close() failed");
    }

    logf("TCP connection closed gracefully");

    connection_open = false;
    call_if_available(event_callback, ConnectionEvent::CLOSED);
}

} // namespace iso15118::io
