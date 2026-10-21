#include "economic_overview_geometry.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "provide installed BNW root");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto expansion = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<civ5::assets::LooseResourceIndex>(root);
        base->scan({"Assets/UI"});
        expansion->scan({"Assets/DLC/Expansion/UI"});
        bnw->scan({"Assets/DLC/Expansion2/UI"});
        civ5::assets::ResourceMountTable resources(
            civ5::assets::resource_lookup(base));
        resources.mount({1, 0, 0, civ5::assets::resource_lookup(expansion)});
        resources.mount({2, 0, 0, civ5::assets::resource_lookup(bnw)});
        const auto selected = resources.resolve(civ5::assets::AssetKey(
            "Assets/UI/InGame/Popups/EconomicOverview.xml"));
        const auto *asset = std::get_if<civ5::assets::AssetHandle>(&selected.asset);
        require(asset != nullptr, "active EconomicOverview XML selected");
        const auto &path = asset->candidate().provenance.source_relative_path;
        require(path.find("Expansion2") != std::string::npos,
                "selected EconomicOverview source belongs to Expansion2");
        civ5::ui::EconomicTabSourceProof proof;
        proof.verified = true;
        proof.xml_path = path;
        civ5::ui::EconomicTabOwner owner;
        owner.xml_path = proof.xml_path;
        owner.viewport_width = 1280;
        owner.viewport_height = 960;
        const auto first = civ5::validation::measure_economic_overview_buttons(
            resources, proof, owner);
        require(first && first->general_button.x == 154 &&
                first->general_button.y == 231 &&
                first->happiness_button.x == 454 &&
                first->close_button.x == 174 &&
                first->close_button.y == 759,
                "1280x960 active EconomicOverview buttons");
        owner.viewport_width = 1920;
        owner.viewport_height = 1080;
        const auto second = civ5::validation::measure_economic_overview_buttons(
            resources, proof, owner);
        require(second && second->general_button.x == 474 &&
                second->general_button.y == 291 &&
                second->happiness_button.x == 774 &&
                second->close_button.x == 494 &&
                second->close_button.y == 819,
                "1920x1080 active EconomicOverview buttons");
        owner.xml_path = "stale";
        require(!civ5::validation::measure_economic_overview_buttons(
                    resources, proof, owner),
                "stale selected source cannot bind clicks");
        owner.xml_path = proof.xml_path;
        owner.viewport_height = 640;
        require(!civ5::validation::measure_economic_overview_buttons(
                    resources, proof, owner),
                "truncated popup cannot bind clicks");
        std::cout << "BNW EconomicOverview geometry: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
