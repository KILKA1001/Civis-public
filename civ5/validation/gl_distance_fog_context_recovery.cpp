#include "material_world_validation.hpp"

namespace {
using namespace civ5::assets;
using civ5::graphics::GlContext;
using civ5::graphics::GlDistanceFog;
using civ5::materials::DistanceFogAssets;

void ensure(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

DistanceFogAssets load_fog(const fs::path &root) {
    const AssetEnvironment environment(root);
    const auto data = archive(environment, "Resource/Common/MiscData.fpk");
    const auto textures = archive(environment, "Resource/DX9/MiscTextures.fpk");
    return DistanceFogAssets::load([&](const char *path) {
        const auto &source = std::string_view(path) == DistanceFogAssets::color_path ? data : textures;
        const auto found = source->find(AssetKey(fs::path(path).filename().string()));
        ensure(std::holds_alternative<AssetHandle>(found), "Missing distance fog source asset");
        return std::get<AssetHandle>(found);
    });
}
}

int main(int argc, char **argv) {
    fs::path output;
    bool created = false;
    try {
        ensure(argc == 3, "Usage: civ5_gl_distance_fog_context_recovery asset-root fresh-output");
        const fs::path root(argv[1]);
        output = argv[2];
        ensure(!fs::exists(output), "Fresh distance fog output required");
        fs::create_directories(output);
        created = true;
        const auto assets = load_fog(root);
        auto original = std::make_shared<GlContext>();
        GlDistanceFog fog(*original, assets);
        const auto authored = fog.ramp().read_mip_raw(0);
        auto replacement = std::make_shared<GlContext>();
        unsigned rejected = 0;
        const auto rejects = [&](auto action) {
            bool caught = false;
            try {
                action();
            } catch (const std::invalid_argument &) {
                caught = true;
            }
            ensure(caught, "Missing distance fog recovery guard");
            ++rejected;
        };
        rejects([&] { fog.rebuild_context(nullptr); });
        rejects([&] { fog.rebuild_context(original); });
        fog.rebuild_context(replacement);
        ensure(fog.ramp().belongs_to(*replacement), "Distance fog replacement owner mismatch");
        ensure(fog.ramp().read_mip_raw(0) == authored, "Distance fog authored ramp changed");

        GlDistanceFog moved_source(*replacement, assets);
        GlDistanceFog moved_target(std::move(moved_source));
        rejects([&] { moved_source.rebuild_context(std::make_shared<GlContext>()); });
        const auto moved_bytes = moved_target.ramp().read_mip_raw(0);
        ensure(moved_bytes == authored, "Moved distance fog changed authored ramp");

        std::ostringstream report;
        report << "{\"schema\":\"civ5-distance-fog-context-recovery-v1\",\"complete\":true,"
               << "\"mip0Bytes\":" << authored.size() << ",\"rejectedOperations\":" << rejected
               << ",\"authoredRampPreserved\":true}\n";
        blend_validation::text(output / "result.json", report.str());
        std::cout << report.str();
    } catch (const std::exception &error) {
        if (created)
            blend_validation::text(output / "failure.txt", error.what());
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
