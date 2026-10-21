#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace civ5::content {
namespace {
constexpr std::uint64_t xml_budget = 64ULL * 1024 * 1024;
bool xml_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool ascii_name(std::string_view name) {
    const auto first = [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':';
    };
    if (name.empty() || !first(name.front()))
        return false;
    for (const char c : name)
        if (!first(c) && !(c >= '0' && c <= '9') && c != '-' && c != '.')
            return false;
    return true;
}
std::size_t tag_end(std::string_view input, std::size_t begin) {
    char quote = 0;
    for (auto i = begin; i < input.size(); ++i) {
        const char c = input[i];
        if (quote) {
            if (c == quote)
                quote = 0;
        } else if (c == '\'' || c == '"')
            quote = c;
        else if (c == '>')
            return i + 1;
    }
    throw std::runtime_error("Unterminated gameplay XML tag");
}
void mask(std::string &result, std::string_view body) {
    for (const char c : body) {
        if (!c)
            throw std::runtime_error("Unverified NUL in gameplay XML markup");
        result += c == '\r' || c == '\n' ? c : ' ';
    }
}
std::string normalize_gameplay_markup(const std::string &text) {
    if (text.size() > xml_budget)
        throw std::runtime_error("Gameplay XML exceeds reader budget");
    const std::string_view input(text);
    // The verified native grammar is byte-oriented. Keep the existing strict
    // XmlLite path for UTF-16/32 instead of scanning individual encoding bytes.
    if (input.starts_with("\xff\xfe") || input.starts_with("\xfe\xff") || (!input.empty() && !input[0]) ||
        (input.size() > 1 && !input[1]))
        return text;
    std::string result;
    result.reserve(text.size());
    std::vector<std::string_view> elements;
    for (std::size_t i = 0; i < input.size();) {
        if (input[i] != '<') {
            const auto next = input.find('<', i);
            const auto end = next == std::string_view::npos ? input.size() : next;
            result.append(input.substr(i, end - i));
            i = end;
            continue;
        }
        if (input.substr(i).starts_with("<!--")) {
            // DLL 0x1000AE6B -> 0x100099A0. The native loop checks only
            // '-', '-', '>' and otherwise advances one byte. A '--' within
            // the body is allowed; comments do not produce gameplay nodes.
            const auto begin = i + 4;
            const auto end = input.find("-->", begin);
            if (end == std::string_view::npos)
                throw std::runtime_error("Unterminated gameplay XML comment");
            result += "<!--";
            mask(result, input.substr(begin, end - begin));
            result += "-->";
            i = end + 3;
        } else if (input.substr(i).starts_with("<![CDATA[")) {
            const auto end = input.find("]]>", i + 9);
            if (end == std::string_view::npos)
                throw std::runtime_error("Unterminated gameplay XML CDATA");
            result.append(input.substr(i, end + 3 - i));
            i = end + 3;
        } else if (input.substr(i).starts_with("<?")) {
            const auto end = input.find("?>", i + 2);
            if (end == std::string_view::npos)
                throw std::runtime_error("Unterminated gameplay XML processing instruction");
            result.append(input.substr(i, end + 2 - i));
            i = end + 2;
        } else if (input.substr(i).starts_with("<!")) {
            // DLL 0x1000AE81: unknown declarations (including '<!- -') end
            // at the first '>', without quoting/nesting, and produce no node.
            // DTD has a different native reader; its semantics are not ported.
            if (input.substr(i).starts_with("<!DOCTYPE"))
                throw std::runtime_error("Unverified gameplay XML DTD");
            const auto end = input.find('>', i + 2);
            if (end == std::string_view::npos)
                throw std::runtime_error("Unterminated gameplay XML declaration");
            // An ignored node must not become whitespace in a leaf value.
            result += "<!--";
            mask(result, input.substr(i, end + 1 - i));
            result += "-->";
            i = end + 1;
        } else {
            const bool closing = input.substr(i).starts_with("</");
            const auto begin = i + (closing ? 2 : 1);
            const auto end = tag_end(input, begin);
            auto name_end = begin;
            while (name_end < end && !xml_space(input[name_end]) && input[name_end] != '/' &&
                   input[name_end] != '>')
                ++name_end;
            const auto name = input.substr(begin, name_end - begin);
            if (closing && !elements.empty()) {
                auto suffix = name_end;
                while (suffix < end && xml_space(input[suffix]))
                    ++suffix;
                if (suffix != end - 1 || name.empty())
                    throw std::runtime_error("Unsupported gameplay XML closing tag");
                // DLL 0x1000AF7C..0x1000AFCE consumes the closing token and
                // whitespace, requires '>', but never compares the open name.
                // Normalize proven ASCII names for XmlLite; don't turn an
                // otherwise invalid token into a valid one by renaming it.
                if (name != elements.back() && (!ascii_name(name) || !ascii_name(elements.back())))
                    throw std::runtime_error("Unverified gameplay XML closing name");
                result += "</";
                result.append(elements.back());
                result.append(input.substr(name_end, end - name_end));
                elements.pop_back();
            } else {
                result.append(input.substr(i, end - i));
                if (!closing && input[end - 2] != '/') {
                    // Match the existing XML diagnostic depth budget.
                    if (elements.size() >= 128)
                        throw std::runtime_error("Gameplay XML exceeds depth budget");
                    elements.push_back(name);
                }
            }
            i = end;
        }
    }
    return result;
}
} // namespace
XmlNode read_game_data_xml_text(const std::string &text) {
    return read_xml_metadata(normalize_gameplay_markup(text));
}
XmlNode read_game_data_xml(const std::filesystem::path &path) {
    const auto bytes = std::filesystem::file_size(path);
    if (bytes > xml_budget)
        throw std::runtime_error("Gameplay XML exceeds reader budget");
    std::ifstream stream(path, std::ios::binary);
    runtime::BinaryReader reader(stream, bytes);
    std::string text(static_cast<std::size_t>(bytes), '\0');
    reader.read({reinterpret_cast<std::uint8_t *>(text.data()), text.size()});
    return read_game_data_xml_text(text);
}
} // namespace civ5::content
