#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
std::string quote(const std::string &value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (const auto byte : value) {
        const auto c = static_cast<unsigned char>(byte);
        if (c == '\\' || c == '\"') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 32) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 15];
        } else {
            result += static_cast<char>(c);
        }
    }
    return result + '"';
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout << "civis_payload --save FILE_OR_DIRECTORY\n"
                             "Reads an original .Civ5Save or a converted .CivisSave in the x64 "
                             "importer without executing the original game.\n";
                return 0;
            }
            if (option != L"--save" || ++i == argc)
                throw std::runtime_error("Supply --save FILE_OR_DIRECTORY");
            save = argv[i];
        }
        if (save.empty())
            throw std::runtime_error("Supply --save FILE_OR_DIRECTORY");
        if (runtime::ascii_lower(assets::path_utf8(save.extension())) == ".civissave") {
            const auto loaded = saves::read_loaded_save(save);
            std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                      << ",\"operation\":\"civis_save_payload\",\"can_continue\":false"
                      << ",\"save\":" << quote(assets::path_utf8(save))
                      << ",\"source_sha256\":"
                      << quote(loaded.source_sha256.value_or(std::string{}))
                      << ",\"turn\":" << loaded.header.turn
                      << ",\"payload_bytes\":" << loaded.bytes.size()
                      << ",\"source_prefix_retained\":" << (loaded.source_prefix ? "true" : "false")
                      << ",\"source_prefix_bytes\":" << (loaded.source_prefix ? std::to_string(loaded.source_prefix->size()) : "null")
                      << ",\"metadata_mods\":" << loaded.header.mods.size()
                      << ",\"metadata_dlcs\":" << loaded.header.dlcs.size() << "}\n";
            return 0;
        }
        const auto selection = runtime::select_save(save);
        if (!selection.selected)
            throw std::runtime_error("No supported save header");
        const auto &candidate = selection.candidates[*selection.selected];
        const auto payload = saves::read_loaded_save(candidate.path);
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"save_payload\",\"can_continue\":false"
                  << ",\"save\":" << quote(assets::path_utf8(candidate.path))
                  << ",\"turn\":" << payload.header.turn
                  << ",\"frame_offset\":" << payload.frame.offset
                  << ",\"frame_end\":" << payload.frame.end
                  << ",\"compressed_bytes\":" << payload.inflate.compressed_bytes
                  << ",\"inflated_bytes\":" << payload.bytes.size()
                  << ",\"source_prefix_retained\":" << (payload.source_prefix ? "true" : "false")
                  << ",\"source_prefix_bytes\":" << (payload.source_prefix ? std::to_string(payload.source_prefix->size()) : "null")
                  << ",\"stream_end\":" << (payload.inflate.stream_end ? "true" : "false")
                  << ",\"sync_flush_suffix\":"
                  << (payload.inflate.sync_flush_suffix ? "true" : "false") << "}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"save_payload\",\"can_continue\":false"
                  << ",\"error\":" << quote(error.what()) << "}\n";
        return 2;
    }
}
