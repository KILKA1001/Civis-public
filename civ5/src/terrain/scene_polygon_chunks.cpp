#include "civ5/terrain/scene_polygon_chunks.hpp"
#include <stdexcept>

namespace civ5::terrain {
bool ScenePolygonPointAssembly::append(const ScenePolygonPointChunk &packet){
    // Valid source packets cover at most 512 points, the downstream polygon
    // producer's bounded domain. Preflight before changing the assembly.
    if(packet.block_index>31 || packet.point_count==0 || packet.point_count>16 || packet.points.size()!=16 ||
       packet.block_index*16+packet.point_count>512 ||
       packet.expected_packets==0xffffffffu || received_packets_==0xffffffffu)
        throw std::invalid_argument("Invalid scene polygon point packet");
    const auto first=packet.block_index*16;
    const auto end=first+packet.point_count;
    if(end<points_.size())
        throw std::invalid_argument("Scene polygon point packet would regress original list extent");
    if(expected_packets_==0xffffffffu){expected_packets_=packet.expected_packets;received_packets_=0;}
    ++received_packets_;
    if(end>points_.size())points_.resize(end,packet.points[0]);
    for(std::uint32_t i=0;i<packet.point_count;++i)points_[first+i]=packet.points[i];
    return expected_packets_==received_packets_;
}
} // namespace civ5::terrain
