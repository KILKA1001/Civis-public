#pragma once
#include "civ5/materials/distance_fog.hpp"
#include "civ5/materials/water_surface.hpp"
#include "material_blend_native.hpp"
#include <limits>
#include <type_traits>

namespace distance_fog_validation {
using namespace blend_validation;
struct NativeDistanceFog {
    struct Library {
        HMODULE handle = LoadLibraryExW(L"d3dx9_42.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        Library() { require(handle != nullptr, "System x64 D3DX9_42 is required for fog DDS loading"); }
        ~Library() { FreeLibrary(handle); }
    } library;
    Com<IDirect3DTexture9> ramp;
    civ5::materials::DistanceFogFrame global{};
    std::array<civ5::terrain::Vector4, 2> overview{};
    std::vector<std::array<civ5::terrain::Vector4, 2>> land;
    std::vector<civ5::materials::WaterConstantBlock> water, animation;
    explicit NativeDistanceFog(IDirect3DDevice9 *device, const std::filesystem::path &reference) {
        const auto folder = reference / "distance-fog";
        const auto dds = read(folder / "ramp.dds");
        using Create = HRESULT(WINAPI *)(IDirect3DDevice9 *, LPCVOID, UINT, IDirect3DTexture9 **);
        const auto create =
            reinterpret_cast<Create>(GetProcAddress(library.handle, "D3DXCreateTextureFromFileInMemory"));
        require(create != nullptr && dds.size() <= std::numeric_limits<UINT>::max(),
                "Original fog DDS entry/extent");
        hr(create(device, dds.data(), static_cast<UINT>(dds.size()), &ramp.value),
           "D3DX original FogRamp.dds load");
        const auto load = [&](const std::filesystem::path &path, auto &values, unsigned maximum) {
            const auto bytes = read(path);
            using Value = typename std::decay_t<decltype(values)>::value_type;
            require(!bytes.empty() && bytes.size() % sizeof(Value) == 0 &&
                        bytes.size() / sizeof(Value) <= maximum,
                    "Complete original fog producer output required");
            values.resize(bytes.size() / sizeof(Value));
            std::memcpy(values.data(), bytes.data(), bytes.size());
        };
        const auto globals = read(folder / "global.bin");
        const auto overview_bytes = read(folder / "overview.bin");
        require(globals.size() == 32 && overview_bytes.size() == 32, "Original fog global/overview extent");
        std::memcpy(&global, globals.data(), 32);
        std::memcpy(overview.data(), overview_bytes.data(), 32);
        load(folder / "land.bin", land, 3);
        load(folder / "water.bin", water, 3);
        load(reference / "constants.bin", animation, 183);
        require(land.size() == water.size() && animation.size() == water.size() * 61,
                "Original fog LAND/water/animation view count");
    }
    Bytes level(unsigned mip) const {
        D3DSURFACE_DESC desc{};
        hr(ramp->GetLevelDesc(mip, &desc), "D3DX fog mip descriptor");
        require(desc.Format == D3DFMT_A8R8G8B8 && desc.Width <= 16384 && desc.Height <= 16384,
                "Original authored fog format");
        D3DLOCKED_RECT lock{};
        hr(ramp->LockRect(mip, &lock, nullptr, D3DLOCK_READONLY), "D3DX fog mip readback");
        const bool valid = lock.pBits != nullptr && lock.Pitch >= static_cast<INT>(desc.Width * 4);
        Bytes result(std::size_t(desc.Width) * desc.Height * 4);
        if (valid)
            for (unsigned row = 0; row < desc.Height; ++row)
                std::memcpy(result.data() + std::size_t(row) * desc.Width * 4,
                            static_cast<const char *>(lock.pBits) + std::size_t(row) * lock.Pitch,
                            desc.Width * 4);
        hr(ramp->UnlockRect(mip), "D3DX fog mip unlock");
        require(valid, "D3DX fog readback pitch");
        return result;
    }
};
} // namespace distance_fog_validation
