#pragma once
#include "dds.hpp"
#include "rgba8.hpp"

namespace civ5::assets {
// A tagged atlas must be expanded with its matching index, never sampled as
// the final image. The tag alone does not validate the tile/storage profile.
bool has_ui_block_atlas_header(const DdsData &data) noexcept;
enum class UiTextureStorageProfile { ordinary_dds, interface_color, indexed_interface };
// Native FTXTInterface COLOR has factor zero and keeps the authored DDS mips.
// Unknown interface storage is rejected rather than treated as a raw image.
UiTextureStorageProfile ui_texture_storage_profile(const DdsData &data);
// Civ V FTXTInterface's BC001/BC002/BC004/BC008 UI layout stores each output tile in an
// atlas DDS and a luminance DDS stores the atlas tile number per output tile.
// This expands the authored pixels; it does not resample or invent UI art.
Rgba8Level expand_ui_block_atlas(const DdsData &atlas, const DdsData &index);
} // namespace civ5::assets
