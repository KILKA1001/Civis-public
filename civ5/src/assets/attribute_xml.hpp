#pragma once
// Shared bounded parser for authored attribute/element-only asset XML.
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace civ5::assets::detail {
inline constexpr std::size_t attribute_xml_text_limit = 8 * 1024 * 1024;
inline void require(bool condition, const char *message) {
    if (!condition) throw std::invalid_argument(message);
}
inline bool whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
inline bool xml_character(std::uint32_t c) {
    return c == 9 || c == 10 || c == 13 || (c >= 0x20 && c <= 0xd7ff) || (c >= 0xe000 && c <= 0xfffd) ||
           (c >= 0x10000 && c <= 0x10ffff);
}
inline void validate_utf8(std::string_view s) {
    for (std::size_t i = 0; i < s.size();) {
        const auto first = static_cast<unsigned char>(s[i++]);
        std::uint32_t c = first, minimum = 0;
        unsigned rest = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            c = first & 31;
            rest = 1;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            c = first & 15;
            rest = 2;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            c = first & 7;
            rest = 3;
            minimum = 0x10000;
        } else
            require(first < 0x80, "Invalid asset XML UTF-8");
        require(rest <= s.size() - i, "Truncated asset XML UTF-8");
        for (unsigned j = 0; j < rest; ++j) {
            const auto b = static_cast<unsigned char>(s[i++]);
            require((b & 0xc0) == 0x80, "Invalid asset XML UTF-8 continuation");
            c = c * 64 + (b & 63);
        }
        require(c >= minimum && xml_character(c), "Invalid asset XML character");
    }
}
inline void append_utf8(std::string &out, std::uint32_t c) {
    require(xml_character(c), "Invalid asset XML character reference");
    if (c < 0x80)
        out.push_back(static_cast<char>(c));
    else if (c < 0x800) {
        out.push_back(static_cast<char>(0xc0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 63)));
    } else if (c < 0x10000) {
        out.push_back(static_cast<char>(0xe0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
        out.push_back(static_cast<char>(0x80 | (c & 63)));
    } else {
        out.push_back(static_cast<char>(0xf0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 63)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
        out.push_back(static_cast<char>(0x80 | (c & 63)));
    }
}
struct AttributeXmlNode {
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<AttributeXmlNode> children;
    const AttributeXmlNode &child(std::string_view key) const {
        const auto found =
            std::find_if(children.begin(), children.end(), [&](const auto &n) { return n.name == key; });
        require(found != children.end(), "Missing required asset XML element");
        return *found;
    }
    const std::string &attribute(const std::string &key) const {
        const auto found = attributes.find(key);
        require(found != attributes.end(), "Missing required asset XML attribute");
        return found->second;
    }
};
class AttributeXml {
  public:
    explicit AttributeXml(std::string_view s) : s_(s) {
        require(s.size() <= attribute_xml_text_limit, "Asset XML source exceeds supported bound");
        validate_utf8(s);
    }
    AttributeXmlNode document() {
        if (s_.starts_with("\xef\xbb\xbf"))
            s_.remove_prefix(3);
        if (s_.starts_with("<?xml")) {
            s_.remove_prefix(5);
            require(!s_.empty() && whitespace(s_.front()), "Invalid XML declaration");
            AttributeXmlNode declaration;
            attributes(declaration, "?>");
            consume("?>");
            require(declaration.attribute("version") == "1.0", "Unsupported asset XML version");
            if (const auto it = declaration.attributes.find("encoding"); it != declaration.attributes.end()) {
                auto encoding = it->second;
                for (auto &c : encoding)
                    if (c >= 'A' && c <= 'Z')
                        c = static_cast<char>(c + 32);
                require(encoding == "utf-8", "Asset XML requires UTF-8 encoding");
            }
            for (const auto &[key, value] : declaration.attributes)
                require(key == "version" || key == "encoding" ||
                            (key == "standalone" && (value == "yes" || value == "no")),
                        "Unsupported XML declaration attribute");
        }
        misc();
        auto root = node(0);
        misc();
        require(s_.empty(), "Trailing asset XML content");
        return root;
    }

  private:
    std::string_view s_;
    std::size_t nodes_ = 0;
    void consume(std::string_view token) {
        require(s_.starts_with(token), "Malformed asset XML token");
        s_.remove_prefix(token.size());
    }
    void space() {
        while (!s_.empty() && whitespace(s_.front()))
            s_.remove_prefix(1);
    }
    void misc() {
        for (;;) {
            space();
            if (!s_.starts_with("<!--"))
                return;
            s_.remove_prefix(4);
            const auto end = s_.find("-->");
            require(end != s_.npos && s_.substr(0, end).find("--") == s_.npos &&
                        (end == 0 || s_[end - 1] != '-'),
                    "Malformed asset XML comment");
            s_.remove_prefix(end + 3);
        }
    }
    std::string name() {
        auto initial = [](char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':';
        };
        require(!s_.empty() && initial(s_.front()), "Invalid asset XML name");
        std::size_t size = 1;
        while (size < s_.size() && (initial(s_[size]) || (s_[size] >= '0' && s_[size] <= '9') ||
                                    s_[size] == '-' || s_[size] == '.'))
            ++size;
        require(size <= 1024, "Asset XML name too long");
        std::string result(s_.substr(0, size));
        s_.remove_prefix(size);
        return result;
    }
    std::string value() {
        require(!s_.empty() && (s_.front() == '\'' || s_.front() == '"'), "Unquoted asset XML attribute");
        const char quote = s_.front();
        s_.remove_prefix(1);
        std::string result;
        while (!s_.empty() && s_.front() != quote) {
            const auto c = s_.front();
            s_.remove_prefix(1);
            require(c != '<', "Unescaped asset XML attribute delimiter");
            if (c == '&') {
                const auto end = s_.find(';');
                require(end != s_.npos && end <= 32, "Invalid asset XML entity");
                auto entity = s_.substr(0, end);
                s_.remove_prefix(end + 1);
                if (entity == "amp")
                    result.push_back('&');
                else if (entity == "lt")
                    result.push_back('<');
                else if (entity == "gt")
                    result.push_back('>');
                else if (entity == "quot")
                    result.push_back('"');
                else if (entity == "apos")
                    result.push_back('\'');
                else {
                    require(entity.starts_with('#'), "Unsupported asset XML entity");
                    entity.remove_prefix(1);
                    int base = 10;
                    if (entity.starts_with('x')) {
                        base = 16;
                        entity.remove_prefix(1);
                    }
                    std::uint32_t code = 0;
                    const auto parsed =
                        std::from_chars(entity.data(), entity.data() + entity.size(), code, base);
                    require(!entity.empty() && parsed.ec == std::errc{} &&
                                parsed.ptr == entity.data() + entity.size(),
                            "Invalid asset XML character reference");
                    append_utf8(result, code);
                }
            } else if (whitespace(c)) {
                if (c == '\r' && s_.starts_with('\n'))
                    s_.remove_prefix(1);
                result.push_back(' ');
            } else
                result.push_back(c);
            require(result.size() <= 65536, "Asset XML attribute too long");
        }
        require(!s_.empty(), "Unterminated asset XML attribute");
        s_.remove_prefix(1);
        return result;
    }
    void attributes(AttributeXmlNode &node, std::string_view end) {
        for (;;) {
            const bool separated = !s_.empty() && whitespace(s_.front());
            space();
            if (s_.starts_with(end) || (end == ">" && s_.starts_with("/>")))
                return;
            require(separated && node.attributes.size() < 64, "Invalid asset XML attribute list");
            auto key = name();
            space();
            consume("=");
            space();
            auto v = value();
            require(node.attributes.emplace(std::move(key), std::move(v)).second,
                    "Duplicate asset XML attribute");
        }
    }
    AttributeXmlNode node(unsigned depth) {
        require(depth < 16 && ++nodes_ <= 32768, "Asset XML structural bound exceeded");
        consume("<");
        AttributeXmlNode result;
        result.name = name();
        attributes(result, ">");
        if (s_.starts_with("/>")) {
            s_.remove_prefix(2);
            return result;
        }
        consume(">");
        for (;;) {
            misc();
            if (s_.starts_with("</")) {
                s_.remove_prefix(2);
                require(name() == result.name, "Mismatched asset XML end tag");
                space();
                consume(">");
                return result;
            }
            // Authored schema contains attributes and element children, no text,
            // CDATA, DTD/entity declarations or processing instructions.
            result.children.push_back(node(depth + 1));
        }
    }
};
} // namespace civ5::assets::detail
