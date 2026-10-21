#pragma once
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

// Private source algorithm. A string result cannot issue a Package owner,
// descriptor snapshot, language owner, category source or mount capability.
namespace civ5::ui::native_package_name_detail {
inline void require(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
inline std::string ascii_fold(std::string_view text) {
    std::string out(text);
    for (auto& c : out) {
        const auto byte = static_cast<unsigned char>(c);
        require(byte >= 32 && byte < 127,
                "Native package Name needs CP_ACP/XML text source for non-ASCII/control bytes");
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
    }
    return out;
}
inline void require_flat_text(const content::XmlNode& node) {
    require(node.children.empty(), "Native package Name nested text requires native XML string producer");
    (void)ascii_fold(node.text);
    // Do not silently trim authored text or substitute PackageManifest.name.
    // This finite schema has identical raw and whitespace-trimmed text.
    require(node.text.empty() || (node.text.front() != ' ' && node.text.back() != ' '),
            "Native package Name whitespace needs native XML normalization producer");
}
inline const content::XmlNode* first_name(const content::XmlNode& package) {
    require(package.name == "Civ5Package", "Native package Name requires original DLC XML");
    const content::XmlNode* name = nullptr;
    for (const auto& child : package.children) if (child.name == "Name") {
        require(!name, "Repeated native package Name needs first-node parser producer");
        name = &child;
    }
    return name;
}
// Native890EC0: match current language even after a fallback was selected;
// otherwise first en_US/english fallback while no value has been selected.
// nullopt means an arbitrary current language absent from these authored labels,
// not an actual current-language receipt. This pure kernel grants no authority.
inline std::string name_branch_diagnostic(const content::XmlNode& package,
                                          std::optional<std::string_view> language) {
    const auto* name = first_name(package);
    if (!name) return {}; // native fresh+4 stays the constructor empty string.
    if (name->children.empty()) {
        require_flat_text(*name);
        return name->text;
    }
    std::optional<std::string> selected;
    for (const auto& value : name->children) {
        require(value.name == "Value", "Native package Name child traversal schema needs source owner");
        require_flat_text(value);
        require(!value.text.empty(),
                "Empty native package Name Value needs exact native XML string failure-output producer");
        const auto label = value.attribute("language");
        const auto folded = ascii_fold(label);
        //8355C0/8352A0 failure skips a Value with no language attribute.
        const bool has_language = std::any_of(value.attributes.begin(),value.attributes.end(),
            [](const auto& attribute) { return attribute.first == "language"; });
        if (!has_language) continue;
        require(!label.empty(),
                "Empty native package Name language attribute needs exact native XML attribute producer");
        const bool current_matches = language && folded == ascii_fold(*language);
        if (current_matches || (!selected && (folded == "en_us" || folded == "english")))
            selected = value.text;
    }
    // Native falls back to the enclosing Name text if no Value was selected.
    // Mixed/whitespace text is not authenticated by the current XmlNode port.
    require(selected.has_value(),
            "Native package Name requires authentic language or enclosing XML string producer");
    return *selected;
}
// Production can eliminate the language operand only by proving ALL branches
// return the SAME authored bytes. It never reads OS/user locale or invents a
// current language. A translated alternative requires the real native parent.
inline std::string language_independent_native_name(const content::XmlNode& package) {
    const auto* name = first_name(package);
    const auto invariant = name_branch_diagnostic(package,std::nullopt);
    if (!name) return invariant;
    for (const auto& value : name->children) {
        const auto language = value.attribute("language");
        const auto branch = name_branch_diagnostic(package,std::string_view(language));
        require(branch == invariant,
                "Native package Name varies by language: actual retained890EC0/497940 language producer required");
    }
    return invariant;
}
}
