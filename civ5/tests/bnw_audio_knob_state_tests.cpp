#include "../src/ui/bnw_audio_knob_state_detail.hpp"

using namespace civ5;
using namespace civ5::ui::audio_settings_detail;
namespace {
void require(bool ok) { if (!ok) throw std::runtime_error("Audio knob state expectation failed"); }
template<class F> void rejects(F fn) {
    bool failed = false; try { fn(); } catch (const std::invalid_argument&) { failed = true; } require(failed);
}
content::XmlNode knob(std::string name, std::vector<content::XmlNode> children = {}) {
    return {"SoundType", "", {{"name", std::move(name)}}, std::move(children)};
}
}
int main() {
    const content::XmlNode hierarchy{"SoundTypes", "", {}, {
        knob("USER_VOLUME_MUSIC", {knob("GAME_MUSIC")}),
        knob("USER_VOLUME_SFX", {knob("GAME_INTERFACE")}),
        knob("USER_VOLUME_AMBIENCE", {knob("GAME_AMBIENCE")}),
        knob("USER_VOLUME_SPEECH", {knob("GAME_SPEECH")})}};
    auto indented = hierarchy; indented.text = " \t\r\n";
    for (auto& row : indented.children) row.text = "\n  ";
    KnobNumericGraph stock_shape(indented);
    require(stock_shape.size() == 9 && stock_shape.fields("GAME_MUSIC").parent_index == 1);
    rejects([&] { auto mixed = indented; mixed.text += "x"; KnobNumericGraph bad(mixed); });
    rejects([&] { auto mixed = indented; mixed.children[0].text += "x"; KnobNumericGraph bad(mixed); });
    KnobNumericGraph graph(hierarchy);
    require(graph.size() == 9 && graph.name(0) == "MASTER" && graph.name(2) == "GAME_MUSIC");
    require(graph.fields("GAME_MUSIC").parent_index == 1);
    require(std::bit_cast<std::uint32_t>(graph.category_effective("GAME_MUSIC")) == 0x3f800000u);
    graph.load_four_saved_percentages({53, 75, 0, 100});
    require(std::bit_cast<std::uint32_t>(graph.category_effective("GAME_MUSIC")) == 0x3f07ae14u);
    require(graph.category_effective("GAME_INTERFACE") == 0.75f);
    require(graph.category_effective("GAME_AMBIENCE") == 0.0f);
    const auto before = graph.fields("USER_VOLUME_MUSIC");
    graph.direct("USER_VOLUME_MUSIC", 2.0f);
    const auto after = graph.fields("USER_VOLUME_MUSIC");
    require(after.current == 1.0f && after.transition_mode == 0 && after.target == before.target &&
            after.slope == before.slope && after.cached_effective == before.cached_effective &&
            after.parent_index == before.parent_index && after.inherited_flag == before.inherited_flag);
    graph.direct("MASTER", 0.0f); require(graph.category_effective("GAME_INTERFACE") == 0.0f);
    graph.direct("MASTER", 1.0f); require(graph.category_effective("GAME_INTERFACE") == 0.75f);
    const auto untouched = graph.fields("USER_VOLUME_SFX").current;
    rejects([&] { graph.direct("USER_VOLUME_SFX", std::bit_cast<float>(0x7fc00001u)); });
    require(graph.fields("USER_VOLUME_SFX").current == untouched);
    rejects([&] { auto bad = hierarchy; bad.children.push_back(knob("MASTER")); KnobNumericGraph duplicate(bad); });
    rejects([&] { auto bad = hierarchy; bad.children[0].attributes.push_back({"name", "OTHER"}); KnobNumericGraph duplicate(bad); });
    auto missing = hierarchy; missing.children.pop_back(); KnobNumericGraph partial(missing);
    rejects([&] { partial.load_four_saved_percentages({53, 75, 0, 100}); });
    require(partial.fields("USER_VOLUME_MUSIC").current == 1.0f && partial.fields("USER_VOLUME_SFX").current == 1.0f);
    // Full permitted category index range:127 authored nodes plus MASTER.
    content::XmlNode maximum{"SoundTypes", "\n ", {}, {}};
    for (int i = 0; i < 127; ++i) maximum.children.push_back(knob("KNOB_" + std::to_string(i)));
    KnobNumericGraph full(maximum);
    require(full.size() == 128 && full.name(127) == "KNOB_126");
    for (std::size_t i = 0; i < full.size(); ++i) {
        const auto f = full.fields(full.name(i));
        require(std::bit_cast<std::uint32_t>(f.current) == 0x3f800000u &&
                std::bit_cast<std::uint32_t>(f.target) == 0x3f800000u &&
                std::bit_cast<std::uint32_t>(f.cached_effective) == 0x3f800000u &&
                std::bit_cast<std::uint32_t>(f.slope) == 0u && f.transition_mode == 0 &&
                f.parent_index == (i == 0 ? -1 : 0) && !f.inherited_flag);
    }
    require(full.category_effective("KNOB_126") == 1.0f);
    maximum.children.push_back(knob("KNOB_127"));
    rejects([&] { KnobNumericGraph over_range(maximum); });
}
