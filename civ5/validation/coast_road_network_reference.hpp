#pragma once
#include "civ5/terrain/coast_road_network.hpp"

// Validation serialization of the original network oracle. Native pointers are
// normalized; membership, points, bounds, metadata and junction ordering are not.
namespace coast_road_reference {
using Bytes = civ5::assets::Bytes;
inline void put(Bytes &b,std::uint32_t v,unsigned size=4) {
    for (unsigned i=0;i<size;++i) b.push_back(std::uint8_t(v>>(8*i)));
}
inline void real(Bytes &b,float v) { put(b,std::bit_cast<std::uint32_t>(v)); }
inline void blob(Bytes &b,const Bytes &raw) {
    put(b,static_cast<std::uint32_t>(raw.size())); b.insert(b.end(),raw.begin(),raw.end());
}
inline Bytes snapshot(const civ5::terrain::TerrainRoadGrid &g) {
    Bytes result,body,cells;
    put(result,static_cast<std::uint32_t>(g.segments.size())); put(result,static_cast<std::uint32_t>(g.junctions.size()));
    for (const auto &s:g.segments) {
        put(result,static_cast<std::uint32_t>(s.points.size())); put(result,s.last_junction|(std::uint32_t(s.first_junction)<<16));
    }
    for (const auto &n:g.junctions) {
        put(result,static_cast<std::uint32_t>(n.size()));
        for (auto link:n) put(result,link.segment|(std::uint32_t(link.first)<<28)|(std::uint32_t(link.boundary)<<29));
    }
    real(body,g.span_x); real(body,g.span_y); for (auto f:g.bounds) real(body,f);
    for (const auto &c:g.cells) {
        for (auto w:c.geometry) put(cells,w);
        put(cells,c.handle); put(cells,c.dirty,1); for (auto r:c.reserved) put(cells,r,1);
    }
    blob(body,cells);
    for (const auto &m:g.members) { put(body,static_cast<std::uint32_t>(m.size())); for (auto i:m) put(body,i); }
    for (const auto &s:g.segments) {
        for (auto f:s.bounds) real(body,f);
        put(body,s.group); put(body,s.cell,2); put(body,s.type,1); put(body,s.flags,1);
        for (auto p:s.points) { real(body,p.x); real(body,p.y); }
    }
    // This producer makes no random calls/submissions. These are the oracle's
    // unchanged observer fields, not an assertion about a saved-scene cursor.
    for (auto v:{1u,0u,0u,0u}) put(body,v);
    blob(result,body); return result;
}
} // namespace coast_road_reference
