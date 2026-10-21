// CPU-only D3DX file-layout control. No D3D device, window or game process.
#include "civ5/assets/asset_source.hpp"
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d9.h>
#include <windows.h>

using namespace civ5::assets;
namespace fs = std::filesystem;
struct ImageInfo {
    UINT width, height, depth, mip_levels;
    D3DFORMAT format;
    D3DRESOURCETYPE type;
    UINT file_format;
};
using GetImageInfo = HRESULT(WINAPI *)(LPCVOID, UINT, ImageInfo *);
static_assert(sizeof(ImageInfo) == 28);
struct Library {
    HMODULE handle;
    explicit Library(const fs::path &path)
        : handle(LoadLibraryExW(path.c_str(), nullptr,
                                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)) {
        if (!handle)
            throw std::runtime_error("Cannot load explicit 64-bit D3DX DLL");
    }
    ~Library() { FreeLibrary(handle); }
    Library(const Library &) = delete;
    Library &operator=(const Library &) = delete;
};
fs::path utf8_path(const std::string &name) { return fs::path(std::u8string(name.begin(), name.end())); }
int main(int argc, char **argv) {
    try {
        if (argc != 4)
            throw std::runtime_error(
                "Usage: civ5_dds_native_info asset-root explicit-d3dx9-dll fresh-output.tsv");
        const auto root = fs::canonical(utf8_path(argv[1])), dll = fs::canonical(utf8_path(argv[2])),
                   output = utf8_path(argv[3]);
        if (fs::exists(output))
            throw std::runtime_error("Refusing output overwrite");
        const Library library(dll);
        const auto function = reinterpret_cast<GetImageInfo>(
            GetProcAddress(library.handle, "D3DXGetImageInfoFromFileInMemory"));
        if (!function)
            throw std::runtime_error("D3DX image-info export missing");
        std::ofstream out(output);
        out << "source\theaderSize\tpixelHeaderSize\tfourCC\thr\twidth\theight\tdepth\tmips\tformat\ttype\tfi"
               "leFormat\tsize24Equivalent\tsize124Equivalent\n";
        std::uint32_t total = 0, success = 0;
        auto probe = [&](const std::string &label, const AssetHandle &handle) {
            const auto bytes = handle.read_bytes();
            auto word = [&](std::size_t offset) {
                std::uint32_t v = 0;
                for (unsigned i = 0; i < 4; ++i)
                    v |= std::uint32_t{bytes.at(offset + i)} << (8 * i);
                return v;
            };
            ImageInfo info{};
            const auto hr = function(bytes.data(), static_cast<UINT>(bytes.size()), &info);
            ++total;
            if (SUCCEEDED(hr))
                ++success;
            auto size_control = [&](std::uint32_t size) {
                auto controlled = bytes;
                for (unsigned i = 0; i < 4; ++i)
                    controlled[4 + i] = static_cast<std::uint8_t>(size >> (8 * i));
                ImageInfo value{};
                const auto result = function(controlled.data(), static_cast<UINT>(controlled.size()), &value);
                return result == hr && value.width == info.width && value.height == info.height &&
                       value.depth == info.depth && value.mip_levels == info.mip_levels &&
                       value.format == info.format && value.type == info.type &&
                       value.file_format == info.file_format;
            };
            out << label << '\t' << word(4) << '\t' << word(76) << '\t' << word(84) << '\t'
                << static_cast<std::uint32_t>(hr) << '\t' << info.width << '\t' << info.height << '\t'
                << info.depth << '\t' << info.mip_levels << '\t' << info.format << '\t' << info.type << '\t'
                << info.file_format << '\t' << size_control(24) << '\t' << size_control(124) << '\n';
        };
        const AssetEnvironment environment(root);
        const auto discovery = environment.discover_fpk_sources();
        if (!discovery.unresolved.empty())
            throw std::runtime_error("Unresolved archive");
        for (const auto &source : discovery.supported)
            for (const auto &entry : source->entries())
                if (entry.normalized_name.ends_with(".dds"))
                    probe(source->provenance().source_id + "::" + entry.original_name,
                          std::get<AssetHandle>(source->find(AssetKey(entry.original_name))));
        const auto loose = environment.loose_source();
        for (const auto &entry : fs::recursive_directory_iterator(root))
            if (!entry.is_symlink() && entry.is_regular_file() &&
                normalize_asset_name(path_utf8(entry.path().extension())) == ".dds") {
                const auto name = path_utf8(entry.path().lexically_relative(root));
                probe(name, std::get<AssetHandle>(loose->find(AssetKey(name))));
            }
        out.close();
        if (!out)
            throw std::runtime_error("Native DDS report write failed");
        std::cout << "Native CPU-only D3DX image info: " << total << " files, " << success << " accepted\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
