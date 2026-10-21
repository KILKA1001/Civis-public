// Exact archived inputs selected by this validation fixture, not runtime DLC
// precedence. The C++ resolver/GL uploader never receives expected DDS bytes.
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "material_blend_fixture.hpp"
#include <iostream>
using namespace civ5::graphics;
using namespace blend_validation;
namespace {
std::shared_ptr<FpkSource> archive(const AssetEnvironment &environment, const char *name) {
    auto result = environment.open_fpk_archive(name);
    require(std::holds_alternative<std::shared_ptr<FpkSource>>(result), "Exact lighting archive missing");
    return std::get<std::shared_ptr<FpkSource>>(result);
}
void check(bool ok, const char *why, unsigned &checks) {
    ++checks;
    require(ok, why);
}
template <class F> void rejects(F f, unsigned &checks) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected lighting cube rejection", checks);
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_lighting_cube_audit asset-root fresh-output");
        const std::filesystem::path output = argv[2];
        require(!std::filesystem::exists(output), "Fresh lighting cube output required");
        AssetEnvironment environment(argv[1]);
        const auto base = archive(environment, "Resource/DX9/TerrainTextures.fpk");
        const auto upgrade = archive(environment, "Resource/DX9/Upgrade1Textures.fpk");
        unsigned calls = 0, checks = 0;
        const auto source = LightingCubeAssets::load([&](const LightingSource &request) {
            check(request.loader == LightingLoader::cube, "Cube loader route", checks);
            check(request.native_path == lighting_requests().at(++calls).native_path,
                  "Interleaved source order", checks);
            const auto name = request.native_path.substr(request.native_path.find_last_of('\\') + 1);
            const auto selected = name.ends_with("_2.dds") ? upgrade : base;
            const auto found = selected->find(AssetKey(name));
            const auto *handle = std::get_if<AssetHandle>(&found);
            require(handle != nullptr, "Exact authored lighting cube unresolved");
            return *handle;
        });
        check(calls == 10, "Ten source calls, no specscreen substitution", checks);
        auto context = std::make_shared<GlContext>();
        const auto &api = context->api();
        GlLightingCubes gpu(*context, source);
        std::filesystem::create_directories(output);
        std::string provenance;
        std::uint64_t payload_bytes = 0;
        unsigned payloads = 0;
        for (unsigned i = 0; i < 10; ++i) {
            const auto &cube = source.cubes().at(i);
            const auto &data = cube.data;
            const auto &texture = i % 2 ? gpu.shadowlight(i / 2) : gpu.sunlight(i / 2);
            check(data.width() == 128 && data.height() == 128 && data.mip_count() == 8 &&
                      data.face_count() == 6 && data.format() == DdsFormat::dxt1,
                  "Original lighting cube profile", checks);
            check(&(i % 2 ? source.shadowlight(i / 2) : source.sunlight(i / 2)) == &cube,
                  "Style resource association", checks);
            check(texture.plan().internal_format == 0x8c4d && texture.plan().compressed,
                  "Explicit sRGB BC1 storage", checks);
            GLint value = 0;
            api.GetTextureLevelParameteriv(texture.id(), 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
            check(value == 0x8c4d, "Real sRGB BC1 allocation", checks);
            api.GetTextureParameteriv(texture.id(), GL_TEXTURE_IMMUTABLE_LEVELS, &value);
            check(value == 8, "All authored cube mips allocated", checks);
            api.GetTextureParameteriv(texture.id(), GL_TEXTURE_MAX_LEVEL, &value);
            check(value == 7, "Last authored mip retained", checks);
            Bytes gpu_bytes;
            for (const auto &level : data.levels()) {
                const auto actual = texture.read_level_raw(level.face, level.mip);
                const auto expected = data.payload(level.face, level.mip);
                check(actual.size() == expected.size() &&
                          std::equal(actual.begin(), actual.end(), expected.begin()),
                      "All compressed face/mip bytes preserved", checks);
                payload_bytes += actual.size();
                ++payloads;
                gpu_bytes.insert(gpu_bytes.end(), actual.begin(), actual.end());
            }
            write(output / ("cube-" + std::to_string(i) + ".dds"), data.original_bytes());
            write(output / ("cube-" + std::to_string(i) + "-gpu.bin"), gpu_bytes);
            provenance += std::to_string(i) + ";" + cube.source.native_path + ";" +
                          cube.asset.candidate().provenance.archive_relative_path.value_or("NO_ARCHIVE") +
                          "\n";
        }
        check(payloads == 480 && payload_bytes == 656160, "Complete original cube corpus", checks);
        for (auto style : {5u, 0xffffffffu}) {
            rejects([&] { source.sunlight(style); }, checks);
            rejects([&] { source.shadowlight(style); }, checks);
            rejects([&] { gpu.sunlight(style); }, checks);
            rejects([&] { gpu.shadowlight(style); }, checks);
        }
        rejects([&] { GlLightingCubes bad(*context, LightingCubeAssets{}); }, checks);
        rejects([&] { gpu.rebuild_context(nullptr); }, checks);
        rejects([&] { gpu.rebuild_context(context); }, checks);
        auto replacement = std::make_shared<GlContext>();
        const auto before = gpu.sunlight(0).read_level_raw(0, 0);
        gpu.rebuild_context(replacement);
        check(gpu.sunlight(0).belongs_to(*replacement) &&
                  gpu.sunlight(0).read_level_raw(0, 0) == before,
              "Lighting cube replacement changed authored bytes", checks);
        replacement->check("Lighting cube replacement context");
        context->check("Authored lighting cube upload/readback controls");
        text(output / "provenance.txt", provenance);
        text(output / "execution.txt",
             context->description() + "\nPASS checks=" + std::to_string(checks) +
                 "; payloads=480; compressedBytes=656160; sRGB=true; pixelShaderNotExecuted=true\n");
        std::cout << context->description() << "\nPASS " << checks
                  << " C++ lighting cube checks; 480 payloads /656160 bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
