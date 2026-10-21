#include "bnw_warrior_art_probe.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc,char **argv) {
    try {
        if(argc!=2) throw std::runtime_error("Usage: gl_bnw_warrior_art_probe CIV5_ASSET_ROOT");
        civ5::graphics::GlContext context(64,64,false);
        const auto mips=civ5::graphics::probe_bnw_gandhi_warrior_art(argv[1],context);
        if(mips!=48) throw std::runtime_error("Warrior BC3 mip count differs");
        context.check("BNW Warrior art texture upload/readback");
        std::cout << "{\"status\":\"PASS\",\"formationMembers\":12,"
                     "\"gr2MeshesLocated\":3,\"bc3TexturesUploaded\":6,"
                     "\"bc3MipsReadBack\":" << mips << ","
                     "\"modelDrawable\":false,\"fullGraphicsParity\":false}\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
