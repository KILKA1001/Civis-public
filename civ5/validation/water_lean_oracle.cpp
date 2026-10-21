// Direct FPK -> production LEAN -> owned GL textures. Independent D3D9 HAL
// storage and point-sampling controls use the same authored map bytes.
#include "texture_sampling_native.hpp"
#include <set>

using namespace texture_sampling_validation;
namespace fs = std::filesystem;
namespace {
void write(const fs::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.close();
    require(bool(out), "LEAN output write");
}
Com<IDirect3DTexture9> upload(NativeSampler &native, const LeanData &data, LeanMap map) {
    Com<IDirect3DTexture9> result;
    const auto format = map == LeanMap::rgba16 ? D3DFMT_A16B16G16R16 : D3DFMT_L16;
    checked(native.device->CreateTexture(data.width(), data.height(), data.mip_count(), 0, format,
                                         D3DPOOL_MANAGED, &result.value, nullptr),
            "Authored native LEAN texture");
    for (const auto &level : data.levels(map)) {
        const auto bytes = data.payload(map, level.mip);
        D3DLOCKED_RECT lock{};
        checked(result->LockRect(level.mip, &lock, nullptr, 0), "Native LEAN upload lock");
        const bool valid = lock.pBits && lock.Pitch >= static_cast<INT>(level.row_pitch);
        if (valid)
            for (unsigned y = 0; y < level.height; ++y)
                std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            bytes.data() + std::size_t(y) * level.row_pitch, level.row_pitch);
        checked(result->UnlockRect(level.mip), "Native LEAN upload unlock");
        require(valid, "Native LEAN upload pitch");
    }
    return result;
}
Bytes read_native(IDirect3DTexture9 *texture, const LeanLevel &level) {
    Bytes result(static_cast<std::size_t>(level.length));
    D3DLOCKED_RECT lock{};
    checked(texture->LockRect(level.mip, &lock, nullptr, D3DLOCK_READONLY), "Native LEAN read lock");
    const bool valid = lock.pBits && lock.Pitch >= static_cast<INT>(level.row_pitch);
    if (valid)
        for (unsigned y = 0; y < level.height; ++y)
            std::memcpy(result.data() + std::size_t(y) * level.row_pitch,
                        static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, level.row_pitch);
    checked(texture->UnlockRect(level.mip), "Native LEAN read unlock");
    require(valid, "Native LEAN read pitch");
    return result;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 3, "Usage: civ5_water_lean_oracle asset-root fresh-output-dir");
        const fs::path root(argv[1]), out(argv[2]);
        require(!fs::exists(out), "Fresh LEAN output required");
        fs::create_directories(out);
        NativeSampler native;
        GlContext context;
        GlSampler sampler(context);
        std::ofstream device(out / "device.txt");
        device << context.description() << '\n';
        D3DADAPTER_IDENTIFIER9 adapter{};
        checked(native.api->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapter), "LEAN HAL adapter");
        device << adapter.Description << '\n' << adapter.Driver << '\n';
        device.close();
        std::ofstream report(out / "levels.tsv"), samples(out / "samples.tsv");
        report << "name\tmap\tmip\twidth\theight\toffset\tlength\trowPitch\tgradientScaleBits\tsourceSwapFlag"
                  "\tformat\n";
        samples << "name\tmap\tmip\tx\ty\td3dR\td3dG\td3dB\td3dA\tglR\tglG\tglB\tglA\n";
        unsigned mips = 0, points = 0;
        for (const auto &[archive, name] : std::initializer_list<std::pair<const char *, const char *>>{
                 {"TerrainModels.fpk", "waterbumps16.lean"},
                 {"TerrainModels.fpk", "riversmooth16.lean"},
                 {"Upgrade1Models.fpk", "riverrough_new16.lean"}}) {
            const auto relative = std::string("Resource/Common/") + archive;
            const auto opened = FpkSource::open(root / relative, relative);
            require(std::holds_alternative<std::shared_ptr<FpkSource>>(opened), "Original LEAN archive");
            const auto found = std::get<std::shared_ptr<FpkSource>>(opened)->find(AssetKey(name));
            require(std::holds_alternative<AssetHandle>(found), "Unique original LEAN entry");
            const auto data = LeanData::parse(std::get<AssetHandle>(found).read_bytes());
            write(out / (std::string(name) + ".original.bin"), data.original_bytes());
            for (auto map : {LeanMap::rgba16, LeanMap::luminance16}) {
                auto native_texture = upload(native, data, map);
                auto texture = GlTexture::lean(context, data, map);
                for (const auto &level : data.levels(map)) {
                    const auto stem = std::string(name) + ".map" +
                                      std::to_string(static_cast<unsigned>(map)) + ".mip" +
                                      std::to_string(level.mip);
                    const auto raw = data.payload(map, level.mip);
                    const auto gpu = texture.read_level_raw(0, level.mip);
                    const auto d3d = read_native(native_texture.value, level);
                    require(gpu == d3d && std::equal(gpu.begin(), gpu.end(), raw.begin(), raw.end()),
                            "Whole authored LEAN mip differs after GPU upload");
                    write(out / (stem + ".bin"), raw);
                    write(out / (stem + ".gl.bin"), gpu);
                    write(out / (stem + ".d3d.bin"), d3d);
                    report << name << '\t' << static_cast<unsigned>(map) << '\t' << level.mip << '\t'
                           << level.width << '\t' << level.height << '\t' << level.offset << '\t'
                           << level.length << '\t' << level.row_pitch << '\t' << data.gradient_scale_bits()
                           << '\t' << unsigned(data.source_swap_flag()) << '\t'
                           << (map == LeanMap::rgba16 ? 36 : 81) << '\n';
                    const std::set<unsigned> xs{0, level.width / 7, level.width / 3, level.width / 2,
                                                level.width - 1},
                        ys{0, level.height / 7, level.height / 3, level.height / 2, level.height - 1};
                    for (auto y : ys)
                        for (auto x : xs) {
                            const std::array<float, 4> coordinate{(float(x) + .5f) / float(level.width),
                                                                  (float(y) + .5f) / float(level.height), 0,
                                                                  float(level.mip)};
                            const auto expected = native.sample(native_texture.value, false, coordinate);
                            const auto actual = sampler.sample(texture, false, coordinate);
                            samples << name << '\t' << static_cast<unsigned>(map) << '\t' << level.mip << '\t'
                                    << x << '\t' << y;
                            for (const auto &value : {expected, actual})
                                for (auto channel : value)
                                    samples << '\t' << std::bit_cast<std::uint32_t>(channel);
                            samples << '\n';
                            ++points;
                        }
                    ++mips;
                }
            }
        }
        report.close();
        samples.close();
        require(bool(report) && bool(samples) && bool(device), "Final LEAN reports");
        std::cout << "LEAN original FPK maps: " << mips << " whole mips, " << points
                  << " native/GL float4 samples; compare sample report\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
