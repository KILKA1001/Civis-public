#include "civ5/ui/ui_audio_events.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

constexpr auto base_popups = R"(<ButtonPopupSounds>
  <ButtonPopupSound><ID>BUTTONPOPUP_CHOOSETECH</ID><AudioScript>AS2D_PLACEHOLDER_BLANK</AudioScript></ButtonPopupSound>
  <ButtonPopupSound><ID>BUTTONPOPUP_TECH_TREE</ID><AudioScript>AS2D_INTERFACE_TECH_WINDOW</AudioScript></ButtonPopupSound>
</ButtonPopupSounds>)";
constexpr auto override_popups = R"(<ButtonPopupSounds>
  <ButtonPopupSound><ID>BUTTONPOPUP_CHOOSETECH</ID><AudioScript>AS2D_PLACEHOLDER_BLANK</AudioScript></ButtonPopupSound>
  <ButtonPopupSound><ID>BUTTONPOPUP_TECH_TREE</ID><AudioScript>AS2D_MOD_TECH_WINDOW</AudioScript></ButtonPopupSound>
</ButtonPopupSounds>)";
constexpr auto notifications = R"(<NotificationSounds>
  <NotificationSound><ID>NOTIFICATION_TECH</ID><AudioScript>AS2D_INTERFACE_TECH</AudioScript></NotificationSound>
  <NotificationSound><ID>NOTIFICATION_MINOR</ID><AudioScript>AS2D_PLACEHOLDER_BLANK</AudioScript></NotificationSound>
</NotificationSounds>)";

void write(const fs::path &root, std::string_view name, std::string_view value) {
    const auto path = root / fs::path(name);
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << value;
    if (!out) throw std::runtime_error("Could not write UI audio fixture");
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto catalog = ui::UiAudioCatalog::from_documents(
            content::read_xml_metadata(base_popups),
            content::read_xml_metadata(notifications));
        require(catalog.popups().size() == 2 &&
                    catalog.popups()[1].id == "BUTTONPOPUP_TECH_TREE" &&
                    catalog.popups()[1].ordinal == 1,
                "authored popup order retained");
        require(catalog.find(ui::UiAudioKind::notification, "NOTIFICATION_TECH") &&
                    catalog.find(ui::UiAudioKind::popup, "NOTIFICATION_TECH") == nullptr,
                "popup and notification ID spaces stay separate");

        ui::UiAudioQueue queue(catalog);
        require(queue.emit(ui::UiAudioKind::popup, "BUTTONPOPUP_CHOOSETECH") ==
                    ui::UiAudioEmitStatus::silent,
                "blank script stays silent");
        require(queue.emit(ui::UiAudioKind::popup, "UNKNOWN") ==
                    ui::UiAudioEmitStatus::unknown && queue.pending() == 0,
                "unknown ID is not invented");
        require(queue.emit(ui::UiAudioKind::notification, "NOTIFICATION_TECH") ==
                    ui::UiAudioEmitStatus::queued,
                "notification cue queued");
        require(queue.emit_script("SocialPolicyPopup", "AS2D_INTERFACE_POLICY") ==
                    ui::UiAudioEmitStatus::queued,
                "authored direct Lua script queued");
        require(queue.emit(ui::UiAudioKind::popup, "BUTTONPOPUP_TECH_TREE") ==
                    ui::UiAudioEmitStatus::queued,
                "popup cue queued");
        const auto events = queue.drain();
        require(events.size() == 3 && queue.pending() == 0 &&
                    events[0].sequence == 1 && events[1].sequence == 2 &&
                    events[2].sequence == 3 &&
                    events[0].script_id == "AS2D_INTERFACE_TECH" &&
                    events[2].authored_ordinal == 1,
                "events drain in emission order with authored ordinal");
        require(queue.drain().empty() &&
                    queue.emit_script("SocialPolicyPopup", "AS2D_PLACEHOLDER_BLANK") ==
                        ui::UiAudioEmitStatus::silent,
                "drain is destructive and placeholder never queues");

        bool duplicate_rejected = false;
        try {
            ui::UiAudioCatalog::from_documents(content::read_xml_metadata(
                R"(<ButtonPopupSounds><ButtonPopupSound><ID>X</ID><AudioScript>A</AudioScript></ButtonPopupSound><ButtonPopupSound><ID>X</ID><AudioScript>B</AudioScript></ButtonPopupSound></ButtonPopupSounds>)"),
                content::read_xml_metadata(notifications));
        } catch (const std::runtime_error &) { duplicate_rejected = true; }
        require(duplicate_rejected, "duplicate ID rejected");

        const auto root = fs::temp_directory_path() / "civis-ui-audio-events-test";
        const auto base_root = root / "base";
        const auto mod_root = root / "mod";
        write(base_root, "Assets/Sounds/XML/ButtonPopupSounds.xml", base_popups);
        write(base_root, "Assets/Sounds/XML/NotificationSounds.xml", notifications);
        write(mod_root, "Mods/Sounds/ButtonPopupSounds.xml", override_popups);
        auto base = std::make_shared<assets::LooseResourceIndex>(base_root);
        auto mod = std::make_shared<assets::LooseResourceIndex>(mod_root);
        base->add_file("Assets/Sounds/XML/ButtonPopupSounds.xml");
        base->add_file("Assets/Sounds/XML/NotificationSounds.xml");
        mod->add_file("Mods/Sounds/ButtonPopupSounds.xml");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({71, 0, 0, assets::resource_lookup(mod)});
        const auto selected = ui::UiAudioCatalog::from_resources(resources);
        require(selected.find(ui::UiAudioKind::popup, "BUTTONPOPUP_TECH_TREE") &&
                    selected.find(ui::UiAudioKind::popup, "BUTTONPOPUP_TECH_TREE")->script_id ==
                        "AS2D_MOD_TECH_WINDOW" &&
                    selected.popup_source() && selected.popup_source()->mount_id == 71 &&
                    selected.notification_source() && !selected.notification_source()->mount_id,
                "activated resource mount chooses popup override and base notification");
        std::error_code ignored;
        fs::remove_all(root, ignored);

        if (argc == 2) {
            const fs::path assets_root(argv[1]);
            const auto original = ui::UiAudioCatalog::from_documents(
                content::read_xml(assets_root / "Assets/Sounds/XML/ButtonPopupSounds.xml"),
                content::read_xml(assets_root / "Assets/Sounds/XML/NotificationSounds.xml"));
            require(original.popups().size() > 30 && original.notifications().size() > 20 &&
                        original.find(ui::UiAudioKind::popup, "BUTTONPOPUP_TECH_TREE") &&
                        original.find(ui::UiAudioKind::notification, "NOTIFICATION_TECH"),
                    "retail BNW audio tables parsed");
        }
        std::cout << "UI audio event tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
