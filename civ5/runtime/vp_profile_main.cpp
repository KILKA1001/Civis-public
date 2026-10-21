#include "civ5/content/vp_profile.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::string quote(const std::string &value) {
    std::string out = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : value) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += static_cast<char>(c);
        } else if (c < 32) {
            out += "\\u00";
            out += hex[c >> 4];
            out += hex[c & 15];
        } else
            out += static_cast<char>(c);
    }
    return out + '"';
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        if (argc != 3)
            throw std::runtime_error(
                "Usage: civis_vp_profile BUNDLE_ROOT Core|Civ43CPOnly|FullNoEUI|FullEUI|Civ43NoEUI|Civ43EUI");
        const auto profile =
            content::build_vp_542_profile(argv[1], content::vp_component(assets::path_utf8(argv[2])));
        std::ostringstream out;
        out << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,\"component\":"
            << quote(content::vp_component_name(profile.component))
            << ",\"selection\":\"explicit_installer_component\",\"automatic_save_profile\":false"
            << ",\"files\":" << profile.files.size() << ",\"bytes\":" << profile.total_bytes
            << ",\"installed_mods\":[";
        bool first = true;
        for (const auto &mod : profile.installed_mods) {
            if (!first)
                out << ',';
            first = false;
            const auto actions = profile.action_plan(mod, "OnModActivated", "UpdateDatabase");
            out << "{\"id\":" << quote(mod.manifest.id) << ",\"version\":" << mod.manifest.version
                << ",\"name\":" << quote(mod.manifest.name)
                << ",\"declared_files\":" << mod.manifest.files.size()
                << ",\"omitted_eui_lua\":" << mod.omitted_eui_lua.size()
                << ",\"installer_replaced_gamecore\":" << (mod.installer_replaced_gamecore ? "true" : "false")
                << ",\"installer_relocated_gamecore\":" << (mod.installer_relocated_gamecore ? "true" : "false")
                << ",\"database_actions\":" << actions.size() << ",\"unresolved_database_actions\":[";
            bool first_missing = true;
            for (const auto &action : actions) if (!action.resolved) {
                if (!first_missing) out << ',';
                first_missing = false;
                out << quote(action.requested_path);
            }
            out << "]}";
        }
        const auto &core = profile.file("MODS/(1) Community Patch/CvGameCore_Expansion2.dll");
        out << "],\"gamecore_sha256\":" << quote(core.identity.sha256)
            << ",\"gamecore_executable\":false,\"activated\":false}\n";
        std::cout << out.str();
        return 0;
    } catch (const std::exception &error) {
        std::cout << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,\"error\":"
                  << quote(error.what()) << "}\n";
        return 2;
    }
}
