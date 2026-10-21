#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/save_payload.hpp"
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
std::string quote(const std::string &value) {
    std::string result = "\"";
    for (const auto byte : value) {
        const auto c = static_cast<unsigned char>(byte);
        if (c == '\\' || c == '"') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 32) {
            result += "\\u00";
            constexpr char hex[] = "0123456789abcdef";
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
        fs::path save, output;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout << "civis_save_convert --save FILE_OR_DIRECTORY --output TARGET.CivisSave\n"
                             "Imports the original save prefix, setup bytes and payload into the x64 Civis save envelope.\n"
                             "The envelope is a foundation for future live session state; it is "
                             "not opened by the original game.\n";
                return 0;
            }
            if ((option != L"--save" && option != L"--output") || ++i == argc)
                throw std::runtime_error("Supply --save and --output");
            if (option == L"--save")
                save = argv[i];
            else
                output = argv[i];
        }
        if (save.empty() || output.empty())
            throw std::runtime_error("Supply --save and --output");
        if (runtime::ascii_lower(assets::path_utf8(output.extension())) != ".civissave")
            throw std::runtime_error("Civis save output must use the .CivisSave extension");
        const auto selection = runtime::select_save(save);
        if (!selection.selected)
            throw std::runtime_error("No supported save header");
        const auto &candidate = selection.candidates[*selection.selected];
        const auto source_identity = runtime::identify_file(candidate.path);
        const auto payload = saves::read_save_payload(candidate.path);
        if (payload.header != candidate.header || runtime::identify_file(candidate.path).sha256 != source_identity.sha256)
            throw std::runtime_error("Original save changed during conversion");
        saves::CivisSaveDocument document;
        document.flags = 1; // imported-original-payload
        document.sections.push_back(
            {static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload), 0, payload.bytes});
        document.sections.push_back(saves::make_save_metadata_section(
            {source_identity.sha256, payload.header}));
        document.sections.push_back(
            {static_cast<std::uint32_t>(saves::CivisSaveSectionId::source_prefix), 0, payload.source_prefix});
        saves::write_civis_save(output, document);
        const auto verified = saves::read_imported_civis_save(output);
        if (verified.metadata.source_sha256 != source_identity.sha256 ||
            verified.metadata.header != payload.header || verified.payload != payload.bytes ||
            !verified.source_prefix || *verified.source_prefix != payload.source_prefix)
            throw std::runtime_error("Civis save verification changed imported payload, prefix or metadata");
        const auto converted_identity = runtime::identify_file(output);
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"save_convert\",\"can_continue\":false"
                  << ",\"source\":" << quote(assets::path_utf8(candidate.path))
                  << ",\"source_sha256\":" << quote(source_identity.sha256)
                  << ",\"output\":" << quote(assets::path_utf8(output))
                  << ",\"output_sha256\":" << quote(converted_identity.sha256)
                  << ",\"payload_bytes\":" << payload.bytes.size()
                  << ",\"source_prefix_bytes\":" << payload.source_prefix.size()
                  << ",\"setup_bytes\":" << payload.source_prefix.size() - payload.header.end_offset
                  << ",\"setup_retained\":true,\"setup_decoded\":false"
                  << ",\"metadata_mods\":" << verified.metadata.header.mods.size()
                  << ",\"metadata_dlcs\":" << verified.metadata.header.dlcs.size() << "}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"save_convert\",\"can_continue\":false"
                  << ",\"error\":" << quote(error.what()) << "}\n";
        return 2;
    }
}
