#include "civ5/materials/material_cache_passes.hpp"
#include "civ5/materials/material_normals.hpp"
#include "civ5/materials/material_shadows.hpp"
#include "civ5/materials/material_sources.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
unsigned size_index(const MaterialCachePassSettings &s) {
    require(s.temporary_size >= 1 && s.temporary_size <= 16384 && s.shadow_samples <= 32 &&
                (s.view == 0 || s.view == 5) &&
                (s.precision == terrain::X87Precision::single ||
                 s.precision == terrain::X87Precision::double_precision),
            "Unsupported material cache pass settings");
    for (unsigned i = 0; i < 4; ++i)
        if (s.patch_size == (64u << i))
            return i;
    throw std::invalid_argument("Material cache patch size must be 64/128/256/512");
}
void write_float(std::span<std::uint8_t> bytes, unsigned offset, float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    for (unsigned i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(bits >> (8 * i));
}
void validate(const MaterialCachePassPatch &p, const MaterialCachePassPalette &a) {
    require(p.columns >= 3 && p.columns <= 67 && p.rows >= 3 && p.rows <= 67,
            "Material cache geometry extent outside 3..67");
    require(a.selected.size() <= 8 && a.source_rows.size() <= 64 &&
                a.source_modes.size() == a.source_rows.size() && !a.row_scales.empty() &&
                a.row_scales.size() <= 64 && a.diffuse.size() == a.row_scales.size() &&
                a.specular.size() == a.row_scales.size() && a.height.size() == a.row_scales.size() &&
                std::isfinite(a.global_minimum) && std::isfinite(a.global_maximum),
            "Invalid material cache pass palette");
    const float span = a.global_maximum - a.global_minimum;
    require(std::isfinite(span) && span != 0, "Material cache global span must be finite and nonzero");
    for (auto slot : a.selected) {
        require(slot >= 0 && static_cast<std::size_t>(slot) < a.source_rows.size(),
                "Selected cache material is unavailable");
        const auto row = a.source_rows[slot], mode = a.source_modes[slot];
        require(row >= 0 && static_cast<std::size_t>(row) < a.row_scales.size() && mode >= 0 && mode <= 2,
                "Unavailable cache material projection");
    }
}
void blend_constants(std::span<std::uint8_t> out, const MaterialCachePassSettings &s,
                     const MaterialCachePassPatch &p, const MaterialCachePassPalette &a) {
    // Original x87 half-texel pair; selected coefficients below use SSE scalar
    // operations, independently of the x87 precision control.
    const auto half = static_cast<float>(terrain::round_x87(
        1.0 / terrain::round_x87(double(s.temporary_size) * 2.0, s.precision), s.precision));
    write_float(out, 0, -half);
    write_float(out, 4, half);
    write_float(out, 0x490, static_cast<float>(p.world_rebase[0]));
    write_float(out, 0x494, static_cast<float>(p.world_rebase[1]));
    std::array<std::array<std::uint32_t, 64>, 3> slots{};
    const float span = a.global_maximum - a.global_minimum;
    for (unsigned i = 0; i < a.selected.size(); ++i) {
        const auto selected = a.selected[i], row = a.source_rows[selected], mode = a.source_modes[selected];
        const auto multiplier = material_texture_multiplier(a.row_scales[row], mode, span);
        slots[mode][row] = i;
        write_float(out, 0x10 + 16 * i, multiplier);
        write_float(out, 0x14 + 16 * i, static_cast<float>(mode));
        write_float(out, 0x18 + 16 * i, 0.f);
        write_float(out, 0x1c + 16 * i, 0.f);
    }
    for (unsigned i = 0; i < 64; ++i) {
        for (unsigned mode = 0; mode < 3; ++mode)
            write_float(out, 0x90 + 16 * i + 4 * mode, static_cast<float>(slots[mode][i]));
        // Original FILD signed value + 2^32 for the unsigned flags table.
        double flags = std::bit_cast<std::int32_t>(a.row_flags[i]);
        if (std::bit_cast<std::int32_t>(a.row_flags[i]) < 0)
            flags = terrain::round_x87(flags + 0x1p32, s.precision);
        write_float(out, 0x9c + 16 * i, static_cast<float>(flags));
    }
}
MaterialCachePacket patch_packet(std::uint32_t key, std::uint32_t vb, std::uint32_t base,
                                  const MaterialCachePassSource &source,
                                  const MaterialCachePassPatch &patch) {
    MaterialCachePacket packet{};
    packet.key = key;
    packet.geometry = {source.patch_index_buffer, vb, 2 * (patch.columns - 1) * (patch.rows - 1),
                       patch.start_index, base};
    return packet;
}
MaterialCachePacket blend_packet(const MaterialCachePassSettings &s, const MaterialCachePassPatch &p,
                                 const MaterialCachePassSource &source, const MaterialCachePassPalette &a) {
    auto packet = patch_packet(0x4c, source.material_vertex_buffer, p.material_base_vertex, source, p);
    packet.pixel_count = s.cache_mode ? 3 : 16;
    for (unsigned i = 0; i < 8; ++i) {
        if (i < a.selected.size()) {
            const auto row = a.source_rows[a.selected[i]];
            packet.pixel_textures[i] = static_cast<std::uint16_t>(a.diffuse[row]);
            packet.pixel_textures[i + 8] = static_cast<std::uint16_t>(a.height[row]);
            if (s.cache_mode)
                packet.pixel_samplers[i] = static_cast<std::uint16_t>(
                    a.specular[row] ? a.specular[row] : source.fallback_texture);
            else
                packet.pixel_samplers[i] = packet.pixel_samplers[i + 8] = 0;
        } else {
            const auto fill = static_cast<std::uint16_t>(s.cache_mode ? source.fallback_texture : 0);
            packet.pixel_textures[i] = packet.pixel_textures[i + 8] = fill;
            if (s.cache_mode)
                packet.pixel_samplers[i] = fill;
        }
    }
    if (s.cache_mode) {
        // The original mode1 writer deliberately overwrites the first three
        // halfwords of its third eight-entry table, then sets count=3.
        // Preserve these observed writes until the consuming path is resolved.
        for (unsigned i = 0; i < 3; ++i)
            packet.pixel_samplers[i] = 0;
        if (s.alternate) {
            packet.vertex_textures[0] = static_cast<std::uint16_t>(p.alternate_height);
            packet.vertex_samplers[0] = 0;
            packet.vertex_count = 1;
        }
    }
    return packet;
}
MaterialCachePacket normal_packet(const MaterialCachePassSource &source) {
    MaterialCachePacket p{};
    p.key = 0x4a;
    p.geometry[0] = source.normal_index_buffer;
    p.geometry[1] = source.normal_vertex_buffer;
    p.geometry[2] = 2;
    p.pixel_count = 2;
    p.pixel_textures[0] = p.pixel_textures[1] = static_cast<std::uint16_t>(source.textures[1]);
    p.pixel_samplers[0] = p.pixel_samplers[1] = 5;
    return p;
}
MaterialCachePacket lighting_packet(const MaterialCachePassSettings &s, const MaterialCachePassPatch &p,
                                    const MaterialCachePassSource &source) {
    auto packet = patch_packet(0x4b, source.lighting_vertex_buffer, p.lighting_base_vertex, source, p);
    auto resources = source.lighting;
    resources.mask = p.continent_mask;
    resources.normal = source.textures[4];
    resources.albedo = source.textures[0];
    resources.specular = s.specular ? source.textures[2] : 0;
    resources.shadow = source.textures[3];
    resources.world_height = p.world_height;
    const auto bindings = material_lighting_bindings(resources);
    packet.pixel_count = 12;
    packet.vertex_count = 1;
    for (unsigned i = 0; i < 12; ++i) {
        packet.pixel_textures[i] = bindings.pixel_textures[i];
        packet.pixel_samplers[i] = bindings.pixel_samplers[i];
    }
    packet.vertex_textures[0] = bindings.vertex_texture;
    packet.vertex_samplers[0] = bindings.vertex_sampler;
    return packet;
}
MaterialCachePacket compress_packet(std::uint32_t texture) {
    MaterialCachePacket p{};
    p.key = 0x29;
    p.geometry[0] = p.geometry[1] = 1;
    p.geometry[2] = 2;
    p.pixel_count = 1;
    p.pixel_textures[0] = static_cast<std::uint16_t>(texture);
    p.pixel_samplers[0] = 0;
    return p;
}
} // namespace
MaterialCacheDecalLayers prepare_material_cache_decals(
    const MaterialCachePassSettings &s, const MaterialCachePassPatch &p,
    std::array<std::int32_t, 2> origin, const TerrainDecalStore &store,
    std::uint32_t index_buffer, std::uint32_t shader_binding,
    std::array<std::uint32_t, 2> available) {
    size_index(s);
    require(p.columns>=3 && p.columns<=67 && p.rows>=3 && p.rows<=67,"Decal cache geometry extent");
    const auto q=[&](double v){return terrain::round_x87(v,s.precision);};
    const std::array<float,4> bounds{
        static_cast<float>(terrain::wrap_add(origin[0],-1)),
        static_cast<float>(terrain::wrap_add(origin[1],-1)),
        static_cast<float>(terrain::wrap_add(origin[0],static_cast<std::int32_t>(p.columns)-2)),
        static_cast<float>(terrain::wrap_add(origin[1],static_cast<std::int32_t>(p.columns)-2))};
    require(bounds[0]<bounds[2] && bounds[1]<bounds[3],"Degenerate decal cache bounds");
    const float rx=static_cast<float>(q(1./q(double(bounds[2])-bounds[0])));
    const float ry=static_cast<float>(q(1./q(double(bounds[1])-bounds[3])));
    terrain::Matrix4 projection{static_cast<float>(q(double(rx)*2)),0,0,0,
        0,static_cast<float>(q(double(ry)*2)),0,0,0,0,-.5f,0,
        static_cast<float>(-q(q(double(bounds[0])+bounds[2])*rx)),
        static_cast<float>(-q(q(double(bounds[3])+bounds[1])*ry)),.5f,1};
    auto shift=terrain::identity_matrix();
    const float half=static_cast<float>(q(1./q(double(s.temporary_size)*2)));
    shift[12]=static_cast<float>(q(double(half)*-2));
    shift[13]=static_cast<float>(q(double(half)*2));
    const auto matrix=terrain::matrix_multiply(projection,shift,s.precision,terrain::MatrixPath::reference_x87);
    return {{prepare_terrain_decal_draws(store,bounds,0,available[0],matrix,index_buffer),
             prepare_terrain_decal_draws(store,bounds,1,available[1],matrix,index_buffer)},shader_binding};
}
std::size_t material_cache_constant_bytes(const MaterialCachePassSettings &s, const MaterialCacheDecalLayers *decals) {
    const auto index = size_index(s);
    std::size_t extra=0;
    if(decals)for(const auto &layer:decals->layers) {
        require(layer.capacity_exhausted?layer.draws.empty():layer.draws.size()==layer.selected,"Malformed cache decal layer");
        require(layer.draws.size()<=131072,"Excessive cache decal arena");
        extra+=layer.draws.size()*112;
    }
    return extra+0x4c0u * (1 + s.shadow_samples) + (s.compression ? 16u * (4 + index) * (1 + s.specular) : 0);
}
MaterialCachePassPlan material_cache_pass_plan(const MaterialCachePassSettings &s,
                                               const MaterialCachePassPatch &p,
                                               const MaterialCachePassSource &source,
                                               const MaterialCachePassPalette &palette,
                                               std::span<const std::uint8_t> entering_constants,
                                               const std::array<float, 8> &entering_normal,
                                               const MaterialCacheDecalLayers *decals) {
    const auto index = size_index(s);
    validate(p, palette);
    require(entering_constants.size() == material_cache_constant_bytes(s,decals),
            "Material cache entering constant arena has the wrong extent");
    MaterialCachePassPlan result{{}, {}, {entering_constants.begin(), entering_constants.end()},
                                material_normal_parameters(static_cast<std::int32_t>(s.temporary_size),
                                                           entering_normal)};
    result.passes.reserve(7 + (4 + index) * 2);
    result.packets.reserve(5 + s.shadow_samples);
    const auto packet = [&](MaterialCachePacket value) {
        const auto id = static_cast<std::uint32_t>(result.packets.size());
        result.packets.push_back(std::move(value));
        return id;
    };
    std::array<std::uint32_t, 2> compression_packets{};
    if (s.compression)
        for (unsigned i = 0; i < 2; ++i)
            compression_packets[i] = packet(compress_packet(source.compression_textures[index][2 + i]));
    const auto normal = packet(normal_packet(source));
    const auto lighting = packet(lighting_packet(s, p, source));
    std::uint32_t constant_offset = 0, constant_bytes = 0x4c0, next_constant = 0x4c0;
    const auto draw = [&](MaterialCachePass &pass, std::uint32_t id, unsigned binding, unsigned selector) {
        pass.draws.push_back({id, source.shader_bindings[binding], selector, constant_offset, constant_bytes});
    };
    const auto pass = [&](MaterialCachePassKind kind, std::array<std::uint32_t, 4> targets,
                          std::uint32_t depth, std::uint32_t blend, std::uint32_t flags,
                          std::array<float, 4> clear = {}) -> MaterialCachePass & {
        MaterialCachePass value{};
        value.kind = kind;
        value.targets = targets;
        value.depth_surface = depth;
        value.blend_state = blend;
        value.flags = flags;
        value.clear_color = clear;
        result.passes.push_back(std::move(value));
        return result.passes.back();
    };
    const auto decal_layer=[&](unsigned mode) {
        if(!decals)return;
        auto &value=pass(mode?MaterialCachePassKind::decal_shadow:MaterialCachePassKind::decal,
            mode?std::array<std::uint32_t,4>{source.textures[3],0,0,0}:
                 std::array<std::uint32_t,4>{source.textures[0],source.textures[1],s.specular?source.textures[2]:0,0},
            mode?0:source.depth_surface,mode?4:8,0);
        for(const auto &d:decals->layers[mode].draws) {
            require(d.triangles && std::uint64_t(d.first_vertex)+std::uint64_t(d.triangles)*3<=65535,"Decal cache draw range");
            constant_offset=next_constant;constant_bytes=112;next_constant+=112;
            auto bytes=std::span(result.constants).subspan(constant_offset,112);
            for(unsigned i=0;i<25;++i) {
                require(std::isfinite(d.constants[i]),"Nonfinite cache decal constants");
                write_float(bytes,i*4,d.constants[i]);
            }
            MaterialCachePacket command{};command.key=0x4e;
            command.geometry={d.index_buffer,d.vertex_buffer,d.triangles,d.first_vertex,0};
            command.pixel_count=2;command.pixel_textures[0]=d.albedo;command.pixel_textures[1]=d.heightmap;
            command.pixel_samplers[0]=2;
            value.draws.push_back({packet(command),decals->shader_binding,0,constant_offset,constant_bytes});
        }
    };
    auto &height = pass(MaterialCachePassKind::height,
                        {source.textures[0], source.textures[1], s.specular ? source.textures[2] : 0, 0},
                        s.specular ? 0 : source.depth_surface, 4, 4);
    blend_constants(std::span(result.constants).first(0x4c0), s, p, palette);
    draw(height, packet(blend_packet(s, p, source, palette)), 0, s.cache_mode ? (s.alternate ? 1 : 2) : 0);
    decal_layer(0);
    auto &normals = pass(MaterialCachePassKind::normal, {source.textures[4], 0, 0, 0}, 0, 0, 128);
    draw(normals, normal, 1, 0);
    auto &shadow = pass(MaterialCachePassKind::shadow, {source.textures[3], 0, 0, 0}, 0, 4, 4);
    const auto jitter = material_shadow_jitter(s.precision);
    for (unsigned i = 0; i < s.shadow_samples; ++i) {
        constant_offset = next_constant;
        constant_bytes = 0x4c0;
        next_constant += 0x4c0;
        auto bytes = std::span(result.constants).subspan(constant_offset, 0x4c0);
        const auto changed = material_shadow_draw_block(s.shadow_samples, p.world_rebase, jitter[i],
                                                        std::span<const std::uint8_t, 0x4c0>(bytes));
        std::copy(changed.begin(), changed.end(), bytes.begin());
        auto command = patch_packet(0x4c, source.material_vertex_buffer, p.material_base_vertex, source, p);
        command.pixel_count = 1;
        command.pixel_textures[0] = static_cast<std::uint16_t>(p.world_height);
        command.pixel_samplers[0] = 0;
        draw(shadow, packet(std::move(command)), 0, 3);
    }
    decal_layer(1);
    const auto light = [&](std::uint32_t diffuse, std::uint32_t specular, bool enabled,
                           std::uint32_t flags) {
        auto &value = pass(MaterialCachePassKind::lighting, {diffuse, enabled ? specular : 0, 0, 0},
                           enabled ? source.depth_surface : 0, 0, flags,
                           {1.f, enabled ? .5f : 1.f, 0.f, 0.f});
        draw(value, lighting, 2, enabled ? 1 : 0);
    };
    if (s.view == 5)
        light(p.higher_cache[0], p.higher_cache[1], s.specular, 128);
    if (!s.compression)
        light(p.final_cache[0], 0, false, 4);
    else {
        light(source.compression_textures[index][2], source.compression_textures[index][3], s.specular, 128);
        for (unsigned channel = 0; channel < (s.specular ? 2u : 1u); ++channel) {
            const auto target = source.compression_textures[index][channel];
            for (unsigned mip = 0; mip < 4 + index; ++mip) {
                auto &value = pass(MaterialCachePassKind::compress, {target, 0, 0, 0}, 0, 0, 32);
                value.mip = mip;
                constant_offset = next_constant;
                constant_bytes = 16;
                next_constant += 16;
                write_float(std::span(result.constants).subspan(constant_offset, 16), 0, static_cast<float>(mip));
                draw(value, compression_packets[channel], 3, 0);
            }
            MaterialCachePass copy{};
            copy.kind = MaterialCachePassKind::copy;
            copy.copy_source = target;
            copy.copy_destination = p.final_cache[channel];
            result.passes.push_back(std::move(copy));
        }
    }
    return result;
}
} // namespace civ5::materials
