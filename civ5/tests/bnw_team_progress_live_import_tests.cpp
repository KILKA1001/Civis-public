#include "civ5/runtime/bnw_team_progress_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                          std::string_view wanted) {
    for (const auto &part : team.arrays)
        if (part.name && wanted == part.name) return part;
    throw std::runtime_error("Selected BNW team technology array missing");
}
saves::PlotArrayObservation &array(saves::TeamObservation &team,
                                    std::string_view wanted) {
    for (auto &part : team.arrays)
        if (part.name && wanted == part.name) return part;
    throw std::runtime_error("Selected BNW team technology array missing");
}
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        check(argc == 3, "Supply selected Gandhi save and installed BNW root");
        const std::filesystem::path selected(argv[1]), root(argv[2]);
        const auto original_sha = runtime::identify_file(selected).sha256;
        auto initial = ui::bootstrap_selected_bnw_preview(selected, root, 0);
        if (initial.preview.blocker)
            throw std::runtime_error(initial.preview.blocker->reason);
        check(initial.preview.host != nullptr, "Selected BNW host missing");
        auto &host = *initial.preview.host;
        const auto input = host.preview_input();
        check(input.frame_sources.database && input.frame_sources.technologies,
              "Selected BNW technology rules unavailable");
        const auto &rules = *input.frame_sources.technologies;
        const auto types = input.frame_sources.database->cache_info_types();
        const auto &source = host.session().source();
        const auto &baseline = host.session().state();
        check(source.snapshot.teams && source.snapshot.teams->teams.size() == 64,
              "Selected BNW source team roster missing");
        auto draft = baseline;
        std::size_t team = 0, row = 0;
        std::int64_t slot = -1, previous = 0;
        for (std::size_t t = 0; t < source.snapshot.teams->teams.size() && slot < 0; ++t) {
            const auto &saved = source.snapshot.teams->teams[t];
            const auto &hashes = array(saved, "tech_hashes").values;
            const auto &progress = array(saved, "research_progress").values;
            for (std::size_t r = 0; r < hashes.size(); ++r) {
                if (hashes[r] <= 0 || progress[r] > INT_MAX - 100) continue;
                const auto *info = types.find_hash(static_cast<std::uint64_t>(hashes[r]));
                if (!info || info->id < 0 ||
                    !rules.find(static_cast<std::uint64_t>(info->id))) continue;
                team = t; row = r; slot = info->id; previous = progress[r]; break;
            }
        }
        check(slot >= 0, "Selected BNW has no supported research progress row");
        const auto expected = previous + 100;
        array(draft.world.teams->teams[team], "research_progress").values[row] = expected;
        draft.technologies.at(team).set_progress_times100(slot, expected);
        const auto pair = runtime::make_bnw_team_progress_checkpoint_pair(
            draft, source.snapshot, baseline.setup, baseline.technologies,
            baseline.player_research, rules, types, host.identity());
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        Cleanup cleanup{std::filesystem::current_path() /
                        ("bnw-progress-live-import-" + suffix + ".CivisSave")};
        auto document = saves::make_civis_import_document(source.source, original_sha);
        document.sections.push_back(pair.raw);
        document.sections.push_back(pair.owned);
        saves::write_civis_save(cleanup.path, document);
        auto resumed = ui::bootstrap_selected_bnw_preview(cleanup.path, root, 0);
        if (resumed.preview.blocker)
            throw std::runtime_error(resumed.preview.blocker->reason);
        check(resumed.preview.host != nullptr, "Resumed BNW host missing");
        const auto &state = resumed.preview.host->session().state();
        check(array(state.world.teams->teams[team], "research_progress").values[row] ==
                  expected &&
              state.technologies.at(team).progress_times100(slot) == expected &&
              array(baseline.world.teams->teams[team], "research_progress").values[row] ==
                  previous &&
              runtime::identify_file(selected).sha256 == original_sha &&
              !resumed.preview.host->session().readiness().can_continue(),
              "BNW live team progress import lost state or overclaimed readiness");
        std::cout << "BNW team progress live import pair PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
