// Complete owned DDS -> real C++ OpenGL upload -> full GPU readback audit.
#include "civ5/graphics/gl_texture.hpp"
#include <algorithm>
#include <bcrypt.h>
#include <iostream>
#include <stdexcept>

using namespace civ5::assets;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
fs::path path(const std::string &text) { return fs::path(std::u8string(text.begin(), text.end())); }
std::array<std::uint8_t, 32> hash(std::span<const std::uint8_t> data) {
    require(data.size() <= 0xffffffffu, "Oversized hash input");
    std::array<std::uint8_t, 32> result{};
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE state = nullptr;
    try {
        require(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0,
                "SHA provider failed");
        require(BCryptCreateHash(algorithm, &state, nullptr, 0, nullptr, 0, 0) >= 0, "SHA state failed");
        require(BCryptHashData(state, const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), 0) >=
                    0,
                "SHA input failed");
        require(BCryptFinishHash(state, result.data(), 32, 0) >= 0, "SHA finish failed");
    } catch (...) {
        if (state)
            BCryptDestroyHash(state);
        if (algorithm)
            BCryptCloseAlgorithmProvider(algorithm, 0);
        throw;
    }
    BCryptDestroyHash(state);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return result;
}
struct Writer {
    std::ofstream stream;
    explicit Writer(const fs::path &out) : stream(out, std::ios::binary) {
        require(bool(stream), "Output open failed");
    }
    void raw(std::span<const std::uint8_t> data) {
        stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        require(bool(stream), "Output write failed");
    }
    void word(std::uint32_t value) {
        std::array<std::uint8_t, 4> data{};
        for (unsigned i = 0; i < 4; ++i)
            data[i] = static_cast<std::uint8_t>(value >> (8 * i));
        raw(data);
    }
    void wide(std::uint64_t value) {
        word(static_cast<std::uint32_t>(value));
        word(static_cast<std::uint32_t>(value >> 32));
    }
    void text(const std::string &text) {
        word(static_cast<std::uint32_t>(text.size()));
        raw({reinterpret_cast<const std::uint8_t *>(text.data()), text.size()});
    }
};
AssetHandle found(AssetLookup lookup) {
    require(std::holds_alternative<AssetHandle>(lookup), "Missing/ambiguous source");
    return std::get<AssetHandle>(std::move(lookup));
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_gl_asset_audit explicit-root fresh-output.bin");
        const auto root = fs::canonical(path(argv[1])), output = path(argv[2]);
        require(!fs::exists(output), "Refusing output overwrite");
        const AssetEnvironment environment(root);
        const auto sources = environment.discover_fpk_sources();
        require(sources.unresolved.empty(), "Unresolved FPK source");
        std::vector<std::pair<std::string, AssetHandle>> entries;
        for (const auto &source : sources.supported)
            for (const auto &entry : source->entries())
                if (entry.normalized_name.ends_with(".dds"))
                    entries.emplace_back(source->provenance().source_id + "::" + entry.original_name,
                                         found(source->find(AssetKey(entry.original_name))));
        auto loose = environment.loose_source();
        std::vector<std::string> names;
        for (const auto &entry : fs::recursive_directory_iterator(root))
            if (!entry.is_symlink() && entry.is_regular_file() &&
                normalize_asset_name(path_utf8(entry.path().extension())) == ".dds")
                names.push_back(path_utf8(entry.path().lexically_relative(root)));
        std::sort(names.begin(), names.end());
        for (const auto &name : names)
            entries.emplace_back(name, found(loose->find(AssetKey(name))));
        GlContext context;
        const auto &gl = context.api();
        std::cout << context.description() << std::endl;
        Writer out(output);
        out.text("C5GLDDS1");
        out.text(context.description());
        out.word(static_cast<std::uint32_t>(entries.size()));
        std::uint64_t total_levels = 0, total_bytes = 0;
        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto &[identity, handle] = entries[index];
            try {
                auto data = DdsData::parse(handle.read_bytes(), NumericDdsProfile::legacy_d3d9,
                                           DdsHeaderProfile::civ5_d3dx_size24);
                out.text(identity);
                out.raw(hash(data.original_bytes()));
                GLuint id = 0;
                {
                    GlTexture texture(context, data);
                    id = texture.id();
                    // Binding an existing texture to an incompatible target must
                    // fail. Verify target by this core API, not GL_TEXTURE_TARGET
                    // (the current driver rejects that DSA query).
                    const auto target = data.face_count() == 6 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
                    const auto binding =
                        data.face_count() == 6 ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D;
                    GLint previous = 0, observed = 0;
                    gl.GetIntegerv(binding, &previous);
                    gl.BindTexture(target, id);
                    context.check("Actual texture target binding");
                    gl.GetIntegerv(binding, &observed);
                    require(static_cast<GLuint>(observed) == id, "GPU target binding mismatch");
                    gl.BindTexture(target, static_cast<GLuint>(previous));
                    out.word(target);
                    // Remaining values queried from GPU state, not DDS metadata.
                    for (auto key :
                         {GL_TEXTURE_IMMUTABLE_LEVELS, GL_TEXTURE_BASE_LEVEL, GL_TEXTURE_MAX_LEVEL}) {
                        GLint value = 0;
                        gl.GetTextureParameteriv(id, key, &value);
                        out.word(static_cast<std::uint32_t>(value));
                        context.check(("GPU texture metadata " + std::to_string(key)).c_str());
                    }
                    std::array<GLint, 4> swizzle{};
                    gl.GetTextureParameteriv(id, GL_TEXTURE_SWIZZLE_RGBA, swizzle.data());
                    for (auto value : swizzle)
                        out.word(static_cast<std::uint32_t>(value));
                    out.word(data.face_count());
                    out.word(data.mip_count());
                    for (std::uint32_t mip = 0; mip < data.mip_count(); ++mip) {
                        out.word(mip);
                        for (auto key : {GL_TEXTURE_WIDTH, GL_TEXTURE_HEIGHT, GL_TEXTURE_INTERNAL_FORMAT,
                                         GL_TEXTURE_COMPRESSED}) {
                            GLint value = 0;
                            gl.GetTextureLevelParameteriv(id, mip, key, &value);
                            out.word(static_cast<std::uint32_t>(value));
                            context.check(("GPU level metadata " + std::to_string(key)).c_str());
                        }
                        context.check("Actual GPU mip metadata");
                        const auto raw = texture.read_mip_raw(mip);
                        for (std::uint32_t face = 0; face < data.face_count(); ++face) {
                            const auto source = data.payload(face, mip);
                            const auto observed = std::span{raw}.subspan(face * source.size(), source.size());
                            require(std::equal(source.begin(), source.end(), observed.begin()),
                                    "Raw GPU payload mismatch");
                            out.word(face);
                            out.wide(observed.size());
                            out.raw(hash(observed));
                            ++total_levels;
                            total_bytes += observed.size();
                        }
                    }
                }
                require(gl.IsTexture(id) == GL_FALSE, "GPU texture lifetime leak");
                context.check("Own DDS upload/delete");
                if (index % 250 == 0 || index + 1 == entries.size())
                    std::cout << "PASS C++ GPU DDS " << index + 1 << '/' << entries.size()
                              << "; levels=" << total_levels << " bytes=" << total_bytes << std::endl;
            } catch (const std::exception &e) {
                throw std::runtime_error(identity + ": " + e.what());
            }
        }
        out.stream.close();
        require(bool(out.stream), "Final output close failed");
        std::cout << "PASS complete C++ GPU DDS corpus: " << entries.size() << " files / " << total_levels
                  << " levels / " << total_bytes << " bytes, no game process" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
