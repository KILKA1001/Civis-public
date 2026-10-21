#include "coast_asset_io.hpp"
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 5, "Usage: civ5_coast_asset_oracle asset-root archive-relative-path-or---loose "
                         "input.bin fresh-output");
        const fs::path root = argv[1], input = argv[3], output = argv[4];
        const auto archive = civ5::assets::path_utf8(fs::path(argv[2]));
        check(!fs::exists(output), "Output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 24 && size <= 256 * 1024 * 1024, "Invalid coast asset input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Coast asset input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5CASR01", "Wrong coast asset input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid coast asset record count");
        const auto key = r.blob();
        check(!key.empty() && key.size() <= 1024, "Invalid coast style asset key");
        const civ5::assets::AssetKey style_key(std::string(key.begin(), key.end()));
        const auto worker = r.blob(), gates = r.blob();
        const auto source = coast_source(root, archive);
        const auto pc24 = CoastLibrary::load(*source, style_key, X87Precision::single);
        const auto pc53 = CoastLibrary::load(*source, style_key, X87Precision::double_precision);
        const Library lib{pc24.profiles(), {pc24.characters(), pc53.characters()}};
        check(fs::create_directories(output), "Cannot create coast asset output directory");
        write_sources(pc24, output / "source-24.bin");
        write_sources(pc53, output / "source-53.bin");
        write_library(lib, output);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            Reader item{r.blob()};
            const auto raw = run_strip(kind, item, lib, worker, gates, output / std::to_string(i));
            save(output / (std::to_string(i) + ".bin"), raw);
        }
        r.end();
        std::cout << "PASS actual coast source load and " << count << " connected records (64-bit C++)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
