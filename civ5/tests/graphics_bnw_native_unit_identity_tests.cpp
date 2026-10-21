#include "civ5/graphics/bnw_native_unit_identity.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>

namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejected(F f,const char *why) {
    try {f();} catch(const std::invalid_argument &) {return;}
    throw std::runtime_error(why);
}
using civ5::graphics::BnwNativeUnitIdentityRow;
std::vector<std::byte> encode(std::span<const BnwNativeUnitIdentityRow> rows) {
    std::vector<std::byte> bytes;
    for(const auto &row:rows) for(const auto value:
        {std::bit_cast<std::uint32_t>(row.owner),std::bit_cast<std::uint32_t>(row.unit_id),row.pool_index})
        for(std::size_t i=0;i<4;++i) bytes.push_back(static_cast<std::byte>((value>>(i*8))&255));
    return bytes;
}
}
int main() {
    try {
        using namespace civ5::graphics;
        // Synthetic codec cases only: none of these rows asserts native placement.
        const std::array<BnwNativeUnitIdentityRow,7> rows{{
            {std::numeric_limits<std::int32_t>::min(),-1,0}, {-1,-1,1},
            {0,-1,2}, {0,16385,7}, {0,0x1004001,8}, {1,16385,9},
            {std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max(),10}}};
        auto bytes=encode(rows);
        const auto decoded=decode_bnw_native_unit_identity_rows(bytes,11);
        require(std::equal(decoded.begin(),decoded.end(),rows.begin(),rows.end()),"LE signed decode mismatch");
        for(const auto &row:rows)
            require(lookup_bnw_native_unit_pool_index(decoded,11,row.owner,row.unit_id)==row.pool_index,"exact identity lookup failed");
        require(!lookup_bnw_native_unit_pool_index(decoded,11,0,16386),"absent ID matched");
        require(!lookup_bnw_native_unit_pool_index(decoded,11,2,16385),"absent owner matched");
        require(!lookup_bnw_native_unit_pool_index({},0,0,0),"empty table matched");
        require(decode_bnw_native_unit_identity_rows({},0).empty(),"empty table decode failed");
        bytes.pop_back();
        rejected([&]{decode_bnw_native_unit_identity_rows(bytes,11);},"partial row accepted");
        auto malformed=decoded;
        std::swap(malformed[0],malformed[1]);
        rejected([&]{lookup_bnw_native_unit_pool_index(malformed,11,0,16385);},"unordered keys accepted");
        malformed=decoded; malformed[1]=malformed[0];
        rejected([&]{lookup_bnw_native_unit_pool_index(malformed,11,0,16385);},"duplicate keys accepted");
        malformed=decoded; malformed.back().pool_index=11;
        rejected([&]{lookup_bnw_native_unit_pool_index(malformed,11,0,16385);},"bad unqueried index accepted");
        rejected([&]{decode_bnw_native_unit_identity_rows(encode(rows),0);},"zero pool accepted nonempty rows");
        rejected([&]{lookup_bnw_native_unit_pool_index({},16385,0,0);},"pool reader bound ignored");
        std::vector<std::byte> excessive((bnw_native_unit_identity_reader_limit+1)*12);
        rejected([&]{decode_bnw_native_unit_identity_rows(excessive,1);},"row reader bound ignored");
        std::cout<<"PASS: synthetic codec/lookup 12 positive and 7 malformed cases; no placement generated\n";
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
