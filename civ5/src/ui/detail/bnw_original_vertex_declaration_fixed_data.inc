// Exact initialized Original definitions; CPU indices below are not native resource IDs.
namespace {
struct Original8e1060Type {std::uint8_t type;std::uint32_t size;};
struct Original8e1060Usage {std::uint8_t usage,index;};
struct Original8e1060Record {std::uint32_t usage_key,type_key;std::uint16_t offset;};
struct Original8e1060Row {std::uint32_t first,count;};
inline constexpr std::array<Original8e1060Type,9> original8e1060_types{{{0,4},{1,8},{2,12},{3,16},{15,4},{16,8},{17,2},{5,4},{8,4}}};
inline constexpr std::array<Original8e1060Usage,14> original8e1060_usages{{{0,0},{3,0},{5,0},{5,1},{5,2},{5,3},{10,0},{10,1},{1,0},{1,1},{2,0},{2,1},{6,0},{7,0}}};
inline constexpr std::array<Original8e1060Record,60> original8e1060_records{{{0,7,0},{0,2,0},{1,2,12},{2,1,24},{0,5,0},{1,5,8},{2,4,16},{0,2,0},{8,8,12},{10,7,16},{1,2,20},{2,1,32},{0,5,0},{8,8,8},{10,7,12},{1,5,16},{2,4,24},{0,2,0},{1,2,12},{6,8,24},{7,8,28},{2,1,32},{0,2,0},{6,8,12},{7,8,16},{2,1,20},{12,3,28},{13,2,44},{1,2,56},{0,2,0},{6,8,12},{7,8,16},{2,1,20},{12,3,28},{13,2,44},{1,2,56},{8,8,68},{10,7,72},{0,2,0},{1,2,12},{6,8,24},{7,8,28},{2,1,32},{3,1,40},{4,1,48},{5,1,56},{0,2,0},{6,8,12},{2,1,16},{0,2,0},{2,1,12},{3,1,20},{6,8,28},{0,2,0},{2,1,12},{6,8,20},{10,7,24},{0,2,0},{0,1,0},{0,3,0}}};
inline constexpr std::array<Original8e1060Row,29> original8e1060_rows{{{0,1},{1,3},{4,3},{7,5},{12,5},{17,5},{22,7},{29,9},{38,8},{46,3},{49,4},{53,4},{57,1},{58,1},{59,1},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0},{60,0}}};
}
