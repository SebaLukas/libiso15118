#include <cstdio>

#include <iso15118/tbd_controller.hpp>

int main(int argc, char* argv[]) {

    iso15118::session::logging::set_session_log_callback(
        [](std::uintptr_t id, const iso15118::session::logging::Event& event) {});

    auto config = iso15118::TbdConfig{};
    config.interface_name = "enp2s0";

    auto callbacks = iso15118::session::feedback::Callbacks{};
    callbacks.signal = [](iso15118::session::feedback::Signal signal) {};
    callbacks.dc_charge_target = [](const iso15118::session::feedback::DcChargeTarget& charge_target) {};
    callbacks.dc_max_limits = [](const iso15118::session::feedback::DcMaximumLimits& max_limits) {};

    iso15118::TbdController controller{config, callbacks};
    controller.start_session();

    return 0;
}
