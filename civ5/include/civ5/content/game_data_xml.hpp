#pragma once
#include "civ5/content/manifest.hpp"

namespace civ5::content {
// Gameplay XML uses the verified retail comment/declaration terminators and
// accepts mismatched ASCII closing names. This is not a full native parser.
// Manifests/descriptors still use the separate strict XML reader.
XmlNode read_game_data_xml_text(const std::string &text);
XmlNode read_game_data_xml(const std::filesystem::path &path);
} // namespace civ5::content
