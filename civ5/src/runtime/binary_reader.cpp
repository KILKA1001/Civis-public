#include "civ5/runtime/binary_reader.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
BinaryReader::BinaryReader(std::istream &stream, std::uint64_t length) : stream_(stream), length_(length) {
    seek(0);
}
void BinaryReader::seek(std::uint64_t position) {
    if (position > length_ ||
        position > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        throw std::runtime_error("Binary offset outside input");
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(position));
    if (!stream_)
        throw std::runtime_error("Binary seek failed");
    position_ = position;
}
void BinaryReader::read(std::span<std::uint8_t> destination) {
    if (destination.size() > remaining())
        throw std::runtime_error("Truncated binary input at " + std::to_string(position_));
    while (!destination.empty()) {
        const auto count = std::min<std::size_t>(destination.size(), 1024 * 1024);
        stream_.read(reinterpret_cast<char *>(destination.data()), static_cast<std::streamsize>(count));
        if (!stream_)
            throw std::runtime_error("Binary read failed at " + std::to_string(position_));
        position_ += count;
        destination = destination.subspan(count);
    }
}
std::uint8_t BinaryReader::u8() {
    std::uint8_t result;
    read({&result, 1});
    return result;
}
std::uint16_t BinaryReader::u16() {
    std::array<std::uint8_t, 2> bytes{};
    read(bytes);
    return std::uint16_t(bytes[0]) | (std::uint16_t(bytes[1]) << 8);
}
std::int16_t BinaryReader::i16() { return std::bit_cast<std::int16_t>(u16()); }
std::uint32_t BinaryReader::u32() {
    std::array<std::uint8_t, 4> bytes{};
    read(bytes);
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) | (std::uint32_t(bytes[2]) << 16) |
           (std::uint32_t(bytes[3]) << 24);
}
std::int32_t BinaryReader::i32() { return std::bit_cast<std::int32_t>(u32()); }
std::string BinaryReader::string32(std::uint64_t maximum_bytes) {
    const std::uint64_t count = u32();
    if (count > maximum_bytes || count > remaining())
        throw std::runtime_error("String exceeds input or resource budget");
    std::string result(static_cast<std::size_t>(count), '\0');
    read({reinterpret_cast<std::uint8_t *>(result.data()), result.size()});
    if (!valid_utf8(result) || result.find('\0') != std::string::npos)
        throw std::runtime_error("Invalid UTF-8 string");
    return result;
}
bool valid_utf8(std::string_view text) {
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<std::uint8_t>(text[i++]);
        if (first < 0x80)
            continue;
        unsigned count;
        std::uint32_t value, minimum;
        if (first >= 0xc2 && first <= 0xdf) {
            count = 1;
            value = first & 31;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            count = 2;
            value = first & 15;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            count = 3;
            value = first & 7;
            minimum = 0x10000;
        } else
            return false;
        if (count > text.size() - i)
            return false;
        while (count--) {
            const auto next = static_cast<std::uint8_t>(text[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (next & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
std::string ascii_lower(std::string text) {
    for (auto &c : text)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c + ('a' - 'A'));
    return text;
}
} // namespace civ5::runtime
