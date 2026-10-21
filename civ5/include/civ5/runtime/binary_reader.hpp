#pragma once
#include <cstdint>
#include <istream>
#include <span>
#include <string>
#include <string_view>

namespace civ5::runtime {
static_assert(sizeof(void *) == 8, "Civis runtime requires a 64-bit process");
// Wire widths belong to the imported format. Offsets and allocation budgets
// belong to the host and never pass through an x86 pointer or signed int.
class BinaryReader {
  public:
    BinaryReader(std::istream &stream, std::uint64_t length);
    std::uint64_t position() const { return position_; }
    std::uint64_t remaining() const { return length_ - position_; }
    void seek(std::uint64_t position);
    void read(std::span<std::uint8_t> destination);
    std::uint8_t u8();
    std::uint16_t u16();
    std::int16_t i16();
    std::uint32_t u32();
    std::int32_t i32();
    std::string string32(std::uint64_t maximum_bytes);

  private:
    std::istream &stream_;
    std::uint64_t length_, position_ = 0;
};
bool valid_utf8(std::string_view text);
std::string ascii_lower(std::string text);
} // namespace civ5::runtime
