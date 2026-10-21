#pragma once
namespace {
Bytes serialize_plan(const MaterialCachePassPlan &plan) {
    using generation_validation::blob;
    using generation_validation::raw;
    using generation_validation::word;
    Bytes result{'C', '5', 'C', 'H', 'P', 'L', '0', '1'};
    word(result, static_cast<unsigned>(plan.passes.size()));
    for (const auto &p : plan.passes) {
        word(result, static_cast<unsigned>(p.kind));
        raw(result, std::span<const std::uint32_t>(p.targets));
        for (auto v : {p.mip, p.depth_surface, p.blend_state, p.depth_state, p.flags})
            word(result, v);
        raw(result, std::span<const float>(p.clear_color));
        word(result, std::bit_cast<std::uint32_t>(p.clear_depth));
        word(result, p.copy_source);
        word(result, p.copy_destination);
        word(result, static_cast<unsigned>(p.draws.size()));
        for (const auto &d : p.draws)
            for (auto v : {d.packet, d.shader_binding, d.selector, d.constant_offset, d.constant_bytes})
                word(result, v);
    }
    word(result, static_cast<unsigned>(plan.packets.size()));
    for (const auto &p : plan.packets) {
        word(result, p.key);
        raw(result, std::span<const std::uint32_t>(p.geometry));
        raw(result, std::span<const std::uint16_t>(p.pixel_textures));
        raw(result, std::span<const std::uint16_t>(p.pixel_samplers));
        raw(result, std::span<const std::uint16_t>(p.vertex_textures));
        raw(result, std::span<const std::uint16_t>(p.vertex_samplers));
        result.push_back(p.pixel_count);
        result.push_back(p.vertex_count);
        result.push_back(0);
        result.push_back(0);
    }
    blob(result, plan.constants);
    raw(result, std::span<const float>(plan.normal_constants));
    return result;
}
} // namespace
