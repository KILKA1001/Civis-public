// Standalone pre-world front end. It deliberately grants no gameplay/native
// WorldView readiness; rendering and input borrow the existing engine owners.
#include "civ5/ui/frontend_menu.hpp"
#include "civ5/ui/frontend_startup.hpp"
#include "civ5/ui/frontend_process_backend.hpp"
#include "../src/ui/detail/civis_frontend_party_routing_owner.hpp"
#include "civ5/ui/frontend_sources.hpp"
#include "civ5/ui/frontend_content.hpp"
#include "civ5/ui/frontend_save_discovery.hpp"
#include "civ5/ui/frontend_existing.hpp"
#include "civ5/ui/win32_client_window.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/assets/dds.hpp"
#include "ui_screen_gl.hpp"
#include "gdi_extended_font.hpp"
#include <shellapi.h>
#include <shlobj.h>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace {
using namespace civ5;
namespace fs = std::filesystem;
void startup_trace(const char *text, DWORD length) {
#ifdef CIVIS_FRONTEND_STARTUP_DIAGNOSTIC
    const auto target = GetStdHandle(STD_ERROR_HANDLE);
    DWORD written = 0;
    if (target && target != INVALID_HANDLE_VALUE) WriteFile(target, text, length, &written, nullptr);
#else
    (void)text; (void)length;
#endif
}
struct Options {
    fs::path assets_root;
    std::optional<fs::path> party_game_root, party_payload_root, civis_settings, party_output_dir, default_save;
    std::string language = "en_US";
    std::optional<std::size_t> smoke_frames;
    std::vector<std::string> smoke_route;
    std::optional<ui::WorldViewPoint> smoke_pointer;
    std::optional<fs::path> capture, acceptance_observation;
    std::vector<fs::path> mods_roots, saves;
    int width = 1280, height = 800;
};

std::vector<std::string> command_line() {
    int count = 0;
    auto **wide = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!wide) throw std::runtime_error("Cannot read the Windows command line");
    struct Release {
        wchar_t **value;
        ~Release() { LocalFree(value); }
    } release{wide};
    std::vector<std::string> result;
    for (int i = 0; i < count; ++i) {
        const std::wstring_view value(wide[i]);
        if (value.empty()) { result.emplace_back(); continue; }
        const auto length = static_cast<int>(value.size());
        const auto bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value.data(), length, nullptr, 0, nullptr, nullptr);
        if (bytes <= 0) throw std::runtime_error("Windows command-line argument is not valid Unicode");
        std::string text(static_cast<std::size_t>(bytes), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), length,
            text.data(), bytes, nullptr, nullptr) != bytes)
            throw std::runtime_error("Windows command-line Unicode conversion failed");
        result.push_back(std::move(text));
    }
    return result;
}

Options options(int argc, char **argv) {
    Options result;
    bool language_seen = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view name(argv[i]);
        if (i + 1 == argc) throw std::runtime_error("Missing argument value: " + std::string(name));
        const std::string_view value(argv[++i]);
        if (name == "--assets-root") result.assets_root = fs::path(std::u8string(value.begin(), value.end()));
        else if (name == "--party-game-root")
            result.party_game_root = fs::path(std::u8string(value.begin(), value.end()));
        else if (name == "--party-payload-root")
            result.party_payload_root = fs::path(std::u8string(value.begin(), value.end()));
        else if (name == "--party-output-dir") {
            if (result.party_output_dir || value.empty())
                throw std::runtime_error("Party output requires one nonempty directory path");
            result.party_output_dir = fs::path(std::u8string(value.begin(), value.end()));
        }
        else if (name == "--language" || name == "--ui-language") {
            if (language_seen) throw std::runtime_error("Duplicate UI language argument");
            language_seen = true;
            result.language = value;
        }
        else if (name == "--civis-settings") {
            if (result.civis_settings || value.empty())
                throw std::runtime_error("Civis settings requires one nonempty explicit path");
            result.civis_settings = fs::path(std::u8string(value.begin(), value.end()));
        }
        else if (name == "--capture") result.capture = fs::path(std::u8string(value.begin(), value.end()));
        else if (name == "--acceptance-observation") {
            if (result.acceptance_observation || value.empty())
                throw std::runtime_error("Acceptance observation requires one nonempty JSONL path");
            result.acceptance_observation = fs::path(std::u8string(value.begin(), value.end()));
        }
        else if (name == "--mods-root") result.mods_roots.emplace_back(std::u8string(value.begin(), value.end()));
        else if (name == "--save") result.saves.emplace_back(std::u8string(value.begin(), value.end()));
        else if (name == "--default-save") {
            if (result.default_save || value.empty())
                throw std::runtime_error("Default save requires one nonempty optional path");
            result.default_save = fs::path(std::u8string(value.begin(), value.end()));
        }
        else if (name == "--smoke-route") {
            if (value.empty()) throw std::runtime_error("Smoke route must contain navigation item IDs");
            std::size_t begin = 0;
            while (begin <= value.size()) {
                const auto comma = value.find(',', begin);
                const auto item = value.substr(begin, comma == value.npos ? value.size() - begin : comma - begin);
                if (item.empty() || !std::all_of(item.begin(), item.end(), [](unsigned char ch) {
                    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
                })) throw std::runtime_error("Invalid smoke route item");
                result.smoke_route.emplace_back(item);
                if (comma == value.npos) break;
                begin = comma + 1;
            }
        }
        else if (name == "--smoke-pointer") {
            const auto comma = value.find(',');
            if (comma == value.npos) throw std::runtime_error("Smoke pointer requires X,Y");
            int x = 0, y = 0;
            const auto px = std::from_chars(value.data(), value.data() + comma, x);
            const auto py = std::from_chars(value.data() + comma + 1, value.data() + value.size(), y);
            if (px.ec != std::errc{} || py.ec != std::errc{} || px.ptr != value.data() + comma ||
                py.ptr != value.data() + value.size() || x < 0 || y < 0)
                throw std::runtime_error("Smoke pointer requires nonnegative integer X,Y");
            result.smoke_pointer = ui::WorldViewPoint{x, y};
        }
        else if (name == "--width" || name == "--height") {
            int size = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), size);
            const int minimum = name == "--width" ? 320 : 240;
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || size < minimum || size > 16384)
                throw std::runtime_error("Invalid window dimension: " + std::string(name));
            (name == "--width" ? result.width : result.height) = size;
        }
        else if (name == "--smoke-frames") {
            std::size_t count = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), count);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || count == 0)
                throw std::runtime_error("Smoke frame count must be a positive integer");
            result.smoke_frames = count;
        } else throw std::runtime_error("Unknown argument: " + std::string(name));
    }
    if (result.assets_root.empty() || !fs::is_directory(result.assets_root / "Assets"))
        throw std::runtime_error("--assets-root must name an existing Civ V asset root");
    if (result.party_game_root.has_value() != result.party_payload_root.has_value())
        throw std::runtime_error("--party-game-root and --party-payload-root must be provided together");
    if (result.party_game_root && (result.party_game_root->empty() || result.party_payload_root->empty()))
        throw std::runtime_error("Party source roots must be nonempty explicit paths");
    if (result.party_output_dir && !result.party_game_root)
        throw std::runtime_error("--party-output-dir requires the paired party source roots");
    // The existing product language owner reads the same physical argv.
    // Require an explicit shared selection when opening a party or importing
    // settings; never manufacture a language source from the menu default.
    if ((result.party_game_root || result.civis_settings) && !language_seen)
        throw std::runtime_error("Party/settings require an explicit --ui-language or --language selection");
    if (result.language != "en_US" && result.language != "ru_RU")
        throw std::runtime_error("--language must be en_US or ru_RU");
    if (result.capture && result.capture->extension() != ".ppm")
        throw std::runtime_error("--capture requires a .ppm path");
    if (result.capture && fs::exists(*result.capture))
        throw std::runtime_error("Capture output already exists");
    if ((!result.smoke_route.empty() || result.smoke_pointer) && !result.smoke_frames)
        throw std::runtime_error("Smoke route/pointer require --smoke-frames");
    if (result.smoke_pointer && (result.smoke_pointer->x >= result.width || result.smoke_pointer->y >= result.height))
        throw std::runtime_error("Smoke pointer must lie inside the requested viewport");
    if (result.acceptance_observation) {
        if (result.acceptance_observation->extension() != ".jsonl")
            throw std::runtime_error("Acceptance observation requires a .jsonl path");
        if (!result.smoke_route.empty() || result.smoke_pointer)
            throw std::runtime_error("Acceptance observation requires physical frontend input");
    }
    return result;
}

// Ordinary artifact root only. The actual party load creates a fresh child
// occurrence before writing; existing files in the root are preserved.
// Path selection grants no source, window, renderer or gameplay readiness.
fs::path party_output_directory(const Options &configuration) {
    if (configuration.party_output_dir)
        return fs::absolute(*configuration.party_output_dir).lexically_normal();
    const auto parent = configuration.capture
        ? fs::absolute(*configuration.capture).parent_path()
        : fs::current_path() / "build" / "civis-client-runs";
    if (fs::exists(parent) && !fs::is_directory(parent))
        throw std::runtime_error("Party output parent is not a directory");
    return parent.lexically_normal();
}

// Minimum UI preparation shares its actual FPK controls and base UI index
// with the final frontend mounts. Metadata fallback is not mounted there.
struct FrontendBootResources {
    std::shared_ptr<assets::LooseResourceIndex> base, metadata;
    std::vector<std::shared_ptr<assets::FpkSource>> archives;
    assets::ResourceMountTable table;
    FrontendBootResources(const fs::path &root, std::string_view language,
                         const runtime::StartupStageProgress &progress)
        : base(std::make_shared<assets::LooseResourceIndex>(root)),
          metadata(std::make_shared<assets::LooseResourceIndex>(root)),
          table([ui = base, locale = metadata](const assets::AssetKey &key) {
              auto found = ui->find(key);
              if (std::holds_alternative<std::monostate>(found)) found = locale->find(key);
              return assets::ResourceMountHit{std::move(found), true};
          }) {
        base->scan({root / "Assets/UI", {}, true}); progress.report(1, 7); progress.checkpoint();
        metadata->add_file(root / "Assets/Gameplay/XML/NewText" /
            (language == "ru_RU" ? "Russian.xml" : "English.xml")); progress.report(2, 7); progress.checkpoint();
        const assets::AssetEnvironment environment(root);
        for (const auto *name : {"Resource/DX9/UITextures.fpk",
                                "Resource/DX9/Expansion1UITextures.fpk",
                                "Resource/DX9/Expansion2UITextures.fpk"}) {
            const auto opened = environment.open_fpk_archive(name);
            const auto *archive = std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
            if (!archive) throw std::runtime_error("Cannot open UI archive: " + std::string(name));
            archives.push_back(*archive);
            table.mount({static_cast<std::uint64_t>(archives.size()), 0, 0,
                assets::resource_lookup(*archive), true, true,
                assets::resource_lookup_all(*archive), assets::resource_keys(*archive)});
            progress.report(2 + archives.size(), 7); progress.checkpoint();
        }
    }
};
struct FrontendStartupCancelled {};

assets::ResourceMountTable mounted_resources(const fs::path &root,
    const FrontendBootResources &bootstrap, const runtime::StartupStageProgress &progress) {
    std::uint64_t completed = 0;
    const auto layer = [&](const fs::path &base_path) {
        const bool already_indexed_ui = base_path == root / "Assets";
        auto index = already_indexed_ui ? bootstrap.base : std::make_shared<assets::LooseResourceIndex>(root);
        for (const auto *name : {"UI", "Gameplay", "Shared"}) {
            if (already_indexed_ui && std::string_view(name) == "UI") continue;
            const auto directory = base_path / name;
            if (fs::is_directory(directory)) index->scan({directory, {}, true});
            progress.report(++completed, 14); progress.checkpoint();
        }
        return index;
    };
    auto base = layer(root / "Assets");
    assets::ResourceMountTable result(assets::resource_lookup(base),
        assets::resource_lookup_all(base), assets::resource_keys(base));
    for (const auto *expansion : {"Expansion", "Expansion2", "Shared"}) {
        auto source = layer(root / "Assets/DLC" / expansion);
        result.mount({static_cast<std::uint64_t>(result.mounts().size() + 1), 0, 0,
            assets::resource_lookup(source), true, true,
            assets::resource_lookup_all(source), assets::resource_keys(source)});
    }
    for (const auto &archive : bootstrap.archives) {
        result.mount({static_cast<std::uint64_t>(result.mounts().size() + 1), 0, 0,
            assets::resource_lookup(archive), true, true,
            assets::resource_lookup_all(archive), assets::resource_keys(archive)});
        progress.report(++completed, 14); progress.checkpoint();
    }
    return result;
}

assets::DdsData selected_dds(const assets::ResourceMountTable &resources, std::string_view key) {
    const auto selected = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle) throw std::runtime_error("Missing or ambiguous UI atlas: " + std::string(key));
    return assets::DdsData::parse(handle->read_bytes());
}

std::string extended_face(const assets::ResourceMountTable &resources, std::string_view language) {
    const auto key = std::string("Assets/Gameplay/XML/NewText/") +
        (language == "ru_RU" ? "Russian.xml" : "English.xml");
    const auto selected = resources.resolve(assets::AssetKey(key));
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle) throw std::runtime_error("Selected language metadata is unavailable");
    const auto bytes = handle->read_bytes();
    const auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
    const auto *languages = document.child("Languages");
    if (!languages || languages->children.size() != 1)
        throw std::runtime_error("Selected language metadata has no unique language row");
    const auto &row = languages->children.front();
    const auto *type = row.child("Type"), *table = row.child("TableName");
    if (!type || !table || type->text != language || table->text !=
        (language == "ru_RU" ? "Language_RU_RU" : "Language_en_US"))
        throw std::runtime_error("Selected language metadata differs from requested language");
    const auto *extended = row.child("UseExtendedFont"), *face = row.child("FontName");
    if (extended && extended->text == "1") {
        if (!face || face->text.empty()) throw std::runtime_error("Selected extended font face is absent");
        return face->text;
    }
    if (language == "ru_RU") throw std::runtime_error("Russian UI requires authored extended font metadata");
    return {};
}

struct BoundFont {
    ui::GlyphGenGeometry geometry;
    graphics::GlTexture texture;
};

BoundFont font(const graphics::GlContext &context, const assets::ResourceMountTable &resources,
               int size, const ui::TopPanelLocalizationCatalog &texts, std::string_view face) {
    const auto name = "TwCenMT" + std::to_string(size);
    const auto source = ui::read_glyphgen(resources, "Assets/UI/Fonts/Tw Cent MT/" + name + ".ggxml");
    const auto texture = std::find_if(source.textures.begin(), source.textures.end(),
        [&](const auto &item) { return item.name() == name + "Base"; });
    if (texture == source.textures.end()) throw std::runtime_error("Selected GlyphGen Base layer is absent");
    const auto key = texture->source();
    auto prepared = prepare_preview_alpha_font(source,
        selected_dds(resources, key.substr(0, key.size() - 4) + "-index.dds"),
        selected_dds(resources, key), texts, face, 2048);
    return {std::move(prepared.geometry), graphics::GlTexture::alpha8(context,
        prepared.atlas.width, prepared.atlas.height, prepared.atlas.alpha)};
}

BoundFont icon_font(const graphics::GlContext &context, const assets::ResourceMountTable &resources) {
    const auto source = ui::read_glyphgen(resources,
        "Assets/DLC/Expansion2/UI/Fonts/Font Icons/FontIcons.ggxml");
    const auto texture = std::find_if(source.textures.begin(), source.textures.end(),
        [](const auto &item) { return item.source() == "FontIcons.dds"; });
    if (texture == source.textures.end()) throw std::runtime_error("Selected FontIcons layer is absent");
    const auto key = texture->source();
    const auto atlas = ui::reconstruct_glyphgen_rgba(source,
        selected_dds(resources, key.substr(0, key.size() - 4) + "-index.dds"), selected_dds(resources, key));
    const assets::Rgba8Level level{atlas.width, atlas.height, atlas.rgba};
    return {ui::glyphgen_geometry(source), graphics::GlTexture::rgba8(context,
        std::span<const assets::Rgba8Level>(&level, 1))};
}

std::map<std::string, std::uint32_t, std::less<>> icon_mapping(const assets::ResourceMountTable &resources) {
    const auto selected = resources.resolve(assets::AssetKey("CIV5IconFontMapping.xml"));
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle) throw std::runtime_error("Selected icon mapping is unavailable");
    const auto bytes = handle->read_bytes();
    const auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
    const auto *table = document.child("IconFontMapping");
    if (!table) throw std::runtime_error("Selected icon mapping has no authored table");
    std::map<std::string, std::uint32_t, std::less<>> result;
    for (const auto &row : table->children) {
        const auto *name = row.child("IconName"), *number = row.child("IconMapping");
        if (!name || !number) throw std::runtime_error("Selected icon mapping row is incomplete");
        std::uint32_t code = 0;
        const auto parsed = std::from_chars(number->text.data(), number->text.data() + number->text.size(), code);
        if (parsed.ec != std::errc{} || parsed.ptr != number->text.data() + number->text.size())
            throw std::runtime_error("Selected icon mapping code is invalid");
        if (!result.emplace(name->text, code).second)
            throw std::runtime_error("Selected icon mapping has a duplicated icon");
    }
    return result;
}

// Root's nonce names a run only; WGL handles and driver reads provide context facts.
struct MenuCaptureObservation {
    std::string run_id, context_id, observed_utc, captured_utc;
    std::string vendor, renderer, version;
    std::uintptr_t native_context = 0, native_dc = 0;
    int width = 0, height = 0;
};

std::string capture_utc_now() {
    FILETIME file_time{};
    GetSystemTimeAsFileTime(&file_time);
    SYSTEMTIME utc{};
    if (!FileTimeToSystemTime(&file_time, &utc))
        throw std::runtime_error("Cannot obtain frontend capture UTC");
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << utc.wYear << '-'
        << std::setw(2) << utc.wMonth << '-' << std::setw(2) << utc.wDay << 'T'
        << std::setw(2) << utc.wHour << ':' << std::setw(2) << utc.wMinute << ':'
        << std::setw(2) << utc.wSecond << '.' << std::setw(3) << utc.wMilliseconds << 'Z';
    return out.str();
}

std::string capture_json_string(std::string_view value) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    for (const char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (ch == '"' || ch == '\\') { out.push_back('\\'); out.push_back(ch); }
        else if (byte < 0x20) {
            out += "\\u00";
            out.push_back(hex[byte >> 4]); out.push_back(hex[byte & 0x0f]);
        } else out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

const char *frontend_command_status(ui::UiCommandStatus value) {
    switch (value) {
    case ui::UiCommandStatus::ready: return "ready";
    case ui::UiCommandStatus::navigation: return "navigation";
    case ui::UiCommandStatus::performed: return "performed";
    case ui::UiCommandStatus::unavailable: return "unavailable";
    case ui::UiCommandStatus::invalid: return "invalid";
    case ui::UiCommandStatus::pending_refresh: return "pending_refresh";
    case ui::UiCommandStatus::uncertain: return "uncertain";
    }
    return "unrecognized"; // The actual numeric value is also recorded.
}

// Acceptance-only observation of the existing physical route. These records
// do not dispatch commands or grant source, input, renderer or party readiness.
class FrontendAcceptanceObservation final {
    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::uint64_t records_ = 0;
    static constexpr std::uint64_t record_limit = 128;
    static void window_snapshot(std::ostream &out, HWND window, const ui::ClientViewport &viewport) {
        DWORD process = 0;
        const auto thread = GetWindowThreadProcessId(window, &process);
        POINT origin{};
        RECT client{};
        const auto origin_ok = ClientToScreen(window, &origin);
        const auto client_ok = GetClientRect(window, &client);
        out << ",\"hwnd\":" << reinterpret_cast<std::uintptr_t>(window)
            << ",\"pid\":" << process << ",\"window_thread\":" << thread
            << ",\"foreground_hwnd\":" << reinterpret_cast<std::uintptr_t>(GetForegroundWindow())
            << ",\"client_origin_observed\":" << (origin_ok ? "true" : "false")
            << ",\"client_origin\":[" << origin.x << ',' << origin.y << ']'
            << ",\"client_rect_observed\":" << (client_ok ? "true" : "false")
            << ",\"client_rect\":[" << client.left << ',' << client.top << ',' << client.right << ',' << client.bottom << ']'
            << ",\"viewport\":{\"client_width\":" << viewport.client_width
            << ",\"client_height\":" << viewport.client_height
            << ",\"framebuffer_width\":" << viewport.framebuffer_width
            << ",\"framebuffer_height\":" << viewport.framebuffer_height
            << ",\"dpi\":" << viewport.dpi << '}';
    }
    void append(std::ostringstream &out) {
        if (records_ == record_limit)
            throw std::runtime_error("Acceptance observation exceeded its 128-record limit");
        out << ",\"observation_sequence\":" << ++records_ << ",\"record_limit\":" << record_limit << "}\n";
        const auto text = out.str();
        if (text.size() > std::numeric_limits<DWORD>::max())
            throw std::runtime_error("Acceptance observation record is too large");
        DWORD written = 0;
        if (!WriteFile(file_, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) ||
            written != text.size() || !FlushFileBuffers(file_))
            throw std::runtime_error("Cannot write actual frontend acceptance observation");
    }
    static std::ostringstream begin(const char *kind) {
        std::ostringstream out;
        out << "{\"schema\":\"civis-frontend-physical-acceptance-observation-v1\",\"kind\":"
            << capture_json_string(kind) << ",\"observed_utc\":" << capture_json_string(capture_utc_now());
        return out;
    }
  public:
    explicit FrontendAcceptanceObservation(const fs::path &path) {
        file_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Acceptance observation requires an exclusive fresh output file");
    }
    ~FrontendAcceptanceObservation() { if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_); }
    FrontendAcceptanceObservation(const FrontendAcceptanceObservation &) = delete;
    FrontendAcceptanceObservation &operator=(const FrontendAcceptanceObservation &) = delete;
    void presented(HWND window, const ui::ClientViewport &viewport, std::size_t frame,
                   const ui::FrontendMenuPlan &plan, const ui::FrontendMenuSession &session,
                   const std::vector<std::string> &paint_unresolved) {
        auto out = begin("menu_presented");
        window_snapshot(out, window, viewport);
        out << ",\"presented_frame\":" << frame << ",\"revision\":" << plan.revision
            << ",\"first_row\":" << plan.first_row << ",\"total_rows\":" << plan.total_rows
            << ",\"visible_rows\":" << plan.visible_rows
            << ",\"screen\":" << capture_json_string(plan.screen) << ",\"selected_save_id\":";
        if (session.draft().save) out << capture_json_string(*session.draft().save); else out << "null";
        out << ",\"message\":" << capture_json_string(session.message()) << ",\"selected_packages\":[";
        bool first = true;
        for (const auto &key : session.draft().packages) {
            if (!first) out << ',';
            first = false;
            out << capture_json_string(key);
        }
        out << "],\"packages\":[";
        first = true;
        for (const auto &package : session.data().packages) {
            if (!first) out << ',';
            first = false;
            out << "{\"key\":" << capture_json_string(package.key)
                << ",\"mod\":" << (package.mod ? "true" : "false")
                << ",\"version\":" << package.version
                << ",\"availability\":{\"status\":" << capture_json_string(frontend_command_status(package.availability.status))
                << ",\"status_value\":" << static_cast<int>(package.availability.status)
                << ",\"reason\":" << capture_json_string(package.availability.reason) << "}}";
        }
        out << "],\"hits\":[";
        first = true;
        for (const auto &hit : plan.hits) {
            if (!first) out << ',';
            first = false;
            out << "{\"id\":" << capture_json_string(hit.id)
                << ",\"bounds\":{\"x\":" << hit.bounds.x << ",\"y\":" << hit.bounds.y
                << ",\"width\":" << hit.bounds.width << ",\"height\":" << hit.bounds.height << '}'
                << ",\"enabled\":" << (hit.enabled ? "true" : "false")
                << ",\"unavailable_reason\":" << capture_json_string(hit.unavailable_reason);
            if (const auto *item = session.registry().item(hit.id))
                out << ",\"registry_id\":" << capture_json_string(item->id)
                    << ",\"source_key\":" << capture_json_string(item->source_key)
                    << ",\"control_id\":" << capture_json_string(item->control_id)
                    << ",\"target_screen\":" << capture_json_string(item->target_screen)
                    << ",\"operation\":" << capture_json_string(item->operation);
            for (const auto &save : session.data().saves) if (hit.id == "save:" + save.id) {
                out << ",\"save_id\":" << capture_json_string(save.id)
                    << ",\"save_path\":" << capture_json_string(assets::path_utf8(save.path))
                    << ",\"save_availability_status\":" << capture_json_string(frontend_command_status(save.availability.status))
                    << ",\"save_availability_reason\":" << capture_json_string(save.availability.reason);
                break;
            }
            out << '}';
        }
        out << "],\"layout_unresolved\":[";
        first = true;
        for (const auto &reason : plan.unresolved) { if (!first) out << ','; first = false; out << capture_json_string(reason); }
        out << "],\"paint_unresolved\":[";
        first = true;
        for (const auto &reason : paint_unresolved) { if (!first) out << ','; first = false; out << capture_json_string(reason); }
        out << ']';
        append(out);
    }
    void click(HWND window, const ui::ClientViewport &viewport, const ui::FrontendMenuPlan &plan,
               ui::ClientPoint press, ui::ClientPoint release, const ui::FrontendMenuSession &session,
               const ui::UiCommandResult *returned) {
        auto out = begin(returned ? "click_returned" : "click_begin");
        window_snapshot(out, window, viewport);
        const auto *press_hit = plan.hit(press.x, press.y), *release_hit = plan.hit(release.x, release.y);
        out << ",\"revision\":" << plan.revision << ",\"screen\":" << capture_json_string(plan.screen)
            << ",\"press\":[" << press.x << ',' << press.y << "],\"release\":[" << release.x << ',' << release.y << ']'
            << ",\"press_hit_id\":" << (press_hit ? capture_json_string(press_hit->id) : "null")
            << ",\"release_hit_id\":" << (release_hit ? capture_json_string(release_hit->id) : "null")
            << ",\"session_revision\":" << session.revision()
            << ",\"session_screen\":" << capture_json_string(session.screen())
            << ",\"message\":" << capture_json_string(session.message());
        if (returned)
            out << ",\"status\":" << capture_json_string(frontend_command_status(returned->status))
                << ",\"status_value\":" << static_cast<int>(returned->status)
                << ",\"reason\":" << capture_json_string(returned->reason);
        append(out);
    }
};

void require_capture_current(const MenuCaptureObservation &value) {
    if (reinterpret_cast<std::uintptr_t>(wglGetCurrentContext()) != value.native_context ||
        reinterpret_cast<std::uintptr_t>(wglGetCurrentDC()) != value.native_dc)
        throw std::runtime_error("Frontend capture CURRENT WGL identity changed");
}

std::optional<MenuCaptureObservation> observe_menu_capture(const graphics::GlContext &context,
                                                          int width, int height) {
    const auto *nonce = std::getenv("CIVIS_MENU_PREVIEW_RUN_ID");
    if (!nonce) return std::nullopt;
    const std::string run_id(nonce);
    if (run_id.empty() || run_id.size() > 128 ||
        !std::all_of(run_id.begin(), run_id.end(), [](char ch) {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        })) throw std::runtime_error("Invalid Root menu preview run ID");
    const auto &api = context.api(); // Existing owner establishes CURRENT first.
    const auto owned = context.native_identity(); // This alone is not CURRENT proof.
    const auto current = reinterpret_cast<std::uintptr_t>(wglGetCurrentContext());
    const auto dc = reinterpret_cast<std::uintptr_t>(wglGetCurrentDC());
    if (!owned.owner_thread || !current || !dc || current != owned.context || dc != owned.dc)
        throw std::runtime_error("Frontend capture is not SAME owned CURRENT WGL context");
    const auto *vendor = api.GetString(GL_VENDOR);
    const auto *renderer = api.GetString(GL_RENDERER);
    const auto *version = api.GetString(GL_VERSION);
    if (!vendor || !renderer || !version)
        throw std::runtime_error("Frontend CURRENT GL driver strings are unavailable");
    std::ostringstream identity;
    identity << "wgl-hglrc:0x" << std::hex << current << ";dc:0x" << dc;
    MenuCaptureObservation value{run_id, identity.str(), capture_utc_now(), {},
        reinterpret_cast<const char *>(vendor), reinterpret_cast<const char *>(renderer),
        reinterpret_cast<const char *>(version), current, dc, width, height};
    require_capture_current(value);
    return value;
}

void publish_menu_capture(const MenuCaptureObservation &value) {
    std::cout << "MENU_PREVIEW_CAPTURE_OBSERVATION={\"schema\":\"civis-inprocess-menu-capture-observation-v1\","
        << "\"run_id\":" << capture_json_string(value.run_id)
        << ",\"context_id\":" << capture_json_string(value.context_id)
        << ",\"observed_utc\":" << capture_json_string(value.observed_utc)
        << ",\"captured_utc\":" << capture_json_string(value.captured_utc)
        << ",\"current_context_observed\":true,\"owned_identity_matched\":true,"
        << "\"same_current_after_readback\":true,\"vendor\":" << capture_json_string(value.vendor)
        << ",\"renderer\":" << capture_json_string(value.renderer)
        << ",\"version\":" << capture_json_string(value.version)
        << ",\"width\":" << value.width << ",\"height\":" << value.height << "}\n" << std::flush;
}

void capture(const graphics::GlContext &context, const fs::path &path, int width, int height) {
    const auto read_pixels = reinterpret_cast<PFNGLREADPIXELSPROC>(context.proc_address("glReadPixels"));
    const auto read_buffer = reinterpret_cast<PFNGLREADBUFFERPROC>(context.proc_address("glReadBuffer"));
    if (!read_pixels || !read_buffer) throw std::runtime_error("OpenGL readback entry point is unavailable");
    context.api().PixelStorei(GL_PACK_ALIGNMENT, 1);
    read_buffer(GL_BACK);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 3);
    auto observation = observe_menu_capture(context, width, height);
    read_pixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    if (observation) {
        observation->captured_utc = capture_utc_now(); // Immediately AFTER actual readback.
        require_capture_current(*observation);
    }
    context.check("Read standalone frontend frame");
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        out.write(reinterpret_cast<const char *>(pixels.data() +
            static_cast<std::size_t>(y) * width * 3), static_cast<std::streamsize>(width * 3));
    out.close();
    if (!out) throw std::runtime_error("Cannot write frontend capture");
    if (observation) publish_menu_capture(*observation);
}

using FrontendProcessBackend = ui::FrontendProcessBackend;

std::optional<std::string> russian_status(std::string_view text) {
    static const std::map<std::string, std::string, std::less<>> exact{
        {"Select content for saved game", "Выбрать моды и DLC для сохранения"},
        {"Selected saved game", "Выбранное сохранение"},
        {"Select a button with Tab first", "Сначала выберите кнопку клавишей Tab"},
        {"No available button on this screen", "На этом экране нет доступных кнопок"},
        {"Selected packages required by this saved game", "Выбраны моды и DLC, необходимые для этого сохранения"},
        {"Selected source", "Выбранный источник"},
        {"Replaced source", "Источник заменён"},
        {"Unavailable", "Недоступно"},
        {"Saved-content selection owner unavailable", "Подбор содержимого для сохранения пока недоступен"},
        {"Selected save is no longer installed", "Выбранное сохранение больше не найдено"},
        {"Saved-game selection request unavailable", "Не удалось подготовить выбор содержимого сохранения"},
        {"Saved content requirements are not known", "Не удалось определить требования сохранения"},
        {"Saved content has an unknown or repeated source", "Необходимый пакет отсутствует или указан несколько раз"},
        {"Saved content source is ambiguous; choose it explicitly", "Найдено несколько вариантов пакета. Выберите нужный вручную"},
        {"Saved content requirements changed during selection", "Требования сохранения изменились во время выбора"},
        {"Open the saved-game menu", "Откройте список сохранений"},
        {"Multiple selected sources/versions for one package", "Выбрано несколько вариантов одного пакета. Оставьте один"},
        {"This action has no retained game owner", "Это действие ещё не подключено"},
        {"Loading cancelled; reopen the client to retry", "Загрузка отменена. Для повторной попытки откройте клиент заново"},
        {"Window changed while loading; reopen the client to retry", "Размер окна изменился во время загрузки. Откройте клиент заново"},
        {"The party window and decoded-scene executor is not connected", "Переход к партии ещё не подключён: нет владельца окна и загруженной сцены"},
        {"The new-game generator and party executor is not connected", "Генератор новой партии ещё не подключён"},
        {"The settings apply and Civis profile persistence owner is not connected", "Применение и сохранение настроек Civis ещё не подключено"},
        {"The window settings executor is not connected", "Применение параметров окна ещё не подключено"},
        {"Network and Steam services are not connected", "Сетевые функции и Steam ещё не подключены"},
        {"The previous application action has an uncertain result; reopen the client", "Результат предыдущего действия не подтверждён. Откройте клиент заново"},
        {"Content admission has no verified save requirements", "Требования сохранения не прошли полную проверку"},
        {"This operation has no connected pre-world application owner", "Эта функция ещё не реализована"},
        {"This feature is not implemented yet", "Эта функция ещё не реализована"},
        {"Starting a game is not connected yet", "Создание партии ещё не подключено"},
        {"Loading a game is not connected yet", "Загрузка партии ещё не подключена"},
        {"Starting a scenario is not connected yet", "Запуск сценариев ещё не подключён"},
        {"Starting a tutorial is not connected yet", "Запуск обучения ещё не подключён"},
        {"Network play is not connected yet", "Сетевая игра ещё не подключена"},
        {"Saving settings is not connected yet", "Сохранение настроек ещё не подключено"},
        {"Changing game setup is not connected yet", "Изменение настроек партии ещё не подключено"},
        {"Existing data/presenter owner unavailable", "Этот экран ещё не подключён"},
        {"Select a saved game", "Выберите сохранение"},
        {"Selected content differs from save requirements", "Выбранные моды и DLC не совпадают с требованиями сохранения"},
        {"Saved game requires a different content selection. Select its mods and DLC before loading.", "Перед загрузкой выберите моды и DLC, указанные в сохранении"},
        {"Native mod DLL requires a Civis port", "Для этого мода требуется перенос DLL в Civis"},
        {"Selected package files missing or invalid", "Файлы выбранного пакета отсутствуют или повреждены"},
        {"Selected content cannot activate", "Выбранный контент нельзя активировать"},
        {"Save changed; refresh the save list", "Сохранение изменилось. Обновите список сохранений"},
        {"Save changed during payload validation", "Сохранение изменилось во время проверки"},
        {"Save payload is corrupt or incomplete", "Сохранение повреждено или записано не полностью"},
        {"Save payload framing is corrupt or incomplete", "Сохранение повреждено или записано не полностью"},
        {"Package manifest changed; refresh the content catalog", "Описание пакета изменилось. Обновите каталог контента"},
        {"Toggle value unavailable", "Текущее значение настройки недоступно"},
        {"Value requires the existing editor adapter", "Редактор этого значения ещё не подключён"},
        {"Civilopedia search query is empty", "Введите название статьи"},
        {"Civilopedia search has multiple authored matches", "Найдено несколько статей. Уточните название"},
        {"Civilopedia search has no authored match", "Статьи с таким названием не найдены"},
        {"Civilopedia search exceeds authored 50 characters", "В поисковом запросе допускается до 50 символов"},
        {"No previous Civilopedia page", "Это первая страница"},
        {"No next Civilopedia page", "Это последняя страница"},
        {"No adjacent Civilopedia history", "В истории нет соседней статьи"},
        {"No selected Civilopedia article", "Выберите статью"},
        {"Already at the start of the article", "Это начало статьи"},
        {"Already at the end of the article", "Это конец статьи"},
        {"Selected Civilopedia category has no supported article data", "Статьи этой категории пока недоступны"},
        {"Civilopedia category is already selected", "Эта категория уже открыта"},
        {"Enlarge the window to view the Civilopedia", "Увеличьте окно\nдля просмотра Цивилопедии"},
    };
    if (const auto found = exact.find(text); found != exact.end()) return found->second;
    constexpr std::string_view uncertain_prefix = "Application executor result is uncertain: ";
    if (text.starts_with(uncertain_prefix)) {
        const auto detail = text.substr(uncertain_prefix.size());
        const auto translated = russian_status(detail);
        return "Не удалось завершить запуск партии: " + translated.value_or(std::string(detail));
    }
    static const std::vector<std::pair<std::string, std::string>> prefixes{
        {"Selected content is no longer installed: ", "Выбранный пакет больше не установлен: "},
        {"Required package missing: ", "Не установлен необходимый пакет: "},
        {"Required package version mismatch: ", "Не совпадает версия необходимого пакета: "},
        {"Required package source ambiguous: ", "Для пакета найдено несколько источников: "},
        {"Package manifest unavailable: ", "Описание пакета недоступно: "},
        {"Save metadata unavailable: ", "Не удалось прочитать сохранение: "},
        {"Select a game setting: ", "Выберите значение настройки: "},
        {"A selected game setting is no longer valid: ", "Выбранное значение настройки больше недоступно: "},
    };
    for (const auto &[prefix, replacement] : prefixes)
        if (text.starts_with(prefix)) return replacement + std::string(text.substr(prefix.size()));
    return std::nullopt;
}

std::optional<std::string> unavailable_presentation(std::string_view reason, bool russian) {
    for (const auto prefix : {"Menu source unavailable: ", "Target menu source unavailable: ",
        "UI context XML is missing or ambiguous: ", "Ambiguous BNW UI context XML: ",
        "Selected Civilopedia source unavailable: "})
        if (reason.starts_with(prefix))
            return russian ? "Не удалось прочитать необходимые файлы экрана" : "Required screen files could not be read";
    for (const auto prefix : {"Authored control unavailable: ", "Frontend operation owner unavailable: ",
        "Existing screen/action adapter unavailable: "})
        if (reason.starts_with(prefix))
            return russian ? "Эта функция ещё не реализована" : "This feature is not implemented yet";
    if (reason.starts_with("Existing screen presenter unavailable: "))
        return russian ? "Этот экран ещё не реализован" : "This screen is not implemented yet";
    if (reason.starts_with("Existing screen presenter failed: "))
        return russian ? "Не удалось открыть экран" : "The screen could not be opened";
    for (const auto exact : {"Existing data/presenter owner unavailable", "Existing screen owner unavailable",
        "Existing screen/action adapter unavailable", "Content owner unavailable", "Save owner unavailable", "Value owner unavailable",
        "Civilopedia owner unavailable", "Selected Civilopedia source unavailable",
        "Civilopedia article data is unavailable", "Civilopedia scroll layout is unavailable",
        "Civilopedia scroll position is stale", "Civilopedia history article is unavailable",
        "Civilopedia article is unavailable", "Civilopedia link is unavailable"})
        if (reason == exact)
            return russian ? "Данные этого экрана пока недоступны" : "This screen's data is currently unavailable";
    return std::nullopt;
}

ui::TopPanelLocalizationCatalog menu_font_texts(const ui::FrontendRegistry &registry,
    const ui::FrontendSourceCatalog &sources, const ui::TopPanelLocalizationCatalog &texts) {
    ui::TopPanelLocalizationCatalog used;
    const auto add = [&](const std::string &key) {
        const auto found = texts.rows.find(key);
        if (found != texts.rows.end()) used.rows.insert_or_assign(key, found->second);
    };
    for (const auto &screen : registry.screens()) add(screen.title_key);
    for (const auto &item : registry.items()) add(item.text_key);
    for (const auto &source : sources.screens())
        for (const auto &control : source.controls) add(control.text_key);
    for (const auto *key : {"TXT_KEY_BACK_BUTTON", "TXT_KEY_CANCEL_BUTTON", "TXT_KEY_MODDING_MENU_BACK",
                            "TXT_KEY_YES_BUTTON", "TXT_KEY_NO_BUTTON", "TXT_KEY_MENU_EXIT_WARN",
                            "TXT_KEY_MAIN_MENU", "frontend.caption.volume_music",
                            "frontend.caption.volume_effects", "frontend.caption.volume_speech",
                            "frontend.caption.volume_ambience", "frontend.caption.city_states",
                            "frontend.caption.server_list_type", "frontend.caption.map_options"}) add(key);
    // Dynamic choices can include any Cyrillic character from the same selected
    // language. Keep this in the runtime extension, never in the authored atlas.
    used.rows["frontend.runtime.cyrillic"] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";
    return used;
}
} // namespace

int main() {
    startup_trace("Frontend: entry\n", 16);
    try {
        auto arguments = command_line();
        startup_trace("Frontend: command line read\n", 28);
        std::vector<char *> pointers;
        for (auto &argument : arguments) pointers.push_back(argument.data());
        const auto configuration = options(static_cast<int>(pointers.size()), pointers.data());
        startup_trace("Frontend: arguments validated\n", 30);
        std::unique_ptr<FrontendAcceptanceObservation> acceptance;
        if (configuration.acceptance_observation)
            acceptance = std::make_unique<FrontendAcceptanceObservation>(*configuration.acceptance_observation);
        // Every sink capture and staged owner outlives the one retirement guard.
        // It runs backend -> routing -> binding before destroying UI/resources.
        std::shared_ptr<graphics::GlContext> context_owner;
        std::shared_ptr<ui::detail::CivisFrontendPartyRoutingOwner> routing;
        std::unique_ptr<ui::Win32ClientWindowBinding> binding_owner;
        std::unique_ptr<FrontendProcessBackend> backend_storage;
        std::unique_ptr<FrontendBootResources> boot_resources;
        std::unique_ptr<assets::ResourceMountTable> resources_storage;
        std::unique_ptr<ui::FrontendCatalogContentAdapter> content_storage;
        std::unique_ptr<ui::FrontendMenuSession> session_storage;
        std::unique_ptr<ui::FrontendExistingScreens> existing_storage;
        std::unique_ptr<BoundFont> boot_font, small_storage, regular_storage, title_storage, icons_storage;
        std::unique_ptr<ui::UiScreenGlRenderer> boot_painter, painter_storage;
        std::unique_ptr<ui::FrontendStartupSession> startup;
        ui::TopPanelLocalizationCatalog texts, used_texts;
        ui::FrontendRegistry registry;
        ui::FrontendSourceCatalog sources;
        std::shared_ptr<content::Catalog> catalog;
        std::string face;
        std::map<std::string, std::uint32_t, std::less<>> mapping;
        std::set<std::string> reported_status, startup_reported;
        const auto startup_captions = ui::frontend_startup_captions(configuration.language == "ru_RU");
        bool menu_retiring = false, startup_quit = false;
        std::function<void(const ui::ClientWindowEvent &)> active_sink;
        std::function<void(const runtime::StartupStageProgress &)> active_startup_step;
        std::function<void()> pump_startup, paint_startup;
        ui::FrontendMenuPlan startup_current;
        struct StartupPress { ui::ClientPoint point; ui::FrontendMenuPlan plan; };
        struct StartupClick { StartupPress press; ui::ClientPoint release; };
        std::optional<StartupPress> startup_press;
        std::optional<StartupClick> startup_click;
        std::optional<ui::WorldViewPoint> startup_pointer;
        std::optional<std::uint64_t> startup_presented;
        std::optional<ui::ClientViewport> startup_presented_viewport;
        std::optional<std::tuple<std::uint64_t, ui::ClientViewport, HWND>> startup_observation;
        struct StartupRetirement {
            std::unique_ptr<FrontendProcessBackend> &backend;
            std::unique_ptr<ui::Win32ClientWindowBinding> &binding;
            std::shared_ptr<ui::detail::CivisFrontendPartyRoutingOwner> &routing;
            std::function<void(const ui::ClientWindowEvent &)> &sink;
            bool &retiring;
            void close() {
                if (retiring) return;
                retiring = true;
                if (backend) backend->should_close = true;
                backend.reset();
                sink = {};
                if (routing) routing->retire_owned();
                binding.reset();
            }
            ~StartupRetirement() { close(); }
        } startup_retirement{backend_storage, binding_owner, routing, active_sink, menu_retiring};

        // Existing sole context/window/input are available before full catalogs.
        context_owner = std::make_shared<graphics::GlContext>(configuration.width, configuration.height, true);
        auto &context = *context_owner;
        std::cout << "GL_DEVICE=" << context.description() << '\n';
        const auto window = static_cast<HWND>(context.native_window_handle());
        if (!window) throw std::runtime_error("Standalone frontend has no owned window");
        graphics::GlRenderApi gl(context);
        routing = ui::detail::CivisFrontendPartyRoutingOwner::create(context_owner);
        auto &input = routing->window_input();
        active_sink = [&](const ui::ClientWindowEvent &event) {
            if (!startup) return;
            if (event.kind == ui::ClientWindowEventKind::close ||
                (event.kind == ui::ClientWindowEventKind::key_down && event.key == VK_ESCAPE &&
                 !(event.native_flags & (1u << 30)))) {
                startup_press.reset(); startup_click.reset(); startup->request_exit();
            } else if (event.kind == ui::ClientWindowEventKind::cancelled ||
                       event.kind == ui::ClientWindowEventKind::resize ||
                       event.kind == ui::ClientWindowEventKind::dpi_changed) {
                const bool completed_release = startup_click && event.kind == ui::ClientWindowEventKind::cancelled &&
                    event.native_flags == WM_CAPTURECHANGED;
                startup_press.reset(); if (!completed_release) startup_click.reset(); startup_pointer.reset();
            } else if (event.kind == ui::ClientWindowEventKind::key_down && event.owner == ui::ClientInputOwner::ui) {
                if (event.key == VK_NEXT || event.key == VK_DOWN) startup->scroll(3);
                if (event.key == VK_PRIOR || event.key == VK_UP) startup->scroll(-3);
            } else if (event.kind == ui::ClientWindowEventKind::pointer && event.owner == ui::ClientInputOwner::ui) {
                if (event.pointer.framebuffer) startup_pointer = ui::WorldViewPoint{
                    event.pointer.framebuffer->x, event.pointer.framebuffer->y};
                if (event.pointer_kind == ui::ClientPointerKind::wheel) {
                    startup_press.reset(); startup->scroll(event.wheel_delta > 0 ? -3 : 3);
                } else if (event.pointer_kind == ui::ClientPointerKind::left_down && event.pointer.framebuffer &&
                           event.pointer.capture && startup_presented == startup_current.revision &&
                           startup_presented_viewport == input.viewport()) {
                    startup_press = StartupPress{*event.pointer.framebuffer, startup_current};
                } else if (event.pointer_kind == ui::ClientPointerKind::left_up) {
                    if (startup_press && event.pointer.framebuffer)
                        startup_click = StartupClick{std::move(*startup_press), *event.pointer.framebuffer};
                    startup_press.reset();
                }
            }
        };
        binding_owner = std::make_unique<ui::Win32ClientWindowBinding>(window, input, [&](const auto &event) {
            if (menu_retiring) return;
            if (active_sink) active_sink(event);
        });
        input.set_ui({}, true);
        pump_startup = [&] {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) { startup_quit = true; startup->request_exit(); break; }
                TranslateMessage(&message); DispatchMessageW(&message);
                if (binding_owner->failed()) throw std::runtime_error("Startup native input binding failed");
                if (startup_click) {
                    auto click = std::move(*startup_click); startup_click.reset();
                    if (startup_presented == click.press.plan.revision &&
                        startup_presented_viewport == input.viewport() && input.viewport().drawable())
                        (void)startup->click(click.press.plan, click.press.point.x, click.press.point.y,
                                             click.release.x, click.release.y);
                }
            }
        };
        paint_startup = [&] {
            const auto viewport = input.viewport();
            if (!viewport.drawable() || startup_quit) return;
            const auto value = startup->snapshot();
            auto title_text = startup_captions.at("frontend.startup.title");
            if (const auto *stage = value.current()) {
                const auto found = startup_captions.find(stage->title_key);
                if (found != startup_captions.end()) title_text += " — " + found->second;
                if (stage->total_units) title_text += " (" + std::to_string(stage->completed_units) +
                    "/" + std::to_string(*stage->total_units) + ")";
            }
            const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title_text.data(),
                static_cast<int>(title_text.size()), nullptr, 0);
            if (count <= 0) throw std::runtime_error("Startup title is not valid Unicode");
            std::wstring title_text_wide(static_cast<std::size_t>(count), L'\0');
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title_text.data(),
                static_cast<int>(title_text.size()), title_text_wide.data(), count) != count ||
                !SetWindowTextW(window, title_text_wide.c_str())) throw std::runtime_error("Cannot set startup title");
            gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
            gl.Viewport(0, 0, viewport.framebuffer_width, viewport.framebuffer_height);
            gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            gl.ClearColor(0.02f, 0.03f, 0.06f, 1.f); gl.Clear(GL_COLOR_BUFFER_BIT);
            auto *presenter = painter_storage ? painter_storage.get() : boot_painter.get();
            if (presenter) {
                startup_current = startup->compose(viewport.framebuffer_width, viewport.framebuffer_height, startup_pointer);
                ui::ComposedUiFrame frame;
                frame.paints = startup_current.paints; frame.unresolved = startup_current.unresolved;
                frame.focused_context = startup_current.screen;
                frame.frame_owner = startup_current.owner; frame.frame_sequence = startup_current.revision;
                const auto report = presenter->paint(frame, viewport.framebuffer_width, viewport.framebuffer_height);
                for (const auto &reason : report.unresolved)
                    if (startup_reported.insert(reason).second) std::cerr << "Startup presentation: " << reason << '\n';
                std::vector<ui::ClientRect> hits;
                for (const auto &hit : startup_current.hits)
                    hits.push_back({hit.bounds.x, hit.bounds.y, hit.bounds.width, hit.bounds.height});
                input.set_ui(hits, true);
            }
            context.present();
            startup_presented = startup_current.revision; startup_presented_viewport = viewport;
            // Separate diagnostic journal; never Source31's menu/session records.
            const auto observation = std::tuple{startup_current.revision, viewport, GetForegroundWindow()};
            if (presenter && (startup_current.screen == "startup_legal" || value.phase == runtime::StartupLoadingPhase::failed) &&
                (!startup_observation || *startup_observation != observation)) {
                std::cout << "STARTUP_PRESENTED {\"screen\":" << capture_json_string(startup_current.screen)
                    << ",\"window_hwnd\":" << reinterpret_cast<std::uintptr_t>(window)
                    << ",\"foreground_hwnd\":" << reinterpret_cast<std::uintptr_t>(GetForegroundWindow())
                    << ",\"revision\":" << startup_current.revision << ",\"width\":" << viewport.framebuffer_width
                    << ",\"height\":" << viewport.framebuffer_height << ",\"hits\":[";
                bool first = true;
                for (const auto &hit : startup_current.hits) {
                    if (!first) std::cout << ','; first = false;
                    std::cout << "{\"id\":" << capture_json_string(hit.id) << ",\"enabled\":" << (hit.enabled ? "true" : "false")
                        << ",\"x\":" << hit.bounds.x << ",\"y\":" << hit.bounds.y << ",\"width\":" << hit.bounds.width
                        << ",\"height\":" << hit.bounds.height << '}';
                }
                std::cout << "]}\n" << std::flush;
                startup_observation = observation;
            }
        };
        std::vector<runtime::StartupLoadingStage> startup_stages;
        for (const auto &[id, caption] : std::initializer_list<std::pair<const char *, const char *>>{
                 {"bootstrap", "frontend.startup.presentation"}, {"resources", "frontend.startup.resources"},
                 {"localization", "frontend.startup.localization"}, {"sources", "frontend.startup.localization"},
                 {"content", "frontend.startup.content"}, {"menu", "frontend.startup.menu"},
                 {"presentation", "frontend.startup.presentation"}}) {
            startup_stages.push_back({id, caption, [&](const auto &progress) {
                if (!active_startup_step) throw std::logic_error("Startup stage has no application work");
                active_startup_step(progress); return runtime::StartupStepResult::completed;
            }});
        }
        startup = std::make_unique<ui::FrontendStartupSession>(std::move(startup_stages),
            [&](std::string_view key) -> std::optional<std::string> {
                const auto found = texts.rows.find(std::string(key));
                if (found != texts.rows.end()) return texts.format(key);
                const auto caption = startup_captions.find(key);
                return caption == startup_captions.end() ? std::nullopt : std::optional<std::string>(caption->second);
            }, [&](runtime::StartupLoadingEvent event, const auto &snapshot) {
                const auto *stage = snapshot.current();
                std::cerr << "STARTUP_EVENT event=" << static_cast<int>(event) << " stage="
                    << (stage ? stage->id : "") << " elapsed_ms=" << snapshot.elapsed.count() << '\n';
                paint_startup(); pump_startup();
            }, configuration.smoke_frames.has_value()); // Presentation diagnostics only; normal launch uses real Continue.
        const auto run_stage = [&](std::string_view id, auto &&work) {
            using Result = std::invoke_result_t<decltype(work), const runtime::StartupStageProgress &>;
            using Stored = std::conditional_t<std::is_void_v<Result>, std::monostate, Result>;
            const auto before = startup->snapshot();
            if (!before.current() || before.current()->id != id) throw std::logic_error("Startup stage order mismatch");
            std::optional<Stored> result;
            active_startup_step = [&](const auto &progress) {
                if constexpr (std::is_void_v<Result>) { work(progress); result.emplace(); }
                else result.emplace(work(progress));
            };
            const auto phase = startup->advance(); active_startup_step = {};
            if (phase == runtime::StartupLoadingPhase::cancelled) throw FrontendStartupCancelled{};
            if (phase == runtime::StartupLoadingPhase::failed) {
                const auto error = startup->snapshot().error;
                if (!boot_painter && !painter_storage) {
                    MessageBoxW(window, configuration.language == "ru_RU" ? L"Не удалось подготовить запуск. Подробности в журнале." :
                        L"Startup preparation failed. See the log for details.", L"Civis", MB_OK | MB_ICONERROR);
                } else while (startup->phase() == ui::FrontendStartupPhase::failed && !startup_quit) {
                    pump_startup(); paint_startup(); MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
                }
                throw std::runtime_error(error);
            }
            if (!result) throw std::runtime_error("Startup callback did not return its result");
            if constexpr (!std::is_void_v<Result>) return std::move(*result);
        };
        run_stage("bootstrap", [&](const auto &progress) {
            boot_resources = std::make_unique<FrontendBootResources>(configuration.assets_root, configuration.language, progress);
            ui::TopPanelLocalizationCatalog boot_texts;
            boot_texts.rows.insert(startup_captions.begin(), startup_captions.end());
            boot_texts.rows["frontend.runtime.cyrillic"] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";
            boot_font = std::make_unique<BoundFont>(font(context, boot_resources->table, 20, boot_texts,
                extended_face(boot_resources->table, configuration.language))); progress.report(6, 7);
            boot_painter = std::make_unique<ui::UiScreenGlRenderer>(context, boot_resources->table,
                ui::UiGlyphBinding{&boot_font->geometry, &boot_font->texture},
                ui::UiGlyphBinding{&boot_font->geometry, &boot_font->texture});
            ui::FrontendStartupSkin initial_skin;
            initial_skin.background = "CivilzationVAtlas.dds"; // Same original FrontEnd.lua asset as MainMenu.
            startup->set_skin(std::move(initial_skin));
            startup->set_text_layout([&](std::string_view text, std::int32_t width) {
                return ui::layout_frontend_startup_text(boot_font->geometry, text, width);
            });
            progress.report(7, 7); progress.checkpoint();
        });
        resources_storage = std::make_unique<assets::ResourceMountTable>(run_stage("resources", [&](const auto &progress) {
            return mounted_resources(configuration.assets_root, *boot_resources, progress);
        }));
        auto &resources = *resources_storage;
        texts = run_stage("localization", [&](const auto &) {
            return ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources,
                configuration.language == "ru_RU" ? "Language_RU_RU" : "Language_en_US");
        });
        texts.rows.insert(startup_captions.begin(), startup_captions.end());
        if (configuration.language == "ru_RU") {
            texts.rows.insert_or_assign("TXT_KEY_MODS", "Моды");
            texts.rows.insert_or_assign("TXT_KEY_OTHER", "Дополнительно");
            texts.rows.insert_or_assign("TXT_KEY_AD_SETUP_DEFAULT", "Сбросить");
        }
        // Lua normally inserts current numbers into these labels. Until their
        // value owners are attached, name the unavailable setting without
        // fabricating a volume/count or feeding missing formatter arguments.
        const bool russian = configuration.language == "ru_RU";
        for (const auto &[key, english, russian_text] : std::initializer_list<
                 std::tuple<const char *, const char *, const char *>>{
                 {"frontend.caption.volume_music", "Music volume", "Громкость музыки"},
                 {"frontend.caption.volume_effects", "Sound effects volume", "Громкость эффектов"},
                 {"frontend.caption.volume_speech", "Speech volume", "Громкость речи"},
                 {"frontend.caption.volume_ambience", "Ambiance volume", "Громкость окружения"},
                 {"frontend.caption.city_states", "City-states", "Города-государства"},
                 {"frontend.caption.server_list_type", "Server list type", "Тип списка серверов"},
                 {"frontend.caption.map_options", "Map options", "Настройки карты"}})
            texts.rows.insert_or_assign(key, russian ? russian_text : english);
        const auto localize = [&texts, &configuration, &reported_status](std::string_view key) -> std::optional<std::string> {
            const auto found = texts.rows.find(std::string(key));
            if (found == texts.rows.end()) {
                if (const auto friendly = unavailable_presentation(key, configuration.language == "ru_RU")) {
                    if (reported_status.emplace(key).second) std::cerr << "Frontend availability: " << key << '\n';
                    return friendly;
                }
                return configuration.language == "ru_RU" ? russian_status(key) : std::nullopt;
            }
            return texts.format(key);
        };
        run_stage("sources", [&](const auto &) {
            registry = ui::FrontendRegistry::original();
            sources = ui::FrontendSourceCatalog::from_resources(resources);
            auto startup_skin = ui::FrontendStartupSkin::from_sources(sources);
            if (startup_skin.panel_style == "Grid9Detail162") {
                // Shared preview style selection currently reads base Styles.xml only.
                // Use the same supported original base frame as this menu's adapter.
                std::cerr << "Startup uses base Grid9Detail140 adaptation for selected Grid9Detail162\n";
                startup_skin.panel_style = "Grid9Detail140";
            }
            startup->set_skin(std::move(startup_skin));
            used_texts = menu_font_texts(registry, sources, texts);
            used_texts.rows.insert(startup_captions.begin(), startup_captions.end());
            face = extended_face(resources, configuration.language);
        });
        std::vector<fs::path> package_roots{configuration.assets_root / "Assets/DLC"};
        package_roots.insert(package_roots.end(), configuration.mods_roots.begin(), configuration.mods_roots.end());
        run_stage("content", [&](const auto &) {
        catalog = std::make_shared<content::Catalog>(content::discover_packages(package_roots));
        for (const auto &reason : catalog->diagnostics) std::cerr << "Content discovery: " << reason << '\n';
        auto saves = configuration.saves;
        if (saves.empty()) {
            std::vector<fs::path> roots{fs::current_path() / "saves"};
            PWSTR documents = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DONT_VERIFY, nullptr, &documents))) {
                roots.push_back(fs::path(documents) / "My Games/Sid Meier's Civilization 5/Saves");
                CoTaskMemFree(documents);
            }
            auto found = ui::discover_frontend_saves(roots);
            saves = std::move(found.saves);
            for (const auto &reason : found.diagnostics) std::cerr << "Save discovery: " << reason << '\n';
            // A packaged optional slot augments discovery; explicit --save stays authoritative.
            if (configuration.default_save) {
                std::error_code default_error;
                const auto optional_save = fs::canonical(*configuration.default_save, default_error);
                if (!default_error && fs::is_regular_file(optional_save, default_error) && !default_error) {
                    const bool listed = std::any_of(saves.begin(), saves.end(), [&](const auto &save) {
                        std::error_code same_error;
                        return fs::equivalent(save, optional_save, same_error);
                    });
                    if (!listed) saves.push_back(optional_save);
                }
            }
        }
        content_storage = std::make_unique<ui::FrontendCatalogContentAdapter>(catalog, saves);
        });
        const auto &content_adapter = *content_storage;
        run_stage("menu", [&](const auto &) {
        backend_storage = std::make_unique<FrontendProcessBackend>(content_adapter);
        auto &backend = *backend_storage;
        session_storage = std::make_unique<ui::FrontendMenuSession>(std::move(registry), std::move(sources), &backend, localize);
        auto &session = *session_storage;
        session.refresh(content_adapter.data());
        const auto pedia_localize = [&texts, &localize](std::string_view key) -> std::optional<std::string> {
            const auto found = texts.rows.find(std::string(key));
            // SystemLocalizer consumes raw authored rows. Its existing article
            // reader expands nested keys and reward arguments where supported.
            return found != texts.rows.end() ? std::optional<std::string>(found->second) : localize(key);
        };
        existing_storage = std::make_unique<ui::FrontendExistingScreens>(resources, pedia_localize);
        auto &existing = *existing_storage;
        session.set_external_presenter([&existing](const auto &screen, const auto &draft, int width, int height) {
            return existing.present(screen, draft, width, height);
        });
        session.set_external_dispatcher([&existing](const auto &screen, const auto &draft, const auto &command) {
            return existing.dispatch(screen, draft, command);
        });
        for (const auto &id : configuration.smoke_route) {
            const auto *item = session.registry().item(id);
            if (!item || item->screen != session.screen() || item->target_screen.empty())
                throw std::runtime_error("Smoke route permits only navigation from the current screen: " + id);
            const auto result = session.activate(id);
            if (result.status != ui::UiCommandStatus::navigation)
                throw std::runtime_error("Smoke route unavailable: " + id + ": " + result.reason);
        }
        std::size_t article_text = 0;
        for (auto &text : existing.font_texts())
            used_texts.rows.emplace("frontend.pedia." + std::to_string(article_text++), std::move(text));
        for (const auto &reason : session.sources().diagnostics()) std::cerr << "Frontend source: " << reason << '\n';
        });
        auto &backend = *backend_storage;
        auto &session = *session_storage;
        auto &existing = *existing_storage;
        run_stage("presentation", [&](const auto &) {
        small_storage = std::make_unique<BoundFont>(font(context, resources, 16, used_texts, face));
        auto &small = *small_storage;
        regular_storage = std::make_unique<BoundFont>(font(context, resources, 20, used_texts, face));
        auto &regular = *regular_storage;
        title_storage = std::make_unique<BoundFont>(font(context, resources, 24, used_texts, face));
        auto &title = *title_storage;
        icons_storage = std::make_unique<BoundFont>(icon_font(context, resources));
        auto &icons = *icons_storage;
        mapping = icon_mapping(resources);
        const auto resolve_icon = [&mapping](std::string_view key) -> std::optional<std::uint32_t> {
            const auto found = mapping.find(key);
            return found == mapping.end() ? std::nullopt : std::optional<std::uint32_t>(found->second);
        };
        painter_storage = std::make_unique<ui::UiScreenGlRenderer>(context, resources,
            ui::UiGlyphBinding{&small.geometry, &small.texture}, ui::UiGlyphBinding{&regular.geometry, &regular.texture},
            ui::UiGlyphBinding{&icons.geometry, &icons.texture}, resolve_icon,
            ui::UiGlyphBinding{&title.geometry, &title.texture});
        startup->set_text_layout([&small](std::string_view text, std::int32_t width) {
            return ui::layout_frontend_startup_text(small.geometry, text, width);
        });
        });
        auto &painter = *painter_storage;
        boot_painter.reset(); boot_font.reset();
        // This same final painter shows Legal before the menu becomes usable.
        while (startup->phase() == ui::FrontendStartupPhase::legal) {
            pump_startup(); paint_startup();
            if (!startup->menu_ready()) MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
        }
        if (startup->phase() == ui::FrontendStartupPhase::cancelled) throw FrontendStartupCancelled{};
        if (!startup->menu_ready()) throw std::runtime_error("Frontend startup did not reach the menu");
        if (!SetWindowTextW(window, L"Civis — Main Menu")) throw std::runtime_error("Cannot set frontend window title");
        // Startup input cannot spill into the first presented menu. The binding
        // remains attached to the same HWND; only the eventual sink changes.
        startup_current = {}; startup_press.reset(); startup_click.reset(); startup_pointer.reset();
        startup_presented.reset(); startup_presented_viewport.reset(); startup_observation.reset();
        input.cancel();
        MSG stale_startup_input{};
        while (PeekMessageW(&stale_startup_input, window, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
        while (PeekMessageW(&stale_startup_input, window, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) {}
        ui::FrontendMenuPlan current;
        struct Press { ui::ClientPoint point; ui::FrontendMenuPlan plan; };
        std::optional<Press> press;
        // Keyboard points are unused; a genuine key targets the focused presented hit.
        struct PendingClick { Press gesture; ui::ClientPoint release; bool keyboard = false; };
        std::optional<PendingClick> pending_click;
        struct PendingKeyboard { ui::FrontendMenuPlan plan; bool backwards = false, activate = false; };
        std::optional<PendingKeyboard> pending_keyboard;
        std::optional<std::uint64_t> presented_revision;
        std::optional<ui::ClientViewport> presented_viewport;
        std::optional<std::uint64_t> observed_revision;
        std::optional<ui::ClientViewport> observed_viewport;
        HWND observed_foreground = nullptr;
        std::optional<ui::WorldViewPoint> pointer;
        bool layout_dirty = true;
        bool loading_ui = false, loading_cancelled = false, loading_close = false, loading_viewport_changed = false;
        bool loading_presented = false, loading_pumping = false;
        ULONGLONG loading_started = 0, loading_last_pump = 0;
        std::string loading_stage;
        const auto menu_sink = [&](const ui::ClientWindowEvent &event) {
            // Teardown events cannot enter a backend already in its destructor.
            // This lifetime gate grants no input or gameplay readiness.
            if (menu_retiring) return;
            if (loading_ui) {
                // A loading pump must never reenter the menu executor. Native
                // resize/activation observations still run in the real binding;
                // the original resource/window owners validate them afterwards.
                if (event.kind == ui::ClientWindowEventKind::close) {
                    loading_cancelled = true; loading_close = true;
                } else if (event.kind == ui::ClientWindowEventKind::key_down && event.key == VK_ESCAPE)
                    loading_cancelled = true;
                // Even an away-and-back resize invalidates this load's initial
                // viewport epoch. Comparing just the final rectangle would miss it.
                if (event.kind == ui::ClientWindowEventKind::resize ||
                    event.kind == ui::ClientWindowEventKind::dpi_changed || IsIconic(window))
                    loading_viewport_changed = true;
                press.reset(); pending_click.reset(); pending_keyboard.reset(); pointer.reset();
                return;
            }
            if (routing->route(event)) return;
            if (event.kind == ui::ClientWindowEventKind::close) {
                press.reset(); pending_click.reset(); pending_keyboard.reset(); pointer.reset(); existing.cancel_text_input();
                if (!session.confirming()) session.request_exit();
                layout_dirty = true;
            } else if (event.kind == ui::ClientWindowEventKind::key_down && event.owner == ui::ClientInputOwner::ui) {
                if (event.key == VK_ESCAPE && !(event.native_flags & (1u << 30))) {
                    press.reset(); pending_click.reset(); pending_keyboard.reset(); pointer.reset(); existing.cancel_text_input(); session.back(); layout_dirty = true;
                } else if (!(event.native_flags & (1u << 30)) &&
                           (event.key == VK_TAB || event.key == VK_RETURN || event.key == VK_SPACE) &&
                           (session.confirming() || !existing.text_focused())) {
                    press.reset(); pending_click.reset(); pending_keyboard.reset(); pointer.reset();
                    if (presented_revision == current.revision && presented_viewport == input.viewport())
                        pending_keyboard = PendingKeyboard{current,
                            event.key == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                            event.key != VK_TAB};
                } else if (session.screen() == "pedia" && !session.confirming()) {
                    if (const auto command = existing.key_down(event.key)) {
                        session.dispatch(*command); layout_dirty = true;
                    }
                }
            } else if (event.kind == ui::ClientWindowEventKind::character &&
                       event.owner == ui::ClientInputOwner::ui && session.screen() == "pedia" && !session.confirming()) {
                if (const auto command = existing.character(event.key)) {
                    session.dispatch(*command); layout_dirty = true;
                }
            } else if (event.kind == ui::ClientWindowEventKind::cancelled ||
                       event.kind == ui::ClientWindowEventKind::resize ||
                       event.kind == ui::ClientWindowEventKind::dpi_changed) {
                // Native left_up is delivered before synchronous ReleaseCapture.
                // Its WM_CAPTURECHANGED notification ends capture, not the
                // already completed matching gesture queued for post-Dispatch.
                const bool completed_release = pending_click &&
                    event.kind == ui::ClientWindowEventKind::cancelled &&
                    event.native_flags == WM_CAPTURECHANGED;
                press.reset();
                if (!completed_release) pending_click.reset();
                pending_keyboard.reset();
                pointer.reset(); existing.cancel_text_input(); layout_dirty = true;
            } else if (event.kind == ui::ClientWindowEventKind::pointer && event.owner == ui::ClientInputOwner::ui) {
                pending_keyboard.reset();
                if (event.pointer.framebuffer) {
                    pointer = ui::WorldViewPoint{event.pointer.framebuffer->x, event.pointer.framebuffer->y};
                    layout_dirty = true;
                }
                if (event.pointer_kind == ui::ClientPointerKind::wheel) {
                    press.reset();
                    if (session.screen() == "pedia" && !session.confirming()) {
                        if (const auto command = existing.wheel(event.wheel_delta)) session.dispatch(*command);
                    } else session.scroll(event.wheel_delta > 0 ? -3 : 3);
                    layout_dirty = true;
                } else if (event.pointer_kind == ui::ClientPointerKind::left_down && event.pointer.framebuffer) {
                    // Queued input must not target a new menu/size that has
                    // been composed but has not actually reached SwapBuffers.
                    if (event.pointer.capture && presented_revision == current.revision &&
                        presented_viewport == input.viewport())
                        press = Press{*event.pointer.framebuffer, current};
                } else if (event.pointer_kind == ui::ClientPointerKind::left_up) {
                    // Only capture this presented gesture in the native sink.
                    // Effectful owners run after the Window procedure's POST.
                    if (press && event.pointer.framebuffer)
                        pending_click = PendingClick{std::move(*press), *event.pointer.framebuffer};
                    press.reset(); layout_dirty = true;
                }
            }
        };
        auto &binding = *binding_owner;
        const auto loading_progress = [&](std::string_view stage) {
            if (stage == "playing") {
                loading_ui = false;
                SetWindowTextW(window, L"Civis");
                return;
            }
            if (!loading_ui) {
                loading_ui = true;
                loading_presented = false;
                loading_started = GetTickCount64();
            }
            if (!loading_presented) {
                // Render once before Bank/GL-world preparation starts. Polling
                // below deliberately does not render into an in-flight GL stage.
                const auto viewport = input.viewport();
                ui::ComposedUiFrame frame;
                frame.paints = current.paints;
                frame.frame_owner = current.owner; frame.frame_sequence = current.revision;
                frame.focused_context = "loading";
                ui::WorldViewPaint shade;
                shade.destination = {0, 0, viewport.framebuffer_width, viewport.framebuffer_height};
                shade.rgba = 0x080d16ee;
                frame.paints.push_back(shade);
                ui::WorldViewPaint label;
                label.kind = ui::WorldViewPaintKind::text;
                label.destination = {32, viewport.framebuffer_height / 2 - 40,
                    std::max(1, viewport.framebuffer_width - 64), 120};
                label.clip = label.destination; label.style = "TwCenMT20:center"; label.rgba = 0xeae3caff;
                label.text = russian ? "Загрузка партии\nЭтап загрузки показан в заголовке окна.\nEsc — отменить" :
                    "Loading game\nThe current stage is shown in the window title.\nEsc to cancel";
                frame.paints.push_back(std::move(label));
                gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
                gl.Viewport(0, 0, viewport.framebuffer_width, viewport.framebuffer_height);
                gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
                gl.ClearColor(0.02f, 0.03f, 0.06f, 1.f); gl.Clear(GL_COLOR_BUFFER_BIT);
                painter.paint(frame, viewport.framebuffer_width, viewport.framebuffer_height);
                context.present();
                loading_presented = true;
            }
            const auto now = GetTickCount64();
            if (loading_pumping || (stage == loading_stage && now - loading_last_pump < 100)) return;
            loading_pumping = true;
            struct ClearPump { bool &flag; ~ClearPump() { flag = false; } } clear{loading_pumping};
            loading_stage.assign(stage); loading_last_pump = now;
            static const std::map<std::string_view, std::pair<const wchar_t*,const wchar_t*>> captions{
                {"checking-save", {L"Проверка сохранения", L"Checking save"}},
                {"checking-content", {L"Проверка содержимого", L"Checking content"}},
                {"loading-resources", {L"Чтение игровых ресурсов", L"Loading resources"}},
                {"loading-language", {L"Загрузка языка", L"Loading language"}},
                {"loading-interface", {L"Загрузка интерфейса", L"Loading interface"}},
                {"loading-fonts", {L"Загрузка шрифтов", L"Loading fonts"}},
                {"restoring-world", {L"Восстановление мира", L"Restoring world"}},
                {"preparing-terrain", {L"Подготовка карты", L"Preparing terrain"}},
                {"preparing-frame", {L"Подготовка кадра", L"Preparing frame"}},
                {"drawing-frame", {L"Отрисовка кадра", L"Drawing frame"}},
                {"starting-world", {L"Запуск партии", L"Starting world"}}};
            const auto caption = captions.find(stage);
            const std::wstring title_text = L"Civis — " + std::wstring(caption == captions.end() ?
                (russian ? L"Загрузка партии" : L"Loading game") :
                (russian ? caption->second.first : caption->second.second)) + L" — " +
                std::to_wstring((now - loading_started) / 1000) + (russian ? L" с" : L" s");
            SetWindowTextW(window, title_text.c_str());
            MSG message{};
            for (unsigned count = 0; count != 32 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count) {
                if (message.message == WM_QUIT) { loading_cancelled = true; loading_close = true; break; }
                TranslateMessage(&message); DispatchMessageW(&message);
            }
            if (binding.failed()) throw std::runtime_error("Standalone frontend native input binding failed");
            if (loading_viewport_changed) throw std::runtime_error("Window changed while loading; reopen the client to retry");
            if (loading_cancelled) throw std::runtime_error("Loading cancelled; reopen the client to retry");
        };
        // This late owner retires the backend while every sink capture is alive.
        // Closing also refuses callback effects during synchronous retirement.
        struct ProcessRetirement {
            std::unique_ptr<FrontendProcessBackend> owner;
            std::unique_ptr<ui::Win32ClientWindowBinding> &binding;
            ui::detail::CivisFrontendPartyRoutingOwner &routing;
            bool &retiring;
            ~ProcessRetirement() {
                retiring = true;
                owner->should_close = true;
                owner.reset();
                // The actual owner verifies the idle same binding/input and
                // retires its retained viewer/audio/metadata before detach.
                // Its input and context remain alive through binding teardown.
                routing.retire_owned();
                binding.reset();
            }
        } process_retirement{std::move(backend_storage), binding_owner, *routing, menu_retiring};
        active_sink = menu_sink;
        routing->attach_existing_binding(binding);
        if (configuration.party_game_root) {
            // Explicit source roots are factory inputs, never launch authority.
            // Reuse the actual saved-world producer and this sole binding.
            // The output path is passed as ordinary IO, without creating files here.
            backend.connect_application(ui::detail::create_civis_frontend_party_renderer_backend(
                content_adapter, routing, *configuration.party_game_root,
                *configuration.party_payload_root, party_output_directory(configuration), loading_progress));
        }
        // Retire application, then the actual viewer/audio/metadata owner,
        // then binding, before sink captures or the input/context can disappear.
        // Final graphics source adoption and genuine saved-world generation are
        // separate requirements; these calls grant no gameplay readiness.
        const auto layout = [&] {
            const auto &viewport = input.viewport();
            if (!viewport.drawable()) return;
            current = session.compose(viewport.framebuffer_width, viewport.framebuffer_height,
                configuration.smoke_pointer ? configuration.smoke_pointer : pointer);
            std::vector<ui::ClientRect> hits;
            for (const auto &hit : current.hits)
                hits.push_back({hit.bounds.x, hit.bounds.y, hit.bounds.width, hit.bounds.height});
            input.set_ui(hits, true, session.screen() == "pedia" && !session.confirming() && existing.text_focused());
            layout_dirty = false;
        };
        std::set<std::string> reported;
        std::size_t frames = 0;
        bool captured = false, quitting = false;
        layout();
        while (!backend.should_close && !session.handed_off() && !quitting &&
               (!configuration.smoke_frames || frames < *configuration.smoke_frames)) {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) { quitting = true; break; }
                TranslateMessage(&message); DispatchMessageW(&message);
                if (binding.failed()) throw std::runtime_error("Standalone frontend native input binding failed");
                if (pending_keyboard) {
                    auto key = std::move(*pending_keyboard); pending_keyboard.reset();
                    const auto viewport = input.viewport();
                    if (presented_revision == key.plan.revision && presented_viewport == viewport && viewport.drawable()) {
                        if (key.activate) pending_click = PendingClick{Press{{}, std::move(key.plan)}, {}, true};
                        else (void)session.focus_next(key.plan, key.backwards);
                        layout_dirty = true;
                    }
                }
                if (pending_click) {
                    // Move/reset before the callback: an uncertain or nested
                    // action must not replay this gesture. Existing click()
                    // checks owner, session revision and matching release hit.
                    auto click = std::move(*pending_click);
                    pending_click.reset();
                    const auto &viewport = input.viewport();
                    if (presented_revision == click.gesture.plan.revision &&
                        presented_viewport == viewport && viewport.drawable()) {
                        if (acceptance && !click.keyboard) acceptance->click(window, viewport, click.gesture.plan,
                            click.gesture.point, click.release, session, nullptr);
                        const auto pressed_hit = click.keyboard ? session.focused_hit(click.gesture.plan) :
                            click.gesture.plan.hit(click.gesture.point.x, click.gesture.point.y);
                        const auto released_hit = click.keyboard ? pressed_hit : click.gesture.plan.hit(click.release.x, click.release.y);
                        std::optional<ui::UiCommandResult> loading_abort;
                        if (pressed_hit && pressed_hit == released_hit && pressed_hit->enabled && pressed_hit->id == "load_start") {
                            try { loading_progress("checking-save"); }
                            catch (const std::exception &error) {
                                loading_abort = ui::UiCommandResult{ui::UiCommandStatus::unavailable, error.what()};
                            }
                        }
                        // An aborted presentation never enters the effectful session/backend.
                        const auto clicked = loading_abort ? *loading_abort : click.keyboard ? session.activate_focused(click.gesture.plan) :
                            session.click(click.gesture.plan, click.gesture.point.x, click.gesture.point.y, click.release.x, click.release.y);
                        std::cerr << "[CIVIS_ACTION] id=" << capture_json_string(pressed_hit ? pressed_hit->id : "")
                            << " status=" << frontend_command_status(clicked.status) << " reason=" << capture_json_string(clicked.reason) << '\n';
                        std::cerr.flush();
                        loading_ui = false;
                        if (loading_close) backend.should_close = true;
                        if (loading_abort) {
                            loading_cancelled = false;
                            loading_stage.clear(); loading_last_pump = 0;
                            // Require a fresh menu Swap before another captured gesture.
                            presented_revision.reset(); presented_viewport.reset();
                        }
                        if (IsWindow(window) && !session.handed_off()) SetWindowTextW(window, L"Civis — Main Menu");
                        if (acceptance && !click.keyboard) acceptance->click(window, viewport, click.gesture.plan,
                            click.gesture.point, click.release, session, &clicked);
                    }
                    layout_dirty = true;
                }
                if (backend.should_close || session.handed_off()) break;
                if (layout_dirty) layout();
            }
            if (backend.should_close || session.handed_off() || quitting) break;
            if (layout_dirty) layout();
            const auto viewport = input.viewport();
            if (!viewport.drawable()) { WaitMessage(); continue; }
            gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
            gl.Viewport(0, 0, viewport.framebuffer_width, viewport.framebuffer_height);
            gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            gl.ClearColor(0.02f, 0.03f, 0.06f, 1.f); gl.Clear(GL_COLOR_BUFFER_BIT);
            ui::ComposedUiFrame frame;
            frame.paints = current.paints; frame.unresolved = current.unresolved;
            frame.focused_context = current.screen;
            frame.frame_owner = current.owner; frame.frame_sequence = current.revision;
            const auto result = painter.paint(frame, viewport.framebuffer_width, viewport.framebuffer_height);
            for (const auto &reason : result.unresolved)
                if (reported.insert(reason).second) std::cerr << "Frontend presentation: " << reason << '\n';
            if (configuration.capture && !captured) {
                capture(context, *configuration.capture, viewport.framebuffer_width, viewport.framebuffer_height);
                captured = true;
            }
            context.present(); ++frames;
            presented_revision = current.revision; presented_viewport = viewport;
            if (acceptance && (observed_revision != presented_revision || observed_viewport != presented_viewport ||
                               observed_foreground != GetForegroundWindow())) {
                acceptance->presented(window, viewport, frames, current, session, result.unresolved);
                observed_revision = presented_revision; observed_viewport = presented_viewport;
                observed_foreground = GetForegroundWindow();
            }
            if (!configuration.smoke_frames) MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
        }
        if (configuration.smoke_frames && frames != *configuration.smoke_frames)
            throw std::runtime_error("Frontend smoke interrupted before requested frame count");
        if (configuration.capture && !captured) throw std::runtime_error("No drawable frontend frame was captured");
        if (session.handed_off() && !backend.should_close)
            std::cout << "Standalone frontend handed control to its connected application executor\n";
        else std::cout << "Standalone frontend presented " << frames << " frame(s); no game world loaded\n";
        return EXIT_SUCCESS;
    } catch (const FrontendStartupCancelled &) {
        std::cout << "Frontend startup cancelled before menu handoff\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        startup_trace("Frontend: handled error\n", 24);
        std::cerr << error.what() << '\n';
        std::cerr << "Usage: civis_frontend --assets-root DIR [--language en_US|ru_RU] "
                     "[--width N] [--height N] [--mods-root DIR ...] [--save FILE ...] [--default-save FILE] "
                     "[--party-game-root DIR --party-payload-root DIR] [--party-output-dir DIR] [--smoke-frames N] [--smoke-route item,item] [--smoke-pointer X,Y] [--capture FILE.ppm] [--acceptance-observation FILE.jsonl]\n";
        startup_trace("Frontend: exit failure\n", 23);
        return EXIT_FAILURE;
    }
}
