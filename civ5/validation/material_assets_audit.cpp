// Exact archived base inputs only. No original-game process or renderer.
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace civ5::assets;
using namespace civ5::materials;
using namespace civ5::graphics;
namespace {
int checks = 0;
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected rejection");
}
struct Writer {
    std::ofstream out;
    explicit Writer(const char *path) : out(path, std::ios::binary) {
        check(bool(out), "Cannot open output");
    }
    void raw(std::span<const std::uint8_t> bytes) {
        out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        check(bool(out), "Write failed");
    }
    void word(std::uint32_t v) {
        for (unsigned i = 0; i < 4; ++i)
            out.put(static_cast<char>(v >> (8 * i)));
    }
    void blob(std::span<const std::uint8_t> data) {
        check(data.size() <= 0xffffffffu, "Oversized audit blob");
        word(static_cast<std::uint32_t>(data.size()));
        raw(data);
    }
    void text(const std::string &s) { blob({reinterpret_cast<const std::uint8_t *>(s.data()), s.size()}); }
    template <class T> void values(std::span<const T> data) {
        word(static_cast<std::uint32_t>(data.size()));
        for (T v : data)
            word(std::bit_cast<std::uint32_t>(v));
    }
};
std::shared_ptr<FpkSource> archive(const AssetEnvironment &environment, const char *path) {
    auto result = environment.open_fpk_archive(path);
    check(std::holds_alternative<std::shared_ptr<FpkSource>>(result), "Exact authored archive unavailable");
    return std::get<std::shared_ptr<FpkSource>>(result);
}
// Hostile transfer state proves entry0 uses the same bounded state handling as
// raw DDS. All state restored here belongs to this own validation context.
void rgba_controls(const GlContext &context) {
    const auto &gl = context.api();
    GlRenderApi render(context);
    GLuint pbo = 0;
    render.CreateBuffers(1, &pbo);
    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    const std::array<GLenum, 12> fields{GL_PACK_ALIGNMENT,     GL_PACK_ROW_LENGTH,   GL_PACK_IMAGE_HEIGHT,
                                        GL_PACK_SKIP_PIXELS,   GL_PACK_SKIP_ROWS,    GL_PACK_SKIP_IMAGES,
                                        GL_UNPACK_ALIGNMENT,   GL_UNPACK_ROW_LENGTH, GL_UNPACK_IMAGE_HEIGHT,
                                        GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_ROWS,  GL_UNPACK_SKIP_IMAGES};
    for (auto field : fields)
        gl.PixelStorei(field, (field == GL_PACK_ALIGNMENT || field == GL_UNPACK_ALIGNMENT) ? 8 : 7);
    const std::array<GLenum, 4> bits{GL_PACK_SWAP_BYTES, GL_PACK_LSB_FIRST, GL_UNPACK_SWAP_BYTES,
                                     GL_UNPACK_LSB_FIRST};
    for (auto field : bits)
        gl.PixelStorei(field, 1);
    const std::array<GLenum, 8> compressed{
        GL_PACK_COMPRESSED_BLOCK_WIDTH,   GL_PACK_COMPRESSED_BLOCK_HEIGHT,  GL_PACK_COMPRESSED_BLOCK_DEPTH,
        GL_PACK_COMPRESSED_BLOCK_SIZE,    GL_UNPACK_COMPRESSED_BLOCK_WIDTH, GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,
        GL_UNPACK_COMPRESSED_BLOCK_DEPTH, GL_UNPACK_COMPRESSED_BLOCK_SIZE};
    const std::array<GLint, 4> block{8, 4, 1, 32};
    for (unsigned i = 0; i < compressed.size(); ++i)
        gl.PixelStorei(compressed[i], block[i % 4]);
    auto levels = material_resource_zero();
    auto texture = GlTexture::rgba8(context, levels);
    for (unsigned i = 0; i < 7; ++i)
        check(texture.read_level_raw(0, i) == levels[i].rgba, "Resource0 GPU bytes differ");
    GLint value = 0;
    gl.GetTextureParameteriv(texture.id(), GL_TEXTURE_IMMUTABLE_LEVELS, &value);
    check(value == 7, "Resource0 must have seven levels");
    gl.GetTextureParameteriv(texture.id(), GL_TEXTURE_MAX_LEVEL, &value);
    check(value == 6, "Resource0 must stop at mip6");
    gl.GetTextureLevelParameteriv(texture.id(), 6, GL_TEXTURE_WIDTH, &value);
    check(value == 4, "Last resource0 width is4");
    gl.GetTextureLevelParameteriv(texture.id(), 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
    check(value == GL_RGBA8, "Resource0 storage must be linear RGBA8");
    std::array<GLint, 4> swizzle{};
    gl.GetTextureParameteriv(texture.id(), GL_TEXTURE_SWIZZLE_RGBA, swizzle.data());
    check(swizzle == std::array<GLint, 4>{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA},
          "Resource0 alpha cannot be replaced");
    rejects([&] { GlTexture::rgba8(context, {}); });
    for (auto bad : std::vector<std::vector<Rgba8Level>>{{{0, 1, {}}},
                                                         {{1, 1, Bytes(3)}},
                                                         {{1, 1, Bytes(4)}, {1, 1, Bytes(4)}},
                                                         {{2, 2, Bytes(16)}, {2, 2, Bytes(16)}},
                                                         {{0xffffffffu, 0xffffffffu, {}}}})
        rejects([&] { GlTexture::rgba8(context, bad); });
    for (auto field : fields) {
        gl.GetIntegerv(field, &value);
        check(value == ((field == GL_PACK_ALIGNMENT || field == GL_UNPACK_ALIGNMENT) ? 8 : 7),
              "RGBA8 pixel store not restored");
    }
    for (auto field : bits) {
        gl.GetIntegerv(field, &value);
        check(value == 1, "RGBA8 swap/bit state not restored");
    }
    for (unsigned i = 0; i < compressed.size(); ++i) {
        gl.GetIntegerv(compressed[i], &value);
        check(value == block[i % 4], "RGBA8 block state not restored");
    }
    for (auto binding : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
        gl.GetIntegerv(binding, &value);
        check(value == static_cast<GLint>(pbo), "RGBA8 PBO binding not restored");
    }
    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    render.DeleteBuffers(1, &pbo);
    for (auto field : fields)
        gl.PixelStorei(field, (field == GL_PACK_ALIGNMENT || field == GL_UNPACK_ALIGNMENT) ? 4 : 0);
    for (auto field : bits)
        gl.PixelStorei(field, 0);
    for (auto field : compressed)
        gl.PixelStorei(field, 0);
    std::vector<Rgba8Level> npot{{5, 3, Bytes(60, 3)}, {2, 1, Bytes(8, 7)}, {1, 1, Bytes(4, 11)}};
    auto non_square = GlTexture::rgba8(context, npot);
    for (unsigned i = 0; i < npot.size(); ++i)
        check(non_square.read_level_raw(0, i) == npot[i].rgba, "NPOT producer chain differs");
    context.check("Material RGBA8 controls");
}
class MissingSource final : public AssetSource {
  public:
    AssetLookup find(const AssetKey &) const override { return std::monostate{}; }
    Provenance provenance() const override {
        return {SourceKind::loose, "explicit-missing", "", std::nullopt};
    }
};
class AmbiguousSource final : public AssetSource {
  public:
    AssetLookup find(const AssetKey &key) const override { return AmbiguousAsset{key, provenance(), {}}; }
    Provenance provenance() const override {
        return {SourceKind::loose, "explicit-ambiguous", "", std::nullopt};
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 3 || (argc == 4 && std::string_view(argv[3]) == "--gpu"),
              "Usage: civ5_material_assets_audit asset-root fresh-output.bin [--gpu]");
        const bool gpu = argc == 4;
        check(!std::filesystem::exists(argv[2]), "Refusing output overwrite");
        AssetEnvironment environment(argv[1]);
        const auto models = archive(environment, "Resource/Common/TerrainModels.fpk"),
                   textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
        auto lookup = models->find(AssetKey("TerrainDef.csv"));
        check(std::holds_alternative<AssetHandle>(lookup), "Exact base TerrainDef unavailable");
        auto definition = std::get<AssetHandle>(lookup);
        auto source = std::make_shared<const MaterialAssets>(MaterialAssets::load(definition, *textures));
        check(source->rows().size() == 41 && source->expansion().source_rows().size() == 53 &&
                  source->textures().size() == 72,
              "Different authored baseline");
        const auto low = archive(environment, "Resource/DX9_Low/TerrainTexturesDX9Low.fpk");
        const auto ordinary_provider = MaterialAssets::load_terrain_provider(definition, *textures);
        const auto low_provider = MaterialAssets::load_terrain_provider(definition, *textures, low.get());
        check(ordinary_provider.textures().size() == 73 && low_provider.textures().size() == 73,
              "Terrain provider texture count changed");
        std::size_t replaced = 0;
        for (const auto &texture : low_provider.textures()) {
            const auto &name = texture.asset.requested_key().original_name();
            const auto candidate = low->find(AssetKey(name));
            const auto *replacement = std::get_if<AssetHandle>(&candidate);
            const auto &actual = texture.asset.candidate();
            const auto &ordinary = ordinary_provider.texture(name).asset.candidate();
            if (replacement) {
                ++replaced;
                check(actual.provenance.archive_relative_path ==
                          replacement->candidate().provenance.archive_relative_path &&
                          actual.payload_size == replacement->candidate().payload_size &&
                          actual.payload_size < ordinary.payload_size,
                      "Low terrain texture did not replace the ordinary DDS");
            } else {
                check(std::holds_alternative<std::monostate>(candidate) &&
                          actual.provenance.archive_relative_path == ordinary.provenance.archive_relative_path &&
                          actual.payload_size == ordinary.payload_size,
                      "Absent low terrain texture did not retain the ordinary DDS");
            }
        }
        check(replaced == 33, "Terrain DX9_Low replacement count changed");
        const AmbiguousSource ambiguous_overlay;
        rejects([&] { MaterialAssets::load_terrain_provider(definition, *textures, &ambiguous_overlay); });
        rejects([&] { MaterialAssets::load(definition, MissingSource{}); });
        rejects([&] { MaterialAssets::load(definition, AmbiguousSource{}); });
        auto context = gpu ? std::make_shared<GlContext>() : nullptr;
        auto resources = gpu ? std::make_unique<GlMaterialTextures>(context, source) : nullptr;
        if (gpu) {
            std::cout << context->description() << '\n';
            rgba_controls(*context);
            rejects([&] { GlMaterialTextures invalid(nullptr, source); });
            rejects([&] { GlMaterialTextures invalid(context, nullptr); });
            rejects([&] { resources->resource(65535); });
            rejects([&] { resources->resource(73); });
            const auto zero_names = resources->bind({});
            rejects([&] { resources->bind(std::array{53}); });
            rejects([&] { resources->bind(std::array{-1}); });
            rejects([&] { resources->bind(std::array<int, 9>{}); });
            const auto &gl = context->api();
            auto active = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
            GLint previous = 0;
            gl.GetIntegerv(GL_ACTIVE_TEXTURE, &previous);
            for (unsigned slot = 0; slot < 16; ++slot) {
                active(GL_TEXTURE0 + slot);
                GLint bound = 0;
                gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
                check(static_cast<GLuint>(bound) == zero_names[slot], "Rejected input mutated GPU slots");
            }
            active(previous);
            context->check("Rejected material input preserves physical bindings");
            rejects([&] { resources->rebuild_context(nullptr); });
            rejects([&] { resources->rebuild_context(context); });
            auto replacement = std::make_shared<GlContext>();
            resources->rebuild_context(replacement);
            check(resources->resource(0).belongs_to(*replacement),
                  "Material source replacement context identity");
            context = std::move(replacement);
            context->check("Material source replacement context");
        }
        Writer out(argv[2]);
        out.text("C5MAST01");
        out.word(gpu ? 1 : 0);
        out.blob(definition.read_bytes());
        out.word(static_cast<std::uint32_t>(source->rows().size()));
        for (const auto &row : source->rows()) {
            out.word(static_cast<std::uint32_t>(row.continent));
            for (auto name : {row.diffuse, row.height, row.specular})
                out.text(name.value_or(""));
            out.word(std::bit_cast<std::uint32_t>(row.dominance));
            out.word(std::bit_cast<std::uint32_t>(row.scale));
            out.word(static_cast<std::uint32_t>(row.projection));
            out.word(row.local_id);
        }
        out.values(source->expansion().source_rows());
        out.values(source->expansion().source_modes());
        out.values(source->expansion().base_slots());
        std::array<float, 384> coefficients{};
        coefficients.fill(-123.25f);
        source->write_constants(2070.375f, coefficients);
        out.values(std::span<const float>(coefficients));
        out.word(static_cast<std::uint32_t>(source->textures().size()));
        std::uint16_t id = 0;
        for (const auto &texture : source->textures()) {
            ++id;
            out.text(texture.asset.candidate().normalized_name);
            out.blob(texture.data.original_bytes());
            out.word(texture.data.mip_count());
            check(&source->texture(texture.asset.candidate().original_name) == &texture,
                  "Authored resource alias differs");
            for (const auto &level : texture.data.levels()) {
                out.word(level.width);
                out.word(level.height);
                if (gpu)
                    out.blob(resources->resource(id).read_level_raw(0, level.mip));
                else
                    out.blob(texture.data.payload(0, level.mip));
            }
        }
        std::vector<std::vector<std::int32_t>> selections{{}};
        for (int i = 0; i < 53; ++i)
            selections.push_back({i});
        for (int i = 0; i < 53; i += 8) {
            std::vector<std::int32_t> selected;
            for (int j = i; j < std::min(i + 8, 53); ++j)
                selected.insert(selected.begin(), j);
            selections.push_back(selected);
        }
        selections.push_back({1, 1, 1, 1, 1, 1, 1, 1});
        out.word(static_cast<std::uint32_t>(selections.size()));
        for (const auto &selected : selections) {
            out.values(std::span<const std::int32_t>(selected));
            const auto ids = source->selected_resources(selected);
            for (auto slot : ids)
                out.word(slot);
            if (gpu) {
                const auto names = resources->bind(selected);
                const auto repeated = resources->bind(selected);
                check(names == repeated, "Material GPU cache identity changed");
                const auto &gl = context->api();
                auto active =
                    reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
                GLint previous = 0;
                gl.GetIntegerv(GL_ACTIVE_TEXTURE, &previous);
                for (unsigned slot = 0; slot < 16; ++slot) {
                    check(names[slot] != 0 && names[slot] == resources->resource(ids[slot]).id(),
                          "Missing original material entry0 or wrong selected row");
                    active(GL_TEXTURE0 + slot);
                    GLint bound = 0;
                    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
                    check(static_cast<GLuint>(bound) == names[slot],
                          "Physical material texture binding differs");
                }
                active(previous);
                context->check("Selected material source physical binding");
            }
        }
        const auto zero = material_resource_zero();
        out.word(7);
        for (unsigned i = 0; i < 7; ++i) {
            out.word(zero[i].width);
            out.word(zero[i].height);
            if (gpu)
                out.blob(resources->resource(0).read_level_raw(0, i));
            else
                out.blob(zero[i].rgba);
        }
        rejects([&] { source->selected_resources(std::array{53}); });
        rejects([&] { source->selected_resources(std::array{-1}); });
        if (gpu) {
            std::weak_ptr<const MaterialAssets> retained = source;
            std::weak_ptr<GlContext> retained_context = context;
            source.reset();
            context.reset();
            check(!retained.expired() && !retained_context.expired(),
                  "Material GPU owner did not retain dependencies");
            check(resources->resource(0).read_level_raw(0, 6) == zero[6].rgba,
                  "Retained material context read failed");
            resources.reset();
            check(retained.expired() && retained_context.expired(), "Material dependencies leaked");
        }
        out.out.close();
        check(bool(out.out), "Output close failed");
        std::cout << "PASS " << checks << " authored material audit checks; GPU=" << gpu
                  << "; no scene/shader parity claim\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
