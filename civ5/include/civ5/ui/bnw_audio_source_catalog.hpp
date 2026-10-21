#pragma once
#include "civ5/ui/ui_audio_assets.hpp"
#include "civ5/ui/bnw_audio_window_activation_loan.hpp"
#include <memory>
#include <thread>
#include <functional>
#include <optional>

namespace civ5::ui::detail {class BnwNativeCompletedResourceBankCapability;}
namespace civ5::ui {
class BnwNativePackageDiscoveryOwner;class SelectedBnwPreviewHost;
struct BnwAudioNativeXmlDocument {
 std::string basename,sha256;
 std::uint64_t bytes=0;
 content::XmlNode document;
};
enum class BnwAudioNativeBasenameResult { inspected_miss, inspected_hit };
struct BnwAudioNativeWavPayload {
 std::string basename,sha256;
 std::vector<std::uint8_t> bytes;
};
struct BnwAudioNativeUserSettings {
 std::filesystem::path logical_path,selected_path;
 std::string sha256,raw;
 std::uint64_t bytes=0;
};
// Genuine completed source storage only. Identity is not XML/stream/device readiness.
class BnwAudioCompletedResourceOwner final {
 public:
 // Consume only the canonical Bank already issued and retained by the
 // completed startup/host. This never recaptures Bank or marks it ready.
 //Use ONLY the genuine private early Parent already adopted by this Host.
 //No caller mounts/path and no second Bank capture or readiness assertion.
 static std::shared_ptr<const BnwAudioCompletedResourceOwner> from_selected_host(
  std::shared_ptr<const SelectedBnwPreviewHost>);
 static std::shared_ptr<const BnwAudioCompletedResourceOwner> from_registered_parent(
  std::shared_ptr<const BnwNativePackageDiscoveryOwner>);
 static std::shared_ptr<const BnwAudioCompletedResourceOwner> from_completed_bank(
  std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>);
 static std::shared_ptr<const BnwAudioCompletedResourceOwner> from_completed_bank(
  std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
  std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>);
 // Exact first native AudioParams bytes only; no table/stream escapes.
 void with_native_parameters_bytes(const std::function<void(std::span<const std::uint8_t>)>&)const;
 // Closed native selected-basename read for authored audio XML dependencies.
 // No filesystem/table fallback, activation grant or persistent handle.
 BnwAudioNativeXmlDocument read_native_audio_xml(std::string_view basename)const;
 // Real bounded SelectedRead/FileReader payload. Ordinary misses/exceptions
 // are errors here, never a fallback path or resource-ready assertion.
 BnwAudioNativeWavPayload read_native_audio_wav(std::string_view basename)const;
 // Exact existing Boot-selected UserSettings bytes; never a caller path or VFS XML.
 BnwAudioNativeUserSettings read_native_audio_user_settings()const;
 // Closed genuine Parent/Bank record operation; copied inspected result only.
 // SelectedRead MISS is never caught or promoted to ordinary absence.
 BnwAudioNativeBasenameResult inspect_native_audio_basename(std::string_view)const;
#ifdef _WIN32
 void require_same_actual_window(const BnwAudioWindowActivationLoan::SourceHandle&)const;
#endif
 ~BnwAudioCompletedResourceOwner();
 BnwAudioCompletedResourceOwner(const BnwAudioCompletedResourceOwner&)=delete;
 BnwAudioCompletedResourceOwner& operator=(const BnwAudioCompletedResourceOwner&)=delete;
 void require_current()const;
 void with_resource_query_loan(const std::function<void(const assets::ResourceMountTable&)>&)const;
 private:
 friend class BnwAudioSourceCatalog;friend class BnwAudioUserSettingsInput;
 friend class BnwAudioConstructorRngOwner;friend class CivisStandaloneAudioOwner;
 friend class BnwAudioLiveKnobOwner;friend class LiveUiAudioBridge;
 friend class BnwNewEraNativeXmlOwner;friend class BnwAudioNativeSettingsOwner;friend class BnwAudioNativeFilenameProbes;
 std::weak_ptr<const BnwAudioCompletedResourceOwner> factory_owner_;
 std::shared_ptr<const BnwAudioCompletedResourceOwner> retain_factory_owner(
  const std::shared_ptr<const BnwAudioCompletedResourceOwner>&)const;
 std::shared_ptr<const BnwNativePackageDiscoveryOwner> retained_native_parent_for_audio_startup()const;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioCompletedResourceOwner(std::shared_ptr<Impl>);
};
// Native winner is authenticated by SAME private leaf. It supplies no mount,
// archive path or UiAudioSource provenance; this snapshot grants no device.
struct BnwAudioNativeParameters {
 std::string basename,sha256;
 content::XmlNode sound_types;
};
enum class BnwAudioXmlKind {scripts_2d,scripts_3d,definitions,parameters};
struct BnwAudioXmlRoot {BnwAudioXmlKind kind;std::string key;};
struct BnwAudioSourceRow {
 BnwAudioXmlKind kind;
 std::string id;
 content::XmlNode row;
 UiAudioSource source;
 std::vector<content::XmlNode> authored_defaults;
};
// Resource component, not a script/event issuer or a full audio startup grant.
// Holds the actual resource-table allocation and authenticates selected bytes.
class BnwAudioSourceCatalog final {
 public:
 static std::shared_ptr<const BnwAudioSourceCatalog> from_resources(
  std::shared_ptr<const assets::ResourceMountTable>,std::span<const BnwAudioXmlRoot>);
 static std::shared_ptr<const BnwAudioSourceCatalog> from_completed_resources(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,std::span<const BnwAudioXmlRoot>);
 // Reject diagnostic raw-table catalogs; no caller relation/ready assertion.
 void require_same_completed_resources(const BnwAudioCompletedResourceOwner&)const;
 BnwAudioNativeParameters native_parameters()const;
 void require_current()const;
 // parameters layers retain the selected AudioParams/SoundTypes hierarchy.
 // They do not grant initialized knobs, current MASTER or voice gain.
 std::vector<UiAudioXmlLayer> layers(BnwAudioXmlKind)const;
 std::vector<BnwAudioSourceRow> rows(BnwAudioXmlKind,std::string_view id)const;
 std::shared_ptr<const assets::ResourceMountTable> resources()const;
 private:
    friend class LiveUiAudioBridge;
    friend class BnwAudioLiveKnobOwner;
    friend class BnwAudioUserSettingsInput;
    std::weak_ptr<const BnwAudioSourceCatalog> factory_owner_;
    std::shared_ptr<const BnwAudioSourceCatalog> retain_factory_owner(
        const std::shared_ptr<const BnwAudioSourceCatalog>&) const;
 struct Impl;std::shared_ptr<Impl> impl_;
 static std::shared_ptr<const BnwAudioSourceCatalog> build(
  std::shared_ptr<Impl>,std::span<const BnwAudioXmlRoot>);
 static std::shared_ptr<const BnwAudioSourceCatalog> build_native_parameters(
  std::shared_ptr<Impl>,std::span<const BnwAudioXmlRoot>);
 static void validate_current(const std::shared_ptr<Impl>&);
 explicit BnwAudioSourceCatalog(std::shared_ptr<Impl>);
};
}
