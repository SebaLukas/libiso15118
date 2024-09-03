// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <iso15118/tbd_controller.hpp>

#include <iso15118/detail/io/socket_helper.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>

#include <iostream>

// #include <iso15118/io/connection_plain.hpp>
// #include <iso15118/session/iso.hpp>
// #ifdef LIBISO_OPENSSL
// #include <iso15118/io/connection_ssl.hpp>
// using ConnectionType = iso15118::io::ConnectionSSL;
// #else
// #include <iso15118/io/connection_tls.hpp>
// using ConnectionType = iso15118::io::ConnectionTLS;
// #endif
#include <iso15118/io/connection_plain_client.hpp>

#include <iso15118/detail/helper.hpp>

namespace iso15118 {

TbdController::TbdController(TbdConfig config_, session::feedback::Callbacks callbacks_) :
    config(std::move(config_)), callbacks(std::move(callbacks_)), sdp_client(config.interface_name) {
    sdp_client.init();
    poll_manager.register_fd(sdp_client.get_fd(), [this]() { handle_sdp_client_input(); });
    session_config = d20::SessionConfig();
}

void TbdController::start_session() {
    static constexpr auto POLL_MANAGER_TIMEOUT_MS = 50;

    auto next_event = get_current_time_point();

    // TODO(sl): After x ms send again + After 50x shutdown

    // switch (config.tls_negotiation_strategy) {
    // case config::TlsNegotiationStrategy::ACCEPT_CLIENT_OFFER:
    //     // nothing to change
    //     break;
    // case config::TlsNegotiationStrategy::ENFORCE_TLS:
    //     request.security = io::v2gtp::Security::TLS;
    //     break;
    // case config::TlsNegotiationStrategy::ENFORCE_NO_TLS:
    //     request.security = io::v2gtp::Security::NO_TRANSPORT_SECURITY;
    //     break;
    // }
    sdp_client.send_request();

    bool session_active{true};

    while (session_active) {
        const auto poll_timeout_ms = get_timeout_ms_until(next_event, POLL_MANAGER_TIMEOUT_MS);
        poll_manager.poll(poll_timeout_ms);

        next_event = offset_time_point_by_ms(get_current_time_point(), POLL_MANAGER_TIMEOUT_MS);

        for (auto& session : sessions) {
            const auto next_session_event = session.poll();
            next_event = std::min(next_event, next_session_event);
        }
    }

    logf("Shutdown EVCC");
}

void TbdController::send_control_event(const d20::ControlEvent& event) {
    if (sessions.size() > 1) {
        logf("Inconsistent state, sessions.size() > 1 -- dropping control event");
        return;
    } else if (sessions.size() == 0) {
        return;
    }

    sessions.front().push_control_event(event);
}

// Should be called once
void TbdController::setup_config() {
}

// Should be called before every session
void TbdController::setup_session(const std::vector<message_20::Authorization>& auth_services,
                                  bool cert_install_service) {

    if (auth_services.empty() == false) {
        session_config.authorization_services = auth_services;
    } else {
        session_config.authorization_services = {{message_20::Authorization::EIM}};
    }

    session_config.cert_install_service = cert_install_service;
}

void TbdController::handle_sdp_client_input() {
    auto response = sdp_client.get_sdp_response();

    if (not response) {
        logf("SDP response is not valid!"); // TODO(sl): adding reason
        return;
    }

    const auto address_name = iso15118::io::sockaddr_in6_to_name(response.address);

    std::cout << "Incoming connection from " << address_name.get() << "\n";

    // logf("Incoming connection from [%s]:%" PRIu16, address_name.get(), ntohs(address.sin6_port));

    // 0. Check Response -> Security, Transport, Ipv6 and Port ()
    // 1. Create TCP or TLS connection client (start tcp/tls handshake and check result)
    // 2. Create session if tcp/tls connection is established

    // TODO(sl): Choose between TCP and TLS

    // TODO(sl): Handle ConnectionTLS
    auto connection =
        std::make_unique<io::ConnectionPlainClient>(poll_manager, response.address, config.interface_name);

    std::cout << "adasdasd" << "\n";

    logf("TEST 6");

    std::cout << "TEST6" << "\n";

    const auto& new_session = sessions.emplace_back(std::move(connection), session_config, callbacks);

    std::cout << "TEST7" << "\n";

    // logf("TEST 7");
    sessions.front().send_sap();

    // ------------------------------------------------------------------------------------------

    // auto request = sdp_server.get_peer_request();

    // if (not request) {
    //     return;
    // }

    // switch (config.tls_negotiation_strategy) {
    // case config::TlsNegotiationStrategy::ACCEPT_CLIENT_OFFER:
    //     // nothing to change
    //     break;
    // case config::TlsNegotiationStrategy::ENFORCE_TLS:
    //     request.security = io::v2gtp::Security::TLS;
    //     break;
    // case config::TlsNegotiationStrategy::ENFORCE_NO_TLS:
    //     request.security = io::v2gtp::Security::NO_TRANSPORT_SECURITY;
    //     break;
    // }

    // auto connection = [this](bool secure_connection) -> std::unique_ptr<io::IConnection> {
    //     if (secure_connection) {
    //         return std::make_unique<ConnectionType>(poll_manager, config.interface_name, config.ssl);
    //     } else {
    //         return std::make_unique<io::ConnectionPlain>(poll_manager, config.interface_name);
    //     }
    // }(request.security == io::v2gtp::Security::TLS);

    // const auto ipv6_endpoint = connection->get_public_endpoint();

    // // Todo(sl): Check if session_config is empty
    // const auto& new_session = sessions.emplace_back(std::move(connection), session_config, callbacks);

    // sdp_server.send_response(request, ipv6_endpoint);
}

} // namespace iso15118
