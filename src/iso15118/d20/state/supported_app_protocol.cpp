// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <iso15118/d20/state/supported_app_protocol.hpp>

#include <iso15118/d20/state/session_setup.hpp>

#include <iso15118/message/session_setup.hpp>
#include <iso15118/message/supported_app_protocol.hpp>

#include <iso15118/detail/d20/context_helper.hpp>

#include <iso15118/detail/helper.hpp>

namespace iso15118::d20::state {

static auto handle_response(const message_20::SupportedAppProtocolResponse& res) {
    message_20::SessionSetupRequest req;

    return req;

    // message_20::SupportedAppProtocolResponse res;

    // for (const auto& protocol : req.app_protocol) {
    //     if (protocol.protocol_namespace.compare("urn:iso:std:iso:15118:-20:DC") == 0) {
    //         res.schema_id = protocol.schema_id;
    //         return response_with_code(res,
    //                                   message_20::SupportedAppProtocolResponse::ResponseCode::OK_SuccessfulNegotiation);
    //     }
    // }

    // return response_with_code(res, message_20::SupportedAppProtocolResponse::ResponseCode::Failed_NoNegotiation);
}

void SupportedAppProtocol::enter() {
    ctx.log.enter_state("SupportedAppProtocol");
}

FsmSimpleState::HandleEventReturnType SupportedAppProtocol::handle_event(AllocatorType& sa, FsmEvent ev) {
    if (ev != FsmEvent::V2GTP_MESSAGE) {
        return sa.PASS_ON;
    }

    const auto variant = ctx.get_response();

    if (const auto res = variant->get_if<message_20::SupportedAppProtocolResponse>()) {

        logf("Received SupportedAppProtocolRes - ResponseCode: %u", res->response_code);

        if (res->schema_id.has_value()) {
            logf("SupportedAppProtocolRes SchemaID: %u", res->schema_id.value());
        }

        if (res->response_code != message_20::SupportedAppProtocolResponse::ResponseCode::Failed_NoNegotiation) {
            const auto req = handle_response(*res);

            ctx.request(req);
        } else {
            logf("ResponseCode: Failed!!!");
            ctx.session_stopped = true;

        }
    }

    return sa.PASS_ON;
}

} // namespace iso15118::d20::state
