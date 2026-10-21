#include "civ5/assets/dds.hpp"
#include "civ5/assets/ui_block_atlas.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace civ5::assets;
namespace fs = std::filesystem;
namespace {
int checks = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected explicit rejection");
}
void put(Bytes &data, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        data.at(at + i) = static_cast<std::uint8_t>(value >> (8 * i));
}
void append(Bytes &data, std::uint32_t value) {
    const auto at = data.size();
    data.resize(at + 4);
    put(data, at, value);
}
void write(const fs::path &path, const Bytes &data) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        throw std::runtime_error("Test fixture write failed");
}
Bytes fpk(const std::vector<std::pair<std::string, Bytes>> &entries,
          const std::vector<unsigned> &padding = {}, unsigned scramble = 0) {
    Bytes result(14);
    put(result, 0, 6);
    result[4] = 'F';
    result[5] = 'P';
    result[6] = 'K';
    result[7] = '_';
    result[8] = static_cast<std::uint8_t>(scramble);
    result[9] = 0x7f;
    put(result, 10, static_cast<std::uint32_t>(entries.size()));
    std::vector<std::size_t> metadata;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto &name = entries[i].first;
        append(result, static_cast<std::uint32_t>(name.size()));
        for (unsigned char c : name)
            result.push_back(static_cast<std::uint8_t>(c + scramble));
        const auto pad = padding.empty() ? 0 : padding.at(i);
        if (pad) {
            result.push_back(static_cast<std::uint8_t>(pad));
            result.insert(result.end(), pad - 1, 0);
        }
        metadata.push_back(result.size());
        result.resize(result.size() + 16);
    }
    for (std::size_t i = 0; i < entries.size(); ++i) {
        put(result, metadata[i] + 8, static_cast<std::uint32_t>(entries[i].second.size()));
        put(result, metadata[i] + 12, static_cast<std::uint32_t>(result.size()));
        result.insert(result.end(), entries[i].second.begin(), entries[i].second.end());
    }
    return result;
}
std::shared_ptr<FpkSource> source(const fs::path &path, const std::string &relative = "Test.fpk") {
    auto result = FpkSource::open(path, relative);
    check(std::holds_alternative<std::shared_ptr<FpkSource>>(result), "Expected valid FPK");
    return std::get<std::shared_ptr<FpkSource>>(result);
}
AssetHandle found(AssetLookup lookup) {
    check(std::holds_alternative<AssetHandle>(lookup), "Expected exact source handle");
    return std::get<AssetHandle>(std::move(lookup));
}
void sources(const fs::path &directory) {
    const AssetKey key("Assets\\terrain//Pieces/./Euro_Grass.DDS");
    check(key.normalized_name() == "assets/terrain/pieces/euro_grass.dds" &&
              key.original_name() == "Assets\\terrain//Pieces/./Euro_Grass.DDS",
          "Key identity / slash normalization");
    for (const auto &name : {"../bad", "Assets/../bad", "/bad", "\\bad", "C:\\bad", "C:relative",
                             "file:stream", "//server/share/file"})
        check(!AssetKey(name).safe_relative_path(), "Unsafe resource path");
    check(!AssetKey(std::string("a\0b", 3)).safe_relative_path(), "Embedded NUL path");
    rejects([] { AssetKey blank(" \t"); });
    rejects([] { AssetKey invalid(std::string(1, static_cast<char>(255))); });
    const auto unicode = directory / fs::path(u8"Ресурсы");
    write(unicode / fs::path(u8"Текстуры/ТРАВА.dds"), {9, 8, 7});
    const LooseSource loose(unicode);
    const auto handle = found(loose.find(AssetKey(path_utf8(fs::path(u8"текстуры/трава.DDS")))));
    check(handle.read_bytes() == Bytes{9, 8, 7} && handle.candidate().provenance.kind == SourceKind::loose,
          "Unicode root and case-insensitive loose key");
    check(std::holds_alternative<std::monostate>(loose.find(AssetKey("трава.dds"))),
          "No loose basename fallback");
    auto stream = handle.open_stream();
    std::array<std::uint8_t, 8> buffer;
    buffer.fill(0xff);
    check(stream.read(std::span{buffer}.first(1)) == 1 && buffer[0] == 9 && stream.remaining() == 2,
          "Partial bounded read");
    check(stream.read({}) == 0 && stream.remaining() == 2, "Zero-length read retains cursor");
    check(stream.read(buffer) == 2 && buffer[0] == 8 && buffer[1] == 7 && buffer[2] == 255 &&
              stream.read(buffer) == 0,
          "Read stops exactly at declared end");
    check(handle.read_bytes() == Bytes{9, 8, 7}, "New stream starts from origin");
    rejects([&] { handle.read_bytes(2); });
    write(unicode / fs::path(u8"Текстуры/ТРАВА.dds"), {9});
    rejects([&] { handle.open_stream(); });
    const Bytes first{1, 0, 0, 0, 4, 5}, second{7, 8, 9};
    const auto path = directory / "direct.fpk";
    const auto direct = fpk({{"Foo.DDS", first}, {"sub/bar.bin", second}});
    write(path, direct);
    auto archive = source(path, "Resource/DX9/Test.fpk");
    check(archive->entries().size() == 2 && archive->extra_header_byte() == 127,
          "Sequential directory/header");
    check(archive->entries()[0].direct_metadata_was_valid && archive->entries()[0].padding_length == 0,
          "Direct metadata selection");
    const auto a = found(archive->find(AssetKey("foo.dds"))),
               b = found(archive->find(AssetKey("SUB\\BAR.BIN")));
    check(a.read_bytes() == first && b.read_bytes() == second, "Exact filename/payload association");
    check(a.candidate().provenance.source_id == "Resource/DX9/Test.fpk" &&
              a.candidate().original_name == "Foo.DDS",
          "Archive provenance");
    check(std::holds_alternative<std::monostate>(archive->find(AssetKey("bar.bin"))),
          "No archive basename fallback");
    for (unsigned pad : {0u, 1u, 3u, 255u})
        for (unsigned scramble : {0u, 1u}) {
            const auto target =
                directory / ("padded-" + std::to_string(pad) + "-" + std::to_string(scramble) + ".fpk");
            write(target, fpk({{"First.bin", first}, {"Next.bin", second}}, {pad, 0}, scramble));
            auto s = source(target);
            check(s->entries()[0].padding_length == pad && s->filename_scrambling() == scramble,
                  "Dummy/scrambling diagnostics");
            check(found(s->find(AssetKey("FIRST.BIN"))).read_bytes() == first &&
                      found(s->find(AssetKey("next.bin"))).read_bytes() == second,
                  "Dummy and scrambling payload identity");
        }
    const auto duplicate = directory / "duplicate.fpk";
    write(duplicate, fpk({{"Same.dds", first}, {"same.DDS", second}}));
    auto dup = source(duplicate);
    auto ambiguous = dup->find(AssetKey("SAME.dds"));
    check(std::holds_alternative<AmbiguousAsset>(ambiguous) &&
              std::get<AmbiguousAsset>(ambiguous).matches.size() == 2,
          "Source-local ambiguity");
    const auto lower_path = directory / "lower.fpk";
    write(lower_path, fpk({{"same.dds", first}, {"Foo.DDS", second}}));
    auto lower = source(lower_path);
    check(std::holds_alternative<AmbiguousAsset>(AssetResolver({dup, lower}).resolve(AssetKey("same.dds"))),
          "Ambiguity stops resolver");
    check(found(AssetResolver({archive, lower}).resolve(AssetKey("foo.dds"))).read_bytes() == first &&
              found(AssetResolver({lower, archive}).resolve(AssetKey("foo.dds"))).read_bytes() == second,
          "Caller order, not assumed DLC policy");
    rejects([] { AssetResolver invalid({nullptr}); });
    auto saved = found(source(path)->find(AssetKey("foo.dds")));
    check(saved.read_bytes() == first, "Handle survives source destruction");
    for (const auto &mutation : std::vector<std::pair<std::size_t, std::uint32_t>>{
             {0, 5},
             {10, 1000001},
             {14, 0},
             {14, 4097},
             {archive->entries()[0].metadata_offset + 8, 0xffffffff},
             {archive->entries()[0].metadata_offset + 12, 0xffffffff}}) {
        auto bad = direct;
        put(bad, mutation.first, mutation.second);
        const auto target = directory / ("bad-" + std::to_string(checks) + ".fpk");
        write(target, bad);
        check(std::holds_alternative<FpkDiagnostic>(FpkSource::open(target, "bad.fpk")),
              "Malformed FPK must be unresolved");
    }
    auto bad = direct;
    bad[8] = 2;
    write(directory / "bad-scramble.fpk", bad);
    check(std::holds_alternative<FpkDiagnostic>(FpkSource::open(directory / "bad-scramble.fpk", "bad.fpk")),
          "Unknown scrambling rejected");
    auto short_data = direct;
    short_data.resize(13);
    write(directory / "short.fpk", short_data);
    check(std::holds_alternative<FpkDiagnostic>(FpkSource::open(directory / "short.fpk", "short.fpk")),
          "Short archive rejected");
    const auto environment_root = directory / "environment";
    write(environment_root / "Resource/Good.fpk", direct);
    write(environment_root / "Resource/Bad.fpk", short_data);
    const AssetEnvironment environment(environment_root);
    auto discovered = environment.discover_fpk_sources();
    check(discovered.supported.size() == 1 && discovered.unresolved.size() == 1,
          "Separate supported/unresolved discovery");
    check(std::holds_alternative<std::shared_ptr<FpkSource>>(
              environment.open_fpk_archive("resource\\GOOD.FPK")),
          "Exact normalized archive lookup");
    check(std::holds_alternative<FpkDiagnostic>(environment.open_fpk_archive("../Good.fpk")),
          "Unsafe archive lookup rejected");
    check(std::holds_alternative<FpkDiagnostic>(environment.open_fpk_archive("Good.fpk")),
          "No archive basename fallback");
}
Bytes dds_header(std::uint32_t width, std::uint32_t height, std::uint32_t mips, std::uint32_t fourcc,
                 bool cube = false) {
    Bytes bytes(128);
    put(bytes, 0, 0x20534444);
    put(bytes, 4, 124);
    put(bytes, 8, 0x20000);
    put(bytes, 12, height);
    put(bytes, 16, width);
    put(bytes, 28, mips);
    put(bytes, 76, 32);
    put(bytes, 80, 4);
    put(bytes, 84, fourcc);
    put(bytes, 112, cube ? 0xfe00 : 0);
    return bytes;
}
void textures() {
    for (const auto code : {0x31545844u, 0x33545844u, 0x35545844u})
        for (bool cube : {false, true}) {
            const unsigned block = code == 0x31545844 ? 8 : 16;
            auto data = dds_header(8, 8, 4, code, cube);
            std::vector<Bytes> levels;
            for (unsigned face = 0; face < (cube ? 6u : 1u); ++face)
                for (unsigned mip = 0; mip < 4; ++mip) {
                    const auto size = mip == 0 ? 4 * block : block;
                    Bytes level(size, static_cast<std::uint8_t>(face * 16 + mip));
                    levels.push_back(level);
                    data.insert(data.end(), level.begin(), level.end());
                }
            auto parsed = DdsData::parse(data);
            check(parsed.original_bytes() == data && parsed.mip_count() == 4 &&
                      parsed.face_count() == (cube ? 6u : 1u),
                  "Lossless authored DDS layout");
            for (std::size_t i = 0; i < levels.size(); ++i) {
                const auto payload =
                    parsed.payload(static_cast<std::uint32_t>(i / 4), static_cast<std::uint32_t>(i % 4));
                check(Bytes(payload.begin(), payload.end()) == levels[i], "Every face/mip byte retained");
            }
            auto truncated = data;
            truncated.pop_back();
            rejects([&] { DdsData::parse(truncated); });
            auto tail = data;
            tail.push_back(0);
            rejects([&] { DdsData::parse(tail); });
            rejects([&] { parsed.payload(parsed.face_count(), 0); });
            rejects([&] { parsed.payload(0, 4); });
        }
    auto alpha = dds_header(3, 2, 2, 0);
    put(alpha, 80, 2);
    put(alpha, 88, 8);
    put(alpha, 104, 255);
    alpha.insert(alpha.end(), {0, 1, 127, 128, 254, 255, 77});
    auto a = DdsData::parse(alpha);
    check(a.format() == DdsFormat::alpha8 && a.levels()[0].row_pitch == 3 && a.payload(0, 1)[0] == 77,
          "A8/odd dimensions/last mip");
    auto rgba = dds_header(2, 1, 1, 0);
    put(rgba, 80, 0x41);
    put(rgba, 88, 32);
    put(rgba, 92, 0xff0000);
    put(rgba, 96, 0xff00);
    put(rgba, 100, 0xff);
    put(rgba, 104, 0xff000000);
    rgba.insert(rgba.end(), {1, 2, 3, 4, 5, 6, 7, 8});
    auto color = DdsData::parse(rgba);
    check(color.channel_masks() == std::array<std::uint32_t, 4>{0xff0000, 0xff00, 0xff, 0xff000000} &&
              color.payload(0, 0)[3] == 4,
          "RGBA masks and alpha unchanged");
    check(ui_texture_storage_profile(color)==UiTextureStorageProfile::ordinary_dds,
          "Ordinary DDS incorrectly requires a UI index");
    auto interface_color=rgba;
    const std::string interface_tag="FTXTInterface Scalable";
    std::copy(interface_tag.begin(),interface_tag.end(),interface_color.begin()+32);
    std::copy_n("COLOR",5,interface_color.begin()+116);
    const auto scalable=DdsData::parse(interface_color);
    check(ui_texture_storage_profile(scalable)==UiTextureStorageProfile::interface_color &&
          !has_ui_block_atlas_header(scalable) && scalable.original_bytes()==interface_color,
          "Native factor-zero COLOR must retain raw DDS without an index");
    auto packed=interface_color;
    std::copy_n("BC008",5,packed.begin()+116);
    const auto indexed=DdsData::parse(packed);
    check(ui_texture_storage_profile(indexed)==UiTextureStorageProfile::indexed_interface &&
          has_ui_block_atlas_header(indexed),"BC UI texture lost mandatory indexed profile");
    // The real base/BNW buttonsides atlas/index pair has matching BC010,
    // 16x16 BGRA atlas and 1x1 luminance index selecting tile zero. Retain
    // that geometry and also permute two distinct tiles to test addressing.
    const auto bc010_dds=[](std::uint32_t width,std::uint32_t height,std::uint32_t bpp) {
        auto data=dds_header(width,height,1,0);
        put(data,8,0x1007);put(data,80,bpp==32?0x41:0x20000);put(data,88,bpp);
        if(bpp==32) {put(data,92,0xff0000);put(data,96,0xff00);put(data,100,0xff);put(data,104,0xff000000);}
        else put(data,92,255);
        std::copy_n("FTXTInterface",13,data.begin()+32);
        std::copy_n("BC010",5,data.begin()+116);
        data.resize(128+width*height*bpp/8,0);return data;
    };
    auto atlas16=bc010_dds(32,16,32),index16=bc010_dds(2,1,8);
    index16[128]=1;index16[129]=0;
    for(std::uint32_t y=0;y<16;++y)for(std::uint32_t x=0;x<32;++x) {
        const auto at=128+(y*32+x)*4;
        atlas16[at]=static_cast<std::uint8_t>(x);
        atlas16[at+1]=static_cast<std::uint8_t>(y);
        atlas16[at+2]=static_cast<std::uint8_t>(x+y);
        atlas16[at+3]=static_cast<std::uint8_t>(200+x/16);
    }
    const auto tiles16=expand_ui_block_atlas(DdsData::parse(atlas16),DdsData::parse(index16));
    check(tiles16.width==32&&tiles16.height==16,"BC010 must address 16-pixel tiles");
    for(std::uint32_t y=0;y<16;++y)for(std::uint32_t x=0;x<32;++x) {
        const auto source_x=x<16?x+16:x-16;
        const auto src=128+(y*32+source_x)*4,dst=(y*32+x)*4;
        check(tiles16.rgba[dst]==atlas16[src+2]&&tiles16.rgba[dst+1]==atlas16[src+1]&&
              tiles16.rgba[dst+2]==atlas16[src]&&tiles16.rgba[dst+3]==atlas16[src+3],
              "BC010 tile permutation, BGRA channels or authored alpha differs");
    }
    auto single16=bc010_dds(16,16,32),single_index16=bc010_dds(1,1,8);
    for(std::size_t at=128;at<single16.size();++at)
        single16[at]=static_cast<std::uint8_t>(at&255);
    const auto single_image16=expand_ui_block_atlas(DdsData::parse(single16),DdsData::parse(single_index16));
    check(single_image16.width==16&&single_image16.height==16,"Actual BC010 single-tile geometry differs");
    for(std::size_t at=0;at<single_image16.rgba.size();at+=4)
        check(single_image16.rgba[at]==single16[128+at+2]&&single_image16.rgba[at+1]==single16[128+at+1]&&
              single_image16.rgba[at+2]==single16[128+at]&&single_image16.rgba[at+3]==single16[128+at+3],
              "Single BC010 tile changed pixels or alpha");
    const auto rejects_bc010_reason=[](auto action,const char* reason) {
        bool rejected=false;
        try {action();} catch(const std::invalid_argument& error) {
            check(std::string(error.what())==reason,"BC010 rejection came from the wrong guard");
            rejected=true;
        }
        check(rejected,"Required BC010 guard rejection missing");
    };
    auto mixed16=index16;std::copy_n("BC008",5,mixed16.begin()+116);
    rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(atlas16),DdsData::parse(mixed16));},"UI block atlas/index tile profiles differ or are unsupported");
    auto bad_geometry16=bc010_dds(24,16,32);
    rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(bad_geometry16),DdsData::parse(index16));},"Invalid FTXTInterface tile geometry");
    auto out_of_range16=index16;out_of_range16[128]=2;
    rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(atlas16),DdsData::parse(out_of_range16));},"FTXTInterface index refers outside its atlas");
    auto unsupported16=atlas16,unsupported_index16=index16;
    std::copy_n("BCFFF",5,unsupported16.begin()+116);std::copy_n("BCFFF",5,unsupported_index16.begin()+116);
    rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(unsupported16),DdsData::parse(unsupported_index16));},"UI block atlas/index tile profiles differ or are unsupported");
    // Actual matching BC020/BC040 pairs have 32/64-pixel single tiles.
    // A two-tile permutation distinguishes those factors from 16 or decimal
    // interpretations, and verifies every reconstructed channel/alpha byte.
    const auto check_large_tile=[&](const char* tag,std::uint32_t factor) {
        auto atlas=bc010_dds(2*factor,factor,32),index=bc010_dds(2,1,8);
        std::copy_n(tag,5,atlas.begin()+116);std::copy_n(tag,5,index.begin()+116);
        index[128]=1;index[129]=0;
        for(std::uint32_t y=0;y<factor;++y)for(std::uint32_t x=0;x<2*factor;++x) {
            const auto at=128+(y*2*factor+x)*4;
            atlas[at]=static_cast<std::uint8_t>(x);atlas[at+1]=static_cast<std::uint8_t>(y);
            atlas[at+2]=static_cast<std::uint8_t>(x+y);atlas[at+3]=static_cast<std::uint8_t>(200+x/factor);
        }
        const auto image=expand_ui_block_atlas(DdsData::parse(atlas),DdsData::parse(index));
        check(image.width==2*factor&&image.height==factor,"Large explicit BC tile dimensions differ");
        for(std::uint32_t y=0;y<factor;++y)for(std::uint32_t x=0;x<2*factor;++x) {
            const auto source_x=x<factor?x+factor:x-factor;
            const auto src=128+(y*2*factor+source_x)*4,dst=(y*2*factor+x)*4;
            check(image.rgba[dst]==atlas[src+2]&&image.rgba[dst+1]==atlas[src+1]&&
                  image.rgba[dst+2]==atlas[src]&&image.rgba[dst+3]==atlas[src+3],
                  "Large BC tile permutation, channels or alpha differ");
        }
        auto single=bc010_dds(factor,factor,32),single_index=bc010_dds(1,1,8);
        std::copy_n(tag,5,single.begin()+116);std::copy_n(tag,5,single_index.begin()+116);
        for(std::size_t at=128;at<single.size();++at)single[at]=static_cast<std::uint8_t>(at&255);
        const auto single_image=expand_ui_block_atlas(DdsData::parse(single),DdsData::parse(single_index));
        check(single_image.width==factor&&single_image.height==factor,"Actual large BC single-tile geometry differs");
        for(std::size_t at=0;at<single_image.rgba.size();at+=4)
            check(single_image.rgba[at]==single[128+at+2]&&single_image.rgba[at+1]==single[128+at+1]&&
                  single_image.rgba[at+2]==single[128+at]&&single_image.rgba[at+3]==single[128+at+3],
                  "Actual-size large BC fixture changed pixels/alpha");
        auto mixed=index;std::copy_n("BC008",5,mixed.begin()+116);
        rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(atlas),DdsData::parse(mixed));},
                            "UI block atlas/index tile profiles differ or are unsupported");
        auto bad_geometry=bc010_dds(factor+1,factor,32);std::copy_n(tag,5,bad_geometry.begin()+116);
        rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(bad_geometry),DdsData::parse(index));},
                            "Invalid FTXTInterface tile geometry");
        auto outside=index;outside[128]=2;
        rejects_bc010_reason([&]{expand_ui_block_atlas(DdsData::parse(atlas),DdsData::parse(outside));},
                            "FTXTInterface index refers outside its atlas");
    };
    check_large_tile("BC020",32);check_large_tile("BC040",64);
    auto unknown=interface_color;
    std::copy_n("WHAT?",5,unknown.begin()+116);
    rejects([&]{ui_texture_storage_profile(DdsData::parse(unknown));});
    auto size24 = rgba;
    put(size24, 4, 24);
    rejects([&] { DdsData::parse(size24); });
    auto legacy_header =
        DdsData::parse(size24, NumericDdsProfile::reject, DdsHeaderProfile::civ5_d3dx_size24);
    check(legacy_header.original_bytes() == size24 && legacy_header.payload(0, 0).size() == 8,
          "Explicit size24 profile preserves full header and payload");
    put(size24, 4, 25);
    rejects([&] { DdsData::parse(size24, NumericDdsProfile::reject, DdsHeaderProfile::civ5_d3dx_size24); });
    for (const auto code : {36u, 110u, 111u, 112u, 113u, 114u, 115u, 116u}) {
        auto data = dds_header(1, 1, 1, code);
        const unsigned bits = code == 111 ? 16 : code == 112 || code == 114 ? 32 : code == 116 ? 128 : 64;
        data.resize(128 + bits / 8, 0x7b);
        rejects([&] { DdsData::parse(data); });
        auto image = DdsData::parse(data, NumericDdsProfile::legacy_d3d9);
        check(image.bits_per_pixel() == bits && image.payload(0, 0).size() == bits / 8,
              "Explicit numeric D3D9 format width");
        if (code == 36)
            check(image.format() == DdsFormat::rgba16_unorm, "36 is four U16 UNORM, never RG half float");
        if (code == 112)
            check(image.format() == DdsFormat::rg16_float, "112 is RG half float");
    }
    auto wrong36 = dds_header(1, 1, 1, 36);
    wrong36.resize(132);
    rejects([&] { DdsData::parse(wrong36, NumericDdsProfile::legacy_d3d9); });
    for (const auto &mutation : std::vector<std::pair<std::size_t, std::uint32_t>>{{4, 0},
                                                                                   {76, 0},
                                                                                   {16, 0},
                                                                                   {12, 4097},
                                                                                   {28, 0},
                                                                                   {28, 9},
                                                                                   {24, 2},
                                                                                   {112, 0x200000},
                                                                                   {112, 0x200},
                                                                                   {84, 0x30315844}}) {
        auto bad = rgba;
        put(bad, mutation.first, mutation.second);
        rejects([&] { DdsData::parse(bad, NumericDdsProfile::legacy_d3d9); });
    }
    auto padded = rgba;
    put(padded, 8, 0x20008);
    put(padded, 20, 12);
    rejects([&] { DdsData::parse(padded); });
    auto npot = dds_header(5, 3, 3, 0x31545844);
    npot.resize(128 + 16 + 8 + 8, 9);
    auto image = DdsData::parse(npot);
    check(image.levels()[0].row_pitch == 16 && image.levels()[1].width == 2 && image.levels()[1].height == 1,
          "NPOT compressed block layout");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "Test output parent required");
        const auto folder =
            fs::path(argv[1]) /
            ("asset-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(fs::create_directory(folder), "Fresh test directory required");
        sources(folder);
        textures();
        std::cout << "PASS: " << checks << " raw asset/FPK/DDS checks; test-only data in "
                  << path_utf8(folder) << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
