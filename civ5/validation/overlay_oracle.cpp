#include "civ5/terrain/overlay.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
using namespace civ5::terrain;
namespace fs = std::filesystem;
namespace {
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 8;
    template <class T> T read() {
        if (at > bytes.size() || bytes.size() - at < sizeof(T))
            throw std::invalid_argument("Truncated overlay input");
        T result;
        std::memcpy(&result, bytes.data() + at, sizeof(T));
        at += sizeof(T);
        return result;
    }
};
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected primitive input and fresh output file");
        const fs::path input(argv[1]), output(argv[2]);
        if (fs::exists(output) || fs::file_size(input) < 12 || fs::file_size(input) > 4 * 1024 * 1024)
            throw std::invalid_argument("Bounded overlay input and fresh output required");
        std::ifstream file(input, std::ios::binary);
        Reader r{{std::istreambuf_iterator<char>(file), {}}};
        if (std::memcmp(r.bytes.data(), "C5OVRL01", 8) != 0)
            throw std::invalid_argument("Overlay primitive schema");
        const auto count = r.read<std::uint32_t>();
        if (count == 0 || count > 2048)
            throw std::invalid_argument("Bounded overlay case count");
        std::vector<OverlayMatrices> result;
        for (unsigned i = 0; i < count; ++i) {
            const auto enabled = r.read<std::uint32_t>();
            CameraFrameState camera{};
            camera.view_count = r.read<std::uint32_t>();
            const auto pc = r.read<std::uint32_t>();
            if (pc != 24 && pc != 53)
                throw std::invalid_argument("Overlay precision");
            camera.frusta[0] = r.read<ViewFrustum>();
            camera.inverse_views[0] = r.read<Matrix4>();
            for (unsigned v = 1; v < 4; ++v)
                camera.views[v] = r.read<Matrix4>();
            const auto prior = r.read<OverlayMatrices>();
            result.push_back(
                overlay_matrices(camera, prior, enabled != 0,
                                 pc == 24 ? X87Precision::single : X87Precision::double_precision));
        }
        if (r.at != r.bytes.size())
            throw std::invalid_argument("Trailing overlay primitive bytes");
        std::ofstream out(output, std::ios::binary);
        out.write(reinterpret_cast<const char *>(result.data()), result.size() * sizeof(OverlayMatrices));
        if (!out)
            throw std::runtime_error("Overlay result write failed");
        std::cout << "Produced " << count << " overlay cases\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
