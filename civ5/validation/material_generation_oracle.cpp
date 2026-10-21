#include "material_generation_fixture.hpp"
#include <iostream>
using namespace generation_validation;
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_material_generation_oracle input.bin fresh-output.bin");
        require(!std::filesystem::exists(argv[2]), "Refusing generation output overwrite");
        const auto input = read(argv[1]);
        const auto corpus = records(input);
        Bytes out(input.begin(), input.begin() + 8);
        word(out, static_cast<std::uint32_t>(corpus.size()));
        for (const auto &record : corpus) {
            const auto result = run(record.op, Reader{record.input});
            word(out, record.op);
            blob(out, record.label);
            blob(out, result);
        }
        write(argv[2], out);
        std::cout << "PASS computed " << corpus.size() << " generation records / " << out.size()
                  << " bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
