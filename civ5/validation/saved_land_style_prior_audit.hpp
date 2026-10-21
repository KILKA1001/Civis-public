#pragma once
#include "material_world_validation.hpp"

// Diagnostic only: identify retained LAND owner bytes that the saved source
// grid does not overwrite. A zero-prior counterfactual is not a native value.
namespace saved_land_style_prior_audit {
inline void write(const MaterialWorld &world, const WorldRequest &request, const fs::path &path) {
    const auto &terrain = world.terrain();
    const auto &grid = terrain.source().layout.worker_grid;
    const auto &styles = terrain.land_styles();
    constexpr auto stride = LandStyleGrid::stride;
    constexpr auto extent = LandStyleGrid::size;
    check(grid.width > 0 && grid.height > 0 && grid.width <= stride && grid.height <= stride &&
              request.prior.size() == extent && request.masks.size() == styles.masks.size() &&
              styles.ordinary_styles.size() == styles.masks.size(),
          "Saved LAND prior audit extent differs from the world");
    std::vector<bool> copied(extent, false), prior_addresses(extent, false);
    for (int x = 0; x < grid.width; ++x)
        for (int y = 0; y < grid.height; ++y)
            copied[x + y * stride] = true;
    auto replayed = request.masks, zero_prior = request.masks;
    std::size_t prior_reads = 0;
    for (const auto &read : styles.reads) {
        check(read.patch_index >= 0 && static_cast<std::size_t>(read.patch_index) < replayed.size() &&
                  read.flat_address >= 0 && read.flat_address < extent &&
                  read.style >= 0 && read.style <= 255,
              "Saved LAND prior audit read differs from the owner grid");
        const auto patch = static_cast<std::size_t>(read.patch_index);
        const auto address = static_cast<std::size_t>(read.flat_address);
        replayed[patch] |= 1u << (read.style & 31);
        if (copied[address]) {
            zero_prior[patch] |= 1u << (read.style & 31);
        } else {
            ++prior_reads;
            prior_addresses[address] = true;
            check(request.prior[address] == read.style,
                  "Saved LAND prior read differs from the controlled owner byte");
            zero_prior[patch] |= 1u;
        }
    }
    check(replayed == styles.masks, "Saved LAND read replay differs from the generated masks");
    for (std::size_t i = 0; i < replayed.size(); ++i)
        check(style_from_mask(replayed[i]) == styles.ordinary_styles[i],
              "Saved LAND ordinary style differs from the generated mask");
    std::vector<std::size_t> addresses, changed_masks, changed_styles;
    for (std::size_t i = 0; i < prior_addresses.size(); ++i)
        if (prior_addresses[i]) addresses.push_back(i);
    for (std::size_t i = 0; i < replayed.size(); ++i) {
        if (replayed[i] != zero_prior[i]) changed_masks.push_back(i);
        if (style_from_mask(replayed[i]) != style_from_mask(zero_prior[i]))
            changed_styles.push_back(i);
    }
    std::ostringstream out;
    out << "{\"schema\":\"civ5-saved-land-style-prior-v1\",\"sourceGrid\":["
        << grid.width << ',' << grid.height << "],\"patches\":" << replayed.size()
        << ",\"orderedReads\":" << styles.reads.size()
        << ",\"priorReads\":" << prior_reads << ",\"priorAddresses\":[";
    for (std::size_t i = 0; i < addresses.size(); ++i)
        out << (i ? "," : "") << addresses[i];
    out << "],\"changedMasksIfPriorZero\":" << changed_masks.size()
        << ",\"changedStylesIfPriorZero\":" << changed_styles.size()
        << ",\"changedStylePatchesIfPriorZero\":[";
    for (std::size_t i = 0; i < changed_styles.size(); ++i)
        out << (i ? "," : "") << changed_styles[i];
    out << "],\"zeroPriorIsOriginalClaim\":false,\"fullGraphicsParity\":false}\n";
    blend_validation::text(path, out.str());
}
} // namespace saved_land_style_prior_audit
