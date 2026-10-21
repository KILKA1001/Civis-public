#include "civ5/runtime/normalised_bnw_state.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

// A diagnostic dump of an imported live BNW world. No gameplay command,
// cache repair, RNG consumption, persistence or readiness promotion occurs.
int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4)
            throw std::invalid_argument(
                "Usage: civis_bnw_state_dump <BNW save> <game root> [active player]");
        std::uint32_t active_player = 0;
        if (argc == 4) {
            const std::string_view text(argv[3]);
            const auto parsed = std::from_chars(
                text.data(), text.data() + text.size(), active_player);
            if (text.empty() || parsed.ec != std::errc{} ||
                parsed.ptr != text.data() + text.size())
                throw std::invalid_argument("Active player must be an unsigned integer");
        }
        auto bootstrap = civ5::ui::bootstrap_selected_bnw_preview(
            argv[1], argv[2], active_player, true);
        if (bootstrap.preview.blocker)
            throw std::runtime_error(bootstrap.preview.blocker->reason);
        if (!bootstrap.preview.host)
            throw std::runtime_error("Imported BNW session is unavailable");
        const auto state = civ5::runtime::normalise_bnw_state(
            bootstrap.preview.host->session());
        std::cout << state.json();
        if (!std::cout)
            throw std::runtime_error("Failed to write normalised state");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
