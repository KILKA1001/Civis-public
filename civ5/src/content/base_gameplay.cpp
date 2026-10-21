#include "civ5/content/base_gameplay.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include <array>
#include <fstream>
#include <stdexcept>

namespace civ5::content {
namespace {
constexpr auto original_hash = "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
// PE fields have their original wire widths. Host offsets and section bounds
// use uint64_t; PE32 is never mapped into the process or called as executable.
class RecipeImage {
    struct Section {
        std::uint64_t address, offset, size;
    };
    runtime::BinaryReader &reader_;
    std::vector<Section> sections_;
    std::uint16_t u16() {
        const auto low = reader_.u8();
        return static_cast<std::uint16_t>(low | (static_cast<unsigned>(reader_.u8()) << 8));
    }
    const Section &section(std::uint64_t address) const {
        for (const auto &item : sections_)
            if (address >= item.address && address - item.address < item.size)
                return item;
        throw std::runtime_error("Unbacked native recipe address");
    }

  public:
    explicit RecipeImage(runtime::BinaryReader &reader) : reader_(reader) {
        require(u16() == 0x5a4d, "Native recipe needs an MZ image");
        reader_.seek(60);
        const std::uint64_t header = reader_.u32();
        reader_.seek(header);
        require(reader_.u32() == 0x4550 && u16() == 0x14c, "Expected original PE32 machine");
        const std::uint64_t count = u16();
        reader_.seek(header + 20);
        const auto optional_size = u16();
        reader_.seek(header + 24);
        require(u16() == 0x10b && optional_size >= 32, "Expected PE32 optional header");
        reader_.seek(header + 52);
        const std::uint64_t base = reader_.u32();
        require(base == 0x400000, "Unexpected original image base");
        for (std::uint64_t i = 0; i < count; ++i) {
            reader_.seek(header + 24 + optional_size + 40 * i + 12);
            const auto address = base + reader_.u32();
            const std::uint64_t size = reader_.u32();
            const std::uint64_t offset = reader_.u32();
            reader_.seek(offset);
            require(size <= reader_.remaining(), "Native recipe section exceeds image");
            sections_.push_back({address, offset, size});
        }
    }
    void seek(std::uint64_t address, std::uint64_t size = 1) {
        const auto &item = section(address);
        require(size <= item.size - (address - item.address), "Native recipe crosses section");
        reader_.seek(item.offset + address - item.address);
    }
    std::string string(std::uint64_t address) {
        const auto &item = section(address);
        seek(address);
        std::string value;
        for (auto available = item.size - (address - item.address); available; --available) {
            const auto c = reader_.u8();
            if (!c)
                return value;
            require(c < 128, "Unexpected non-ASCII native loader literal");
            value += static_cast<char>(c);
        }
        throw std::runtime_error("Unterminated native loader literal");
    }
};
} // namespace
NativeGameplayRecipe read_native_gameplay_recipe(const std::filesystem::path &executable) {
    NativeGameplayRecipe recipe;
    recipe.executable = runtime::identify_file(executable);
    require(recipe.executable.sha256 == original_hash, "Native gameplay loader image is not verified");
    std::ifstream file(executable, std::ios::binary);
    require(static_cast<bool>(file), "Cannot read native loader image");
    runtime::BinaryReader reader(file, recipe.executable.bytes);
    RecipeImage image(reader);
    // Call argument of the engine schema read, EXE 0x54AB99.
    image.seek(0x54ab99, 5);
    require(reader.u8() == 0x68, "Engine schema argument is not a literal");
    recipe.engine_schema = image.string(reader.u32());
    require(recipe.engine_schema == "Assets/SQL/Civ5EngineDatabaseSchema.sql", "Unexpected schema request");
    // 0x698140 reserves room for 111 entries, then emits direct calls. The
    // reserve count is the code's request count, not a Civis resource capacity.
    image.seek(0x69815a, 4);
    require(reader.u8() == 0x8d && reader.u8() == 0x4c && reader.u8() == 0x02,
            "Unexpected gameplay reserve instruction");
    const std::uint64_t declared_count = reader.u8();
    std::uint64_t address = 0x698166, pending_arguments = 0;
    while (address < 0x698652) {
        image.seek(address, 3);
        const auto opcode = reader.u8();
        if (opcode == 0x83) {
            require(reader.u8() == 0xc4 && reader.u8() == 0x40 && pending_arguments == 64,
                    "Unexpected native gameplay argument cleanup");
            pending_arguments = 0;
            address += 3;
            continue;
        }
        require(opcode == 0x68, "Unexpected native gameplay request instruction");
        image.seek(address + 1, 10);
        const auto literal = reader.u32();
        require(reader.u8() == 0x56 && reader.u8() == 0xe8, "Unexpected native gameplay request call");
        const auto target = static_cast<std::int64_t>(address + 11) + reader.i32();
        require(target == 0x69dab0, "Gameplay request calls a different helper");
        recipe.requests.push_back(image.string(literal));
        pending_arguments += 8;
        address += 11;
    }
    require(address == 0x698652 && recipe.requests.size() == declared_count && pending_arguments == 56,
            "Native gameplay sequence boundary changed");
    image.seek(address, 5);
    require(reader.u8() == 0x83 && reader.u8() == 0xc4 && reader.u8() == 0x38 && reader.u8() == 0x5e &&
                reader.u8() == 0xc3,
            "Native gameplay sequence does not return normally");
    require(runtime::identify_file(executable).sha256 == recipe.executable.sha256,
            "Original image changed while reading gameplay recipe");
    return recipe;
}
std::vector<GameplaySource> plan_base_gameplay(std::span<const std::string> requests,
                                               const assets::ResourceMountTable &resources) {
    std::vector<GameplaySource> result;
    for (const auto &request : requests) {
        auto selected = resources.resolve(assets::AssetKey(request));
        if (const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset)) {
            require(handle->candidate().provenance.kind == assets::SourceKind::loose,
                    "Base gameplay physical path for archive backing is not implemented");
        } else
            require(std::holds_alternative<std::monostate>(selected.asset), "Ambiguous base gameplay source");
        result.push_back({request, std::move(selected)});
    }
    return result;
}
} // namespace civ5::content
