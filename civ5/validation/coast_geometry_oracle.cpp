#include "coast_geometry_io.hpp"
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: civ5_coast_geometry_oracle input.bin fresh-output-directory");
        const fs::path input = argv[1], output = argv[2];
        check(!fs::exists(output), "Output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 24 && size <= 256 * 1024 * 1024, "Invalid coast geometry input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Coast geometry input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5CGEO01", "Wrong coast geometry input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid coast geometry record count");
        Reader library_input{r.blob()};
        const auto lib = library(library_input);
        const auto worker = r.blob(), gate_source = r.blob();
        check(fs::create_directories(output), "Cannot create coast geometry output directory");
        write_library(lib, output);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            Reader item{r.blob()};
            const auto raw = run(kind, item, lib, worker, gate_source, output / std::to_string(i));
            save(output / (std::to_string(i) + ".bin"), raw);
        }
        r.end();
        std::cout << "PASS " << count << " coast curve/geometry records (64-bit C++)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
