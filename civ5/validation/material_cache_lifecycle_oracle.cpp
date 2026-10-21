#include "material_cache_lifecycle_validation.hpp"
using namespace cache_lifecycle_validation;

int main(int argc, char **argv) {
    try {
        require(argc == 3, "Lifecycle primitive input and fresh output required");
        Reader in(argv[1]);
        require(in.bytes(8) == Bytes{'C', '5', 'L', 'I', 'F', 'E', 'I', '1'}, "Lifecycle input schema");
        const auto count = in.word();
        Bytes out{'C', '5', 'L', 'I', 'F', 'E', 'O', '1'};
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto mode = in.word(), stamp = in.word(), argument = in.word();
            Case test(in.bytes(1024));
            Pool pool(test);
            const auto metadata = in.word();
            for (unsigned j = 0; j < metadata; ++j) {
                const auto handle = in.word(), width = in.word();
                pool.storage.add_texture(handle, width, false);
            }
            for (auto width : widths) {
                const auto free = in.word();
                for (unsigned j = 0; j < free; ++j) {
                    const auto handle = in.word();
                    require(pool.storage.texture_width(handle) == width, "Entering free-list width mismatch");
                    pool.storage.give(handle);
                }
            }
            Backend backend(test);
            word(out, material_cache_dispatch(test.state, pool, backend, mode != 0, stamp, argument));
            const auto final = test.snapshot();
            out.insert(out.end(), final.begin(), final.end());
            word(out, static_cast<std::uint32_t>(test.events.size()));
            for (const auto &event : test.events) {
                word(out, event.kind);
                word(out, static_cast<std::uint32_t>(event.arguments.size()));
                for (const auto value : event.arguments) {
                    word(out, static_cast<std::uint32_t>(value));
                    word(out, static_cast<std::uint32_t>(value >> 32));
                }
                out.insert(out.end(), event.patch.begin(), event.patch.end());
            }
            for (auto width : widths)
                vector_words(out, pool.storage.free_handles(width));
            backend.validate();
        }
        require(in.at == in.data.size(), "Trailing lifecycle input");
        require(!std::filesystem::exists(argv[2]), "Fresh lifecycle output required");
        std::ofstream stream(argv[2], std::ios::binary);
        stream.write(reinterpret_cast<const char *>(out.data()), out.size());
        require(bool(stream), "Lifecycle output failed");
        std::cout << "PASS native lifecycle " << count << " cases; actual atomic and staging ownership\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
