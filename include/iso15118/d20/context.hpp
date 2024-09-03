// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>

#include <iso15118/message/payload_type.hpp>
#include <iso15118/message/variant.hpp>
#include <iso15118/session/feedback.hpp>
#include <iso15118/session/logger.hpp>

#include "config.hpp"
#include "control_event.hpp"
#include "session.hpp"

namespace iso15118::d20 {

// forward declare
class ControlEventQueue;

class MessageExchange {
public:
    MessageExchange(io::StreamOutputView);

    void set_response(std::unique_ptr<message_20::Variant> new_response);
    std::unique_ptr<message_20::Variant> get_response();

    template <typename MessageType> void set_request(const MessageType& msg) {
        request_size = message_20::serialize(msg, request);
        request_available = true;
        payload_type = message_20::PayloadTypeTrait<MessageType>::type;
    }

    std::tuple<bool, size_t, io::v2gtp::PayloadType> check_and_clear_request();

private:
    // input
    std::unique_ptr<message_20::Variant> response{nullptr};

    // output
    const io::StreamOutputView request;
    size_t request_size{0};
    bool request_available{false};
    io::v2gtp::PayloadType payload_type;
};

std::unique_ptr<MessageExchange> create_message_exchange(uint8_t* buf, const size_t len);

class Context {
public:
    // FIXME (aw): bundle arguments
    Context(MessageExchange&, const std::optional<ControlEvent>&, session::feedback::Callbacks, bool&,
            session::SessionLogger&, const d20::SessionConfig&);

    std::unique_ptr<message_20::Variant> get_response();

    template <typename MessageType> void request(const MessageType& msg) {
        message_exchange.set_request(msg);
    }

    const auto& get_control_event() {
        return current_control_event;
    }

    template <typename T> T const* get_control_event() {
        if (not current_control_event.has_value()) {
            return nullptr;
        }

        if (not std::holds_alternative<T>(*current_control_event)) {
            return nullptr;
        }

        return &std::get<T>(*current_control_event);
    }

    const session::Feedback feedback;

    session::SessionLogger& log;

    Session session;

    const SessionConfig& config;

    bool& session_stopped;

private:
    const std::optional<ControlEvent>& current_control_event;
    MessageExchange& message_exchange;
};

} // namespace iso15118::d20
