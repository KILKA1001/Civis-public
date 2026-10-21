#include "civ5/terrain/coast_assets.hpp"
#include <cmath>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Coast asset check failed at line " + std::to_string(where.line()));
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
std::string xml() {
    return "<?xml version='1.0' encoding='utf-8'?><CoastRoot>"
           "<SplineCharacterList xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>"
           "<!-- ordered --><SplineCharacter Filename='A&amp;B.txt'/><SplineCharacter "
           "Filename='&#x431;&#1077;.txt'/>"
           "</SplineCharacterList><CoastStyleList><CoastStyle type='Grass'>"
           "<SmoothingOffset HighSide='0' LowSide='+20'/><HighBeach Height='.25' Offset='1.5' Character='2' "
           "TerrainType='5'/>"
           "<LowBeach Height='.1' Offset='2' Character='65535' TerrainType='65535'/>"
           "<Waterline Height='-.2e-1' Offset='-0' Character='1'/></CoastStyle></CoastStyleList></CoastRoot>";
}
std::string changed(std::string from, std::string to) {
    auto result = xml();
    const auto at = result.find(from);
    check(at != std::string::npos);
    result.replace(at, from.size(), to);
    return result;
}
void styles() {
    auto parsed = parse_coast_styles(xml());
    check(parsed.profiles.records().size() == 1 && parsed.character_files.size() == 2);
    check(parsed.character_files[0] == "A&B.txt" && parsed.character_files[1] == "\xd0\xb1\xd0\xb5.txt");
    const auto &p = parsed.profiles.records()[0];
    check(p.key == 1 && p.nodes[0].offset == 0.f && std::signbit(p.nodes[0].offset));
    check(p.nodes[0].height == .25f && p.nodes[2].height == -.02f && std::signbit(p.nodes[2].offset));
    check(p.nodes[3].character == -1 && p.materials == std::array<std::int32_t, 4>{-1, 5, -1, -1});
    check(parse_coast_styles(changed("Grass", "grass")).profiles.records()[0].key == -1);
    check(parse_coast_styles("\xef\xbb\xbf" + xml()).profiles.records().size() == 1);
    check(parse_coast_styles(changed("A&amp;B.txt", "a\r\nb\tc.txt")).character_files[0] == "a b c.txt");
    for (const auto &invalid : {changed(" HighSide='0'", ""),
                                changed("LowSide='+20'", "LowSide='nan'"),
                                changed("LowSide='+20'", "LowSide='+-1'"),
                                changed("Character='2'", "Character='2147483648'"),
                                changed("LowSide='+20'", "LowSide='1e1000'"),
                                changed("utf-8", "utf-16"),
                                changed("version='1.0'", "version='1.1'"),
                                changed("type='Grass'", "type='Grass'test='x'"),
                                changed("type='Grass'", "type='Grass' type='Snow'"),
                                changed("A&amp;B.txt", "&external;"),
                                changed("A&amp;B.txt", "&#0;"),
                                changed("A&amp;B.txt", "&#xD800;"),
                                changed("A&amp;B.txt", "&#x110000;"),
                                changed("A&amp;B.txt", "&#x;"),
                                changed("A&amp;B.txt", "<bad"),
                                changed("ordered", "a--b"),
                                changed("<!-- ordered -->", "<!--bad--->"),
                                changed("</CoastRoot>", "</Other>"),
                                changed("<CoastRoot>", "<!DOCTYPE CoastRoot><CoastRoot>"),
                                changed("<CoastRoot>", "<CoastRoot>text"),
                                xml() + "<extra/>",
                                xml().substr(0, xml().size() - 1),
                                changed("Grass", std::string("bad\0name", 8)),
                                changed("Grass", "\xc0\x80"),
                                changed("Grass", "\xed\xa0\x80"),
                                changed("Grass", "\xf4\x90\x80\x80")})
        rejects([&] { parse_coast_styles(invalid); });
    std::string nested;
    for (int i = 0; i < 17; ++i)
        nested += "<x>";
    for (int i = 0; i < 17; ++i)
        nested += "</x>";
    rejects([&] { parse_coast_styles(nested); });
    rejects([] { parse_coast_styles(std::string(8 * 1024 * 1024 + 1, ' ')); });
}
std::string character(std::string ending = "\n") {
    std::string result;
    for (int i = 0; i < 98; ++i)
        result += "[" + std::to_string(i) + ", +0, -0]  " + ending;
    return result;
}
void characters() {
    for (const auto &ending : {"\n", "\r", "\r\n"}) {
        const auto parsed = parse_coast_character(character(ending) + "ignored invalid tail");
        check(parsed.size() == 98 && parsed[97].x == 97.f && std::signbit(parsed[0].z));
        for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
            const auto compiled = compile_coast_character(parsed, pc);
            check(compiled.size() == 98);
            for (auto p : compiled)
                check(p.tangent == 0.f && p.normal == 0.f);
        }
    }
    auto no_last_newline = character();
    no_last_newline.pop_back();
    check(parse_coast_character(no_last_newline).size() == 98);
    for (const auto &first : {"[1,2]", "[1,2,3,4]", "[nan,0,0]", "[0,0,inf]", "[+-,0,0]", "[0x1p2,0,0]",
                              " [0,0,0]", "", "[0,0,0]tail"})
        rejects([&] { parse_coast_character(std::string(first) + "\n" + character()); });
    auto short_text = character();
    short_text.erase(0, short_text.find('\n') + 1);
    rejects([&] { parse_coast_character(short_text); });
    rejects([] { parse_coast_character(std::string(8 * 1024 * 1024 + 1, ' ')); });
}
class UnresolvedSource final : public civ5::assets::AssetSource {
  public:
    explicit UnresolvedSource(bool ambiguous) : ambiguous_(ambiguous) {}
    civ5::assets::AssetLookup find(const civ5::assets::AssetKey &key) const override {
        if (ambiguous_)
            return civ5::assets::AmbiguousAsset{key, provenance(), {}};
        return {};
    }
    civ5::assets::Provenance provenance() const override {
        return {civ5::assets::SourceKind::loose, "explicit", ".", {}};
    }

  private:
    bool ambiguous_;
};
void source_guards() {
    for (bool ambiguous : {false, true}) {
        const UnresolvedSource source(ambiguous);
        rejects([&] {
            CoastLibrary::load(source, civ5::assets::AssetKey("coaststyles.xml"), X87Precision::single);
        });
        rejects([&] {
            CoastLibrary::load(source, civ5::assets::AssetKey("../outside.xml"), X87Precision::single);
        });
        rejects([&] {
            CoastLibrary::load(source, civ5::assets::AssetKey("coaststyles.xml"),
                               static_cast<X87Precision>(123));
        });
    }
}
} // namespace
int main() {
    try {
        styles();
        characters();
        source_guards();
        std::cout << "PASS " << checks << " coast source parser checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
