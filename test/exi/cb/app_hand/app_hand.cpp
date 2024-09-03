#include <catch2/catch_test_macros.hpp>

#include <iso15118/message/supported_app_protocol.hpp>
#include <iso15118/message/variant.hpp>

#include <string>

#include <iostream>

using namespace iso15118;

SCENARIO("App Protocol Ser/Des") {
    GIVEN("A binary representation of an AppProtocolReq document") {

        uint8_t doc_raw[] = "\x80"
                            "\x00\xf3\xab\x93\x71\xd3\x4b\x9b\x79\xd3\x9b\xa3\x21\xd3\x4b\x9b\x79"
                            "\xd1\x89\xa9\x89\x89\xc1\xd1\x69\x91\x81\xd2\x0a\x18\x01\x00\x00\x04"
                            "\x00\x40";

        const io::StreamInputView stream_view{doc_raw, sizeof(doc_raw)};

        message_20::Variant variant(io::v2gtp::PayloadType::SAP, stream_view);

        THEN("It should be decoded succussfully") {
            REQUIRE(variant.get_type() == message_20::Type::SupportedAppProtocolReq);

            const auto& msg = variant.get<message_20::SupportedAppProtocolRequest>();

            REQUIRE(msg.app_protocol.size() == 1);

            const auto& ap = msg.app_protocol[0];

            REQUIRE(ap.version_number_major == 1);
            REQUIRE(ap.version_number_minor == 0);
            REQUIRE(ap.schema_id == 1);
            REQUIRE(ap.priority == 1);

            REQUIRE(ap.protocol_namespace == "urn:iso:std:iso:15118:-20:AC");
        }
    }

    GIVEN("Decode AppProtocolRes document") {
        uint8_t doc_raw[] = "\x80\x40\x00\x40";

        const io::StreamInputView stream_view{doc_raw, sizeof(doc_raw)};

        message_20::Variant variant(io::v2gtp::PayloadType::SAP, stream_view);

        THEN("It should be decoded succussfully") {
            REQUIRE(variant.get_type() == message_20::Type::SupportedAppProtocolRes);

            const auto& msg = variant.get<message_20::SupportedAppProtocolResponse>();

            REQUIRE(msg.response_code ==
                    message_20::SupportedAppProtocolResponse::ResponseCode::OK_SuccessfulNegotiation);
            REQUIRE(msg.schema_id.has_value() == true);
            REQUIRE(msg.schema_id.value() == 1);
        }
    }

    GIVEN("Encoed AppProtocolReq document") {
        message_20::SupportedAppProtocolRequest req;

        auto& ap = req.app_protocol.emplace_back();
        ap.version_number_major = 1;
        ap.version_number_minor = 0;
        ap.schema_id = 1;
        ap.priority = 1;
        ap.protocol_namespace = "urn:iso:std:iso:15118:-20:AC";

        uint8_t serialization_buffer[1024];
        io::StreamOutputView out({serialization_buffer, sizeof(serialization_buffer)});

        const auto size = message_20::serialize(req, out);

        THEN("It should be encoded succussfully") {
            uint8_t expected[] = {0x80, 0x00, 0xf3, 0xab, 0x93, 0x71, 0xd3, 0x4b, 0x9b, 0x79, 0xd3, 0x9b, 0xa3,
                                  0x21, 0xd3, 0x4b, 0x9b, 0x79, 0xd1, 0x89, 0xa9, 0x89, 0x89, 0xc1, 0xd1, 0x69,
                                  0x91, 0x81, 0xd2, 0x0a, 0x18, 0x01, 0x00, 0x00, 0x04, 0x00, 0x40};

            for (auto i = 0; i < sizeof(expected); i++) {
                REQUIRE(serialization_buffer[i] == expected[i]);
            }
        }
    }

    GIVEN("Encode AppProtocolRes document") {
        message_20::SupportedAppProtocolResponse res;
        res.response_code = message_20::SupportedAppProtocolResponse::ResponseCode::OK_SuccessfulNegotiation;
        res.schema_id = 1;

        uint8_t serialization_buffer[1024];
        io::StreamOutputView out({serialization_buffer, sizeof(serialization_buffer)});

        const auto size = message_20::serialize(res, out);

        THEN("It should be encoded succussfully") {
            uint8_t expected[] = {0x80, 0x40, 0x00, 0x40};

            for (auto i = 0; i < sizeof(expected); i++) {
                REQUIRE(serialization_buffer[i] == expected[i]);
            }
        }
    }

    // Todo(sl): Missing Decode message
    // 80400040
    // {"supportedAppProtocolRes": {"ResponseCode": "OK_SuccessfulNegotiation", "SchemaID": 1}}
}
