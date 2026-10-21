#pragma once
#include "civ5/materials/material_cache_rebuild.hpp"
#include "material_cache_lifecycle_validation.hpp"
#include <functional>

namespace cache_rebuild_validation {
using cache_lifecycle_validation::Case;
using cache_lifecycle_validation::Event;
using cache_acquisition_validation::Bytes;
using cache_acquisition_validation::get;
using cache_acquisition_validation::put;
using cache_acquisition_validation::require;
using namespace civ5::materials;
struct Backend final : MaterialCacheRebuildBackend {
    Case &test;
    std::uint32_t next_packet, shared_triangles, capability;
    std::array<unsigned, 3> buffer_references{3, 5, 7};
    std::map<std::uintptr_t, Bytes> staging;
    std::map<std::uintptr_t, std::uint32_t> labels;
    std::map<std::uint32_t, MaterialCachePacket> packets;
    std::map<std::uint32_t, unsigned> packet_references{{11, 1}, {12, 2}};
    std::vector<std::uint32_t> pending_releases;
    std::vector<Event> events;
    std::function<void(unsigned)> on_packet;
    Backend(Case &c, unsigned first, unsigned triangles, unsigned cap)
        : test(c), next_packet(first), shared_triangles(triangles), capability(cap) {
        auto &s = test.state;
        s.geometry.detail_levels = {get(test.entering, 0x1c4), get(test.entering, 0x1c8)};
        for (unsigned i = 0; i < 2; ++i) {
            const auto size = i == 0 ? s.geometry.vertex_count * 24 : s.geometry.triangle_count * 6;
            Bytes storage(size, static_cast<std::uint8_t>(0xa5 + i));
            const auto token = reinterpret_cast<std::uintptr_t>(storage.data());
            require(token > UINT32_MAX, "Rebuild staging must retain pointer width");
            labels.emplace(token, static_cast<std::uint32_t>(s.staging[i]));
            staging.emplace(token, std::move(storage));
            s.staging[i] = token;
        }
        test.staging_label = [this](std::uintptr_t token) { return token ? labels.at(token) : 0u; };
    }
    Bytes snapshot() const {
        auto bytes = test.snapshot();
        put(bytes, 0x1c4, test.state.geometry.detail_levels[0]);
        put(bytes, 0x1c8, test.state.geometry.detail_levels[1]);
        return bytes;
    }
    void record(unsigned kind, std::vector<std::uint64_t> args) {
        events.push_back({kind, std::move(args), snapshot()});
    }
    void upload_geometry_pair(std::uintptr_t vertices, std::uint32_t vertex_count,
                              std::uintptr_t indices, std::uint32_t index_count,
                              std::uint32_t &vertex_buffer, std::uint32_t &index_buffer) override {
        record(0, {labels.at(vertices), vertex_count, labels.at(indices), index_count});
        require(staging.at(vertices).size() >= std::size_t(vertex_count) * 24 &&
                    staging.at(indices).size() >= std::size_t(index_count) * 2,
                "Observed geometry input capacity");
        vertex_buffer = 71;
        index_buffer = 73;
    }
    void shared_geometry(std::uint32_t columns, std::uint32_t rows, std::uint32_t &vertex,
                         std::uint32_t &primary, std::uint32_t &secondary, std::uint32_t &triangles) override {
        record(1, {columns, rows});
        vertex = 81;
        primary = 83;
        secondary = capability >= 10 ? 85 : 0;
        triangles = shared_triangles;
    }
    void retain_vertex_buffer(std::uint32_t handle) override {
        record(2, {handle});
        require(handle == 81, "Unknown retained fixture vertex buffer");
        ++buffer_references[0];
    }
    void retain_index_buffer(std::uint32_t handle) override {
        record(3, {handle});
        if (!handle) return;
        require(handle == 83 || handle == 85, "Unknown retained fixture index buffer");
        ++buffer_references[handle == 83 ? 1 : 2];
    }
    void free_staging(std::uintptr_t token) override {
        record(4, {test.staging_label(token)});
        if (token) require(staging.erase(token) == 1, "Repeated staging free");
    }
    MaterialCachePacket &allocate_land_packet(std::uint32_t &handle) override {
        const auto &handles = test.state.geometry.packets;
        unsigned slot = 0;
        while (slot < handles.size() && &handle != &handles[slot]) ++slot;
        require(slot < handles.size(), "Packet output must reference live owner");
        record(5, {0x154 + slot * 4});
        if (on_packet) on_packet(slot);
        handle = next_packet++;
        require(packet_references.emplace(handle, 1).second, "Packet handle collision");
        return packets[handle];
    }
    void release_packet(std::uint32_t handle) override {
        record(7, {handle});
        if (handle && --packet_references.at(handle) == 0) pending_releases.push_back(handle);
    }
};
inline Bytes packet_bytes(const MaterialCachePacket &p) {
    Bytes out(100, 0);
    put(out, 0, p.key);
    for (unsigned i = 0; i < 5; ++i) put(out, 4 + i * 4, p.geometry[i]);
    const auto half = [&](unsigned offset, std::uint16_t value) {
        out[offset] = static_cast<std::uint8_t>(value);
        out[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    };
    for (unsigned i = 0; i < 16; ++i) {
        half(0x18 + i * 2, p.pixel_textures[i]);
        half(0x38 + i * 2, p.pixel_samplers[i]);
    }
    for (unsigned i = 0; i < 2; ++i) {
        half(0x58 + i * 2, p.vertex_textures[i]);
        half(0x5c + i * 2, p.vertex_samplers[i]);
    }
    out[0x60] = p.pixel_count;
    out[0x61] = p.vertex_count;
    return out;
}
} // namespace cache_rebuild_validation
