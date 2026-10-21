#include "civ5/materials/color_keys.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace civ5::materials;
using namespace civ5::assets;
namespace fs = std::filesystem;
namespace {
void write(const fs::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file)
        throw std::runtime_error("Cannot write ColorKey output");
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected asset root and fresh output directory");
        const fs::path root(argv[1]), output(argv[2]);
        if (fs::exists(output))
            throw std::invalid_argument("Fresh ColorKey output directory required");
        const LooseSource source(root);
        const auto keys = ColorKeyAssets::load([&](const ColorKeyRequest &r) -> std::optional<AssetHandle> {
            const auto value = source.find(AssetKey(r.native_path));
            if (std::holds_alternative<std::monostate>(value))
                return {};
            if (!std::holds_alternative<AssetHandle>(value))
                throw std::runtime_error("Ambiguous ColorKey asset");
            return std::get<AssetHandle>(value);
        });
        fs::create_directories(output);
        write(output / "identity.bgra", ColorKeyVolume::identity().bgra());
        std::ofstream requests(output / "requests.tsv", std::ios::binary);
        for (unsigned i = 0; i < keys.volumes().size(); ++i) {
            const auto &key = keys.volumes()[i];
            std::ostringstream filename;
            filename << std::setfill('0') << std::setw(2) << i << ".bgra";
            write(output / filename.str(), key.volume.bgra());
            requests << i << '\t' << key.authored << '\t' << key.request.native_path << '\n';
        }
        if (!requests)
            throw std::runtime_error("Cannot write ColorKey provenance");
        std::cout << "PASS C++ ColorKey loader: 25 volumes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
