#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace civ5::ui {
// Actual new-client InterfaceDirtyBits setters, not imported engine flags.
struct BnwMajorTechDirtyDelivery {
 std::uint64_t occurrence=0,generation=0,world_revision=0;
 std::int64_t team=-1,technology=-1,player=-1,turn=-1;
 std::string native_name;
 bool operator==(const BnwMajorTechDirtyDelivery&)const=default;
};
struct BnwMajorTechDirtySnapshot {
 bool misc_buttons=false,selection_buttons=false,research_buttons=false;
 std::uint64_t revision=0;
 std::vector<BnwMajorTechDirtyDelivery> deliveries;
 bool operator==(const BnwMajorTechDirtySnapshot&)const=default;
};
}
