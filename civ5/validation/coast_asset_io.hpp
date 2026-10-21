#pragma once
#include "civ5/terrain/coast_assets.hpp"
#include "coast_strip_io.hpp"
namespace {
void string(Writer &out, std::string_view value) {
    out.blob(std::span(reinterpret_cast<const std::uint8_t *>(value.data()), value.size()));
}
void write_asset(Writer &out, const civ5::assets::AssetHandle &asset, std::span<const std::uint8_t> bytes) {
    const auto &c = asset.candidate();
    const auto &p = c.provenance;
    string(out, asset.requested_key().original_name());
    string(out, c.original_name);
    out.word(p.kind == civ5::assets::SourceKind::fpk ? 1 : 0);
    string(out, p.source_id);
    string(out, p.source_relative_path);
    string(out, p.archive_relative_path.value_or(""));
    out.blob(bytes);
}
void write_sources(const CoastLibrary &lib, const fs::path &path) {
    Writer out;
    out.word(static_cast<std::uint32_t>(lib.character_assets().size() + 1));
    const auto write = [&](const civ5::assets::AssetHandle &asset) {
        write_asset(out, asset, asset.read_bytes(8 * 1024 * 1024));
    };
    write(lib.style_asset());
    for (const auto &asset : lib.character_assets())
        write(asset);
    save(path, out.bytes);
}
std::shared_ptr<const civ5::assets::AssetSource> coast_source(const fs::path &root,
                                                              const std::string &archive) {
    if (archive == "--loose")
        return std::make_shared<civ5::assets::LooseSource>(root);
    civ5::assets::AssetEnvironment environment(root);
    auto opened = environment.open_fpk_archive(archive);
    const auto *fpk = std::get_if<std::shared_ptr<civ5::assets::FpkSource>>(&opened);
    check(fpk != nullptr, "Unsupported coast source archive");
    return *fpk;
}
} // namespace
