#pragma once
#include "material_blend_native.hpp"
#include <iomanip>
#include <sstream>

namespace color_key_validation {
using namespace blend_validation;
// Independent original-producer BGRA files feed the original shaders' D3D9
// device. The production GL path loads its own TGA inputs through C++.
struct NativeColorKeys {
    std::vector<Com<IDirect3DVolumeTexture9>> volumes;
    explicit NativeColorKeys(IDirect3DDevice9 *device, const std::filesystem::path &reference) {
        for (unsigned i = 0; i < 25; ++i) {
            std::ostringstream name;
            name << std::setfill('0') << std::setw(2) << i << ".bgra";
            const auto bytes = read(reference / name.str());
            require(bytes.size() == 131072, "Complete original ColorKey BGRA volume required");
            Com<IDirect3DVolumeTexture9> volume;
            hr(device->CreateVolumeTexture(32, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &volume.value,
                                           nullptr),
               "Original ColorKey volume format");
            for (bool verify : {false, true}) {
                D3DLOCKED_BOX box{};
                hr(volume->LockBox(0, &box, nullptr, verify ? D3DLOCK_READONLY : 0), "ColorKey box lock");
                bool valid = box.RowPitch >= 128 && box.SlicePitch >= std::int64_t(box.RowPitch) * 32;
                if (valid)
                    for (unsigned z = 0; z < 32; ++z)
                        for (unsigned y = 0; y < 32; ++y) {
                            auto *dest = static_cast<char *>(box.pBits) + std::size_t(z) * box.SlicePitch +
                                         std::size_t(y) * box.RowPitch;
                            const auto *source = bytes.data() + (z * 32 + y) * 128;
                            if (verify)
                                valid = valid && std::memcmp(dest, source, 128) == 0;
                            else
                                std::memcpy(dest, source, 128);
                        }
                hr(volume->UnlockBox(0), "ColorKey box unlock");
                require(valid, "Original ColorKey volume upload identity");
            }
            volumes.push_back(std::move(volume));
        }
    }
    IDirect3DVolumeTexture9 *at(unsigned system, unsigned style) const {
        require(system < 5 && style < 5, "Native ColorKey system/style domain");
        return volumes.at(system * 5 + style).value;
    }
};
} // namespace color_key_validation
