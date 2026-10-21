#include "civ5/terrain/terrain_pieces.hpp"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace civ5::terrain {
namespace {
constexpr std::size_t record_stride = 0x56c;
constexpr std::size_t maximum_bytes = 256 * 1024 * 1024;
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
std::uint16_t u16(std::span<const std::uint8_t> b, std::size_t p) {
    return std::uint16_t(b[p]) | std::uint16_t(b[p + 1]) << 8;
}
std::uint32_t u32(std::span<const std::uint8_t> b, std::size_t p) {
    return std::uint32_t(u16(b, p)) | std::uint32_t(u16(b, p + 2)) << 16;
}
struct Section {
    std::size_t offset, count;
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    std::uint32_t word() {
        require(bytes.size() - at >= 4, "Truncated terrain library count");
        const auto value = u32(bytes, at);
        at += 4;
        return value;
    }
    Section section(std::size_t width) {
        const auto count = word();
        require(count <= (bytes.size() - at) / width, "Truncated terrain library pool or records");
        const Section result{at, count};
        at += count * width;
        return result;
    }
};
std::string name(std::span<const std::uint8_t> record, std::size_t offset) {
    const auto slot = record.subspan(offset, 64);
    const auto end = std::find(slot.begin(), slot.end(), 0);
    require(end != slot.end(), "Terrain piece name lacks in-slot terminator");
    require(std::all_of(slot.begin(), end, [](auto b) { return b < 128; }),
            "Terrain piece name is not ASCII");
    return std::string(slot.begin(), end);
}
} // namespace
struct TerrainPieceLibrary::Storage {
    struct Cache {
        std::mutex mutex;
        std::shared_ptr<const TerrainPieceGrids> grids;
    };
    assets::Bytes bytes;
    Section starts, byte_pool, u16_pool, primary;
    std::vector<TerrainPieceRecord> records;
    std::vector<std::unique_ptr<Cache>> caches;
    std::optional<assets::AssetHandle> asset;
};
TerrainMarkup TerrainPieceRecord::markup_at(MarkupCoordinate coordinate) const {
    require(coordinate.x < 10 && coordinate.y < 10, "Terrain markup is outside 10x10 storage");
    const auto at = (std::size_t(coordinate.x) + coordinate.y * 10) * 8;
    return {u32(markup, at), markup[at + 4], markup[at + 5]};
}
TerrainPieceLibrary TerrainPieceLibrary::parse(std::span<const std::uint8_t> bytes) {
    require(bytes.size() <= maximum_bytes, "Terrain library exceeds size limit");
    Reader r{bytes};
    require(r.word() == 0x14, "Unsupported terrain library format compatibility");
    const auto starts = r.section(4), byte_pool = r.section(1), u16_pool = r.section(2),
               primary = r.section(record_stride);
    require(r.at == bytes.size(), "Trailing terrain library bytes");
    require(primary.count > 0 && primary.count <= 4096, "Unsupported terrain piece count");
    auto s = std::make_shared<Storage>();
    s->bytes.assign(bytes.begin(), bytes.end());
    s->starts = starts;
    s->byte_pool = byte_pool;
    s->u16_pool = u16_pool;
    s->primary = primary;
    s->records.reserve(primary.count);
    s->caches.reserve(primary.count);
    for (std::size_t i = 0; i < primary.count; ++i) {
        const auto raw = bytes.subspan(primary.offset + i * record_stride, record_stride);
        require(u32(raw, 0x94) == 10 && u32(raw, 0x98) == 10 && u32(raw, 0x3c0) == 10 &&
                    u32(raw, 0x3c4) == 10,
                "Unsupported terrain markup dimensions");
        TerrainPieceRecord p;
        p.ordinal = static_cast<std::uint32_t>(i);
        p.serialized_ordinal = u32(raw, 0x3bc);
        p.generation_pass = u32(raw, 0x3c8);
        p.raw_state = u32(raw, 0x3d0);
        p.row_tables = {u32(raw, 0x3d4), u32(raw, 0x3d8), u32(raw, 0x3dc), u32(raw, 0x3e0)};
        p.result_code = u16(raw, 0x3e4);
        p.selection_key_a = u16(raw, 0x3e6);
        p.selection_key_b = u16(raw, 0x3e8);
        p.primary_name = name(raw, 0x3ea);
        p.secondary_name = name(raw, 0x42a);
        for (unsigned j = 0; j < 4; ++j)
            p.dds_names[j] = name(raw, 0x46a + j * 64);
        std::copy_n(raw.begin() + 0x9c, 800, p.markup.begin());
        // Native cooked coordinate order is X outermost, despite row-major
        // serialized markup storage. The old compact export reversed this.
        for (std::uint8_t x = 0; x < 10; ++x)
            for (std::uint8_t y = 0; y < 10; ++y) {
                const auto m = p.markup_at({x, y});
                if (m.mask != 0xffffffffu || m.claim != 0)
                    p.active_coordinates.push_back({x, y});
            }
        const std::array<std::uint64_t, 4> tables{p.row_tables.heights, p.row_tables.materials,
                                                p.row_tables.weights, p.row_tables.priorities};
        const auto width = static_cast<std::uint64_t>(p.size());
        for (unsigned j = 0; j < 3; ++j)
            require(tables[j] + width == tables[j + 1], "Terrain row tables disagree with source size");
        require(tables[3] + width <= starts.count, "Terrain row table exceeds pool");
        for (unsigned grid = 0; grid < 4; ++grid)
            for (std::size_t row = 0; row < width; ++row) {
                const auto start = u32(bytes, starts.offset + (tables[grid] + row) * 4);
                require(start < (grid == 0 ? u16_pool.count : byte_pool.count),
                        "Terrain encoded row start exceeds pool");
            }
        s->records.push_back(std::move(p));
        s->caches.push_back(std::make_unique<Storage::Cache>());
    }
    return TerrainPieceLibrary(std::move(s));
}
TerrainPieceLibrary TerrainPieceLibrary::load(const assets::AssetSource &source, const assets::AssetKey &key) {
    require(key.safe_relative_path(), "Unsafe terrain library asset key");
    auto found = source.find(key);
    const auto handle = std::get_if<assets::AssetHandle>(&found);
    require(handle != nullptr, "Missing or ambiguous terrain library asset");
    auto result = parse(handle->read_bytes(maximum_bytes));
    result.storage_->asset = *handle;
    return result;
}
std::span<const TerrainPieceRecord> TerrainPieceLibrary::records() const { return storage_->records; }
const TerrainPieceRecord &TerrainPieceLibrary::record(std::uint32_t ordinal) const {
    require(ordinal < records().size(), "Terrain piece ordinal exceeds library");
    return storage_->records[ordinal];
}
std::span<const std::uint8_t> TerrainPieceLibrary::serialized_bytes() const { return storage_->bytes; }
std::span<const std::uint8_t> TerrainPieceLibrary::serialized_record(std::uint32_t ordinal) const {
    record(ordinal);
    return serialized_bytes().subspan(storage_->primary.offset + ordinal * record_stride, record_stride);
}
const std::optional<assets::AssetHandle> &TerrainPieceLibrary::asset() const { return storage_->asset; }
std::shared_ptr<const TerrainPieceGrids> TerrainPieceLibrary::grids(std::uint32_t ordinal) const {
    const auto &p = record(ordinal);
    auto &cache = *storage_->caches[ordinal];
    const std::lock_guard lock(cache.mutex);
    if (cache.grids)
        return cache.grids;
    const auto width = static_cast<std::size_t>(p.size()), count = width * width;
    const std::array<std::uint32_t, 4> tables{p.row_tables.heights, p.row_tables.materials,
                                           p.row_tables.weights, p.row_tables.priorities};
    std::vector<std::uint16_t> heights(count);
    std::array<std::vector<std::uint8_t>, 3> channels;
    for (auto &c : channels)
        c.resize(count);
    const auto bytes = serialized_bytes();
    for (unsigned grid = 0; grid < 4; ++grid) {
        const auto pool = grid == 0 ? storage_->u16_pool : storage_->byte_pool;
        const std::size_t unit = grid == 0 ? 2 : 1;
        for (std::size_t row = 0; row < width; ++row) {
            std::size_t cursor = u32(bytes, storage_->starts.offset + (tables[grid] + row) * 4);
            std::size_t emitted = 0;
            while (emitted < width) {
                require(cursor <= pool.count && pool.count - cursor >= 2, "Truncated terrain RLE pair");
                const auto at = pool.offset + cursor * unit;
                const auto length = grid == 0 ? u16(bytes, at) : bytes[at];
                const auto value = grid == 0 ? u16(bytes, at + unit) : bytes[at + unit];
                require(length != 0, "Zero-length terrain RLE pair");
                const auto emit = std::min<std::size_t>(length, width - emitted);
                if (grid == 0)
                    std::fill_n(heights.begin() + row * width + emitted, emit, value);
                else
                    std::fill_n(channels[grid - 1].begin() + row * width + emitted, emit,
                                static_cast<std::uint8_t>(value));
                emitted += emit;
                cursor += 2;
            }
        }
    }
    cache.grids = std::make_shared<const TerrainPieceGrids>(p.size(), heights, channels[0], channels[1],
                                                         channels[2]);
    return cache.grids;
}
} // namespace civ5::terrain
