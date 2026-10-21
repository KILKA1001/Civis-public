#pragma once
#include <array>
#include <bit>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::ui::native_color_load_detail {
// Pure private value operations. These byte views, hashes and mutations are
// data, never selected Skin/registry identity, currentness or a lookup receipt.
using ColorBits = std::array<std::uint32_t,4>;
using ColorSetBytes = std::array<std::uint8_t,64>;
inline void bounded(std::string_view s) {
    if (s.size() > std::size_t(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("Native Color logical length exceeds signed32");
}
inline bool space(unsigned char c) { return std::isspace(c) != 0; }
// Actual CRT operators, with unsigned-byte input as native MOVZX. This does
// not prove the current CRT locale equals Original MSVCR90's selected locale.
inline std::string_view trim(std::string_view s) {
    bounded(s);
    while (!s.empty() && space(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && space(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
    return s;
}
inline std::string_view c_prefix(std::string_view s) { return s.substr(0,s.find('\0')); }
struct Tokens {
    std::string_view text;
    std::size_t next = 0;
    std::string_view take() {
        // Native callers test negative cursor before taking another token.
        if (next == std::string_view::npos) throw std::logic_error("Native Color token cursor exhausted");
        const auto start = next;
        const auto remainder = text.substr(start);
        const auto delimiter = c_prefix(remainder).find_first_of(" .,x*|");
        if (delimiter == std::string_view::npos) {
            next = std::string_view::npos;
            return remainder; // FString length survives an embedded NUL.
        }
        next = start + delimiter + 1;
        return remainder.substr(0,delimiter); // Advance ONE delimiter only.
    }
};
inline std::int32_t integer(std::string_view s) {
    // 81EC40 called with base10/allowLowercase0x=true. Native multiply/add and
    // NEG wrap DWORD; a C++ signed overflow or from_chars policy is unsuitable.
    s = c_prefix(s);
    while (!s.empty() && space(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    bool negative = false;
    if (!s.empty() && (s.front() == '+' || s.front() == '-')) {
        negative = s.front() == '-'; s.remove_prefix(1);
    }
    std::int32_t base = 10;
    if (s.starts_with("0x")) { base = 16; s.remove_prefix(2); }
    std::uint32_t value = 0;
    for (const char raw_byte : s) {
        const auto byte = static_cast<unsigned char>(raw_byte);
        std::int32_t digit;
        if (std::isdigit(byte)) digit = std::bit_cast<std::int8_t>(std::uint8_t(byte)) - '0';
        else if (std::isalpha(byte)) digit = std::toupper(byte) - '7';
        else break;
        if (digit >= base) break;
        value = value * std::uint32_t(base) + std::uint32_t(digit);
    }
    if (negative) value = 0U - value;
    return std::bit_cast<std::int32_t>(value);
}
inline std::uint32_t scaled(std::int32_t v) {
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    // Each native component uses CVTSI2SS then MULSS, not division/double.
    const float factor = std::bit_cast<float>(std::uint32_t(0x3b808081));
    const float component = static_cast<float>(v) * factor;
    return std::bit_cast<std::uint32_t>(component);
}
inline void numeric(std::string_view input, ColorBits& destination) {
    const auto text = trim(input);
    if (text.empty()) return; // Native leaves all supplied bytes untouched.
    destination = {0,0,0,0x437f0000}; // Alpha255 BEFORE final scale.
    Tokens cursor{text};
    for (std::size_t channel = 0; channel != 4; ++channel) {
        if (channel && cursor.next == std::string_view::npos) break;
        const auto token = cursor.take();
        if (!token.empty()) {
            auto v = integer(token);
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            destination[channel] = std::bit_cast<std::uint32_t>(static_cast<float>(v));
        }
    }
    const float factor = std::bit_cast<float>(std::uint32_t(0x3b808081));
    for (auto& word : destination)
        word = std::bit_cast<std::uint32_t>(std::bit_cast<float>(word) * factor);
}
inline std::uint32_t hash_length(std::string_view bytes) {
    bounded(bytes);
    // Exact retained stock table producer's eight rounds, polynomial operand
    // EDB88320. This owned arithmetic is not a current writable-global lease.
    std::uint32_t state = 0xffffffffU;
    for (const char raw_byte : bytes) {
        const auto byte = static_cast<unsigned char>(raw_byte);
        state ^= byte;
        for (unsigned bit = 0; bit != 8; ++bit)
            state = (state >> 1) ^ ((state & 1) ? 0xedb88320U : 0U);
    }
    return state; // No final XOR and NO case-folding.
}
inline std::uint32_t hash_query(std::string_view bytes) { return hash_length(c_prefix(bytes)); }
inline std::uint32_t read_word(const ColorSetBytes& bytes, std::size_t at) {
    if (at > bytes.size()-4) throw std::out_of_range("Native ColorSet DWORD read");
    return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1])<<8) |
        (std::uint32_t(bytes[at+2])<<16) | (std::uint32_t(bytes[at+3])<<24);
}
inline void write_word(ColorSetBytes& bytes, std::size_t at, std::uint32_t value) {
    if (at > bytes.size()-4) throw std::out_of_range("Native ColorSet DWORD write");
    for (unsigned i = 0; i != 4; ++i) bytes[at+i] = std::uint8_t(value>>(i*8));
}
inline void default_color_set(ColorSetBytes& bytes) {
    for (std::size_t slot = 0; slot != 3; ++slot) {
        for (std::size_t component = 0; component != 4; ++component)
            write_word(bytes,slot*16+component*4,0x3f800000);
        write_word(bytes,0x30+slot*4,0);
        bytes[0x3c+slot] = 0;
    }
    // 540EB0 does NOT initialize byte3f.
}
inline void component_literal(ColorSetBytes& bytes, std::size_t slot, std::string_view text) {
    if (slot > 3) throw std::out_of_range("Native ColorSet terminal selector slot");
    text = trim(text);
    if (text.empty()) return; // 76D3C0 exits before key/tag mutation.
    write_word(bytes,0x30+slot*4,0);
    bytes[0x3c+slot] = 0;
    ColorBits value{};
    numeric(text,value);
    // Color3 destination+30 overlaps reference/tag bytes; order is required.
    for (std::size_t i = 0; i != 4; ++i) write_word(bytes,slot*16+i*4,value[i]);
}
inline void component_named_value(ColorSetBytes& bytes, std::size_t slot,
        const ColorBits& already_read_value, std::uint32_t already_read_key) {
    if (slot > 3) throw std::out_of_range("Native ColorSet terminal selector slot");
    // The actual ordered lookup must already have produced this value. This
    // mutation neither invokes a callback nor manufactures lookup authority.
    for (std::size_t i = 0; i != 4; ++i) write_word(bytes,slot*16+i*4,already_read_value[i]);
    write_word(bytes,0x30+slot*4,already_read_key);
    bytes[0x3c+slot] = 4;
}
inline void component_named_alpha(ColorSetBytes& bytes, std::size_t slot,
        std::string_view extracted_alpha_token) {
    if (slot > 3) throw std::out_of_range("Native ColorSet terminal selector slot");
    // The native successful-reference branch also converts an EMPTY token.
    write_word(bytes,slot*16+12,scaled(integer(extracted_alpha_token)));
    bytes[0x3c+slot] = 3;
}
inline bool remember_first_hash(std::map<std::uint32_t,ColorBits>& values,
        std::uint32_t actual_load_key, const ColorBits& value) {
    return values.try_emplace(actual_load_key,value).second;
}
// Raw value operand, NOT a native filename/Package/source capability. The
// logical length and first-NUL search boundary are deliberately separate.
struct EffectiveFilenameValue {
    std::string_view storage;
    std::int32_t logical_length;
};
inline std::string fallback_nonnull_value(EffectiveFilenameValue filename,
        std::string_view raw_segment) {
    // Ordinary successful62EC20 non-null input/destination VALUE route only.
    // Negative filename length makes81D6B0 clear its initially empty prefix.
    // No NULL-input truncation, allocation-failure parity or owner is issued.
    std::string_view prefix;
    if (filename.logical_length >= 0) {
        const auto length = static_cast<std::size_t>(filename.logical_length);
        if (length > filename.storage.size())
            throw std::out_of_range("Native filename logical bytes unavailable");
        const auto searched = c_prefix(filename.storage.substr(0,length));
        const auto last = searched.rfind('\\'); // strrchr after temporary NUL at logical end
        if (last != std::string_view::npos) {
            //81D6D9 restores the original end byte BEFORE strchr(':').
            // Its raw C prefix can exceed the temporary logical search bound.
            const auto colon = c_prefix(filename.storage).find(':');
            const auto start = colon == std::string_view::npos ? 0 : colon+1;
            const auto end = last+1;
            // Actual81D3B0 clamps a negative substring count to zero.
            if (start <= end) prefix = searched.substr(start,end-start);
        }
    }
    const auto segment = c_prefix(raw_segment); //6F20F0 counts only raw first-NUL bytes
    constexpr auto max = std::size_t(std::numeric_limits<std::int32_t>::max());
    const bool separator = prefix.empty() || prefix.back() != '\\';
    if (segment.size() > max || prefix.size() > max-segment.size() ||
        (separator && prefix.size()+segment.size() == max))
        throw std::length_error("Native fallback successful logical length exceeds signed32");
    std::string result(prefix);
    if (separator) result.push_back('\\'); // non-null EMPTY input still takes this branch
    result.append(segment);
    return result;
}
inline std::string fallback_color_nonnull_value(EffectiveFilenameValue filename,
        std::string_view raw_segment) {
    auto result = fallback_nonnull_value(filename,raw_segment);
    // Separate native CALLER5EB61C..624 effect, not part of62EC20.
    for (auto& byte : result) if (byte == '/') byte = '\\';
    return result;
}
} // namespace civ5::ui::native_color_load_detail
