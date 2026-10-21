#include "civ5/materials/water_current.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>
using namespace civ5::materials;
using namespace generation_validation;
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        require(argc == 3, "Usage: water_current_oracle primitive-input fresh-output");
        const std::filesystem::path destination = argv[2];
        require(!std::filesystem::exists(destination), "Fresh current-map output required");
        const auto input = read(argv[1]);
        Reader reader{input};
        const auto magic = reader.take(8);
        require(std::string(magic.begin(), magic.end()) == "C5WCIN01", "Current-map input schema");
        const auto count = reader.word();
        require(count > 0 && count <= 1024, "Current-map case count");
        Bytes output{'C', '5', 'W', 'C', 'O', 'U', '0', '2'};
        word(output, count);
        const WaterCurrentSettings defaults;
        for (float v : {defaults.flow_strength, defaults.noise_strength, defaults.noise_scale})
            word(output, std::bit_cast<std::uint32_t>(v));
        const auto packet = water_current_packet();
        output.insert(output.end(), packet.begin(), packet.end());
        const auto quad = water_current_quad();
        for (const auto &vertex : quad.vertices)
            for (float value : vertex)
                word(output, std::bit_cast<std::uint32_t>(value));
        for (auto index : quad.indices)
            word(output, index);
        for (unsigned i = 0; i < count; ++i) {
            const auto width = std::bit_cast<std::int32_t>(reader.word());
            const auto height = std::bit_cast<std::int32_t>(reader.word());
            const auto use_defaults = reader.word();
            require(use_defaults <= 1, "Current-map default settings flag");
            WaterCurrentSettings settings{reader.floating(), reader.floating(), reader.floating()};
            WaterCurrentConstants prior;
            for (float &v : prior)
                v = reader.floating();
            const auto constants =
                water_current_constants(width, height, use_defaults ? defaults : settings, prior);
            for (float v : constants)
                word(output, std::bit_cast<std::uint32_t>(v));
        }
        require(reader.at == input.size(), "Trailing current-map input");
        write(destination, output);
        std::cout << "PASS " << count << " current-map constant blocks and complete packet\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
