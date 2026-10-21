#pragma once

#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <variant>
#include <vector>

namespace civ5::ui {

// A frame is a value object. Screen-specific values are published as immutable,
// typed payloads by the live session owner; absent payloads remain absent.
class UiFrameSnapshot {
  public:
    UiFrameSnapshot(std::uint64_t frame, std::optional<std::int64_t> active_player_id,
                    std::optional<std::int32_t> turn);

    std::uint64_t frame() const { return frame_; }
    std::optional<std::int64_t> active_player_id() const { return active_player_id_; }
    std::optional<std::int32_t> turn() const { return turn_; }

    // Read-only payload lineage. Equal scalars alone are not provider admission.
    bool same_payload_storage(const UiFrameSnapshot& other) const noexcept {
        if(frame_!=other.frame_ || active_player_id_!=other.active_player_id_ ||
           turn_!=other.turn_ || payloads_.size()!=other.payloads_.size())return false;
        for(const auto& [type,payload]:payloads_){
            const auto it=other.payloads_.find(type);
            if(it==other.payloads_.end() || payload.get()!=it->second.get() ||
               payload.owner_before(it->second) || it->second.owner_before(payload))return false;
        }
        return true;
    }

    template <class T> UiFrameSnapshot with(std::shared_ptr<const T> value) const {
        UiFrameSnapshot next = *this;
        if (value) next.payloads_[std::type_index(typeid(T))] = std::move(value);
        return next;
    }

    template <class T> const T *find() const {
        const auto it = payloads_.find(std::type_index(typeid(T)));
        return it == payloads_.end() ? nullptr : static_cast<const T *>(it->second.get());
    }

  private:
    std::uint64_t frame_;
    std::optional<std::int64_t> active_player_id_;
    std::optional<std::int32_t> turn_;
    std::unordered_map<std::type_index, std::shared_ptr<const void>> payloads_;
};

enum class UiContextOwner { top_panel, minimap, world_view, progression, city, overview,
                            popup, system };

struct UiContextDescriptor {
    std::string id;
    std::string source_key;
    std::vector<std::string> containment_path;
    UiContextOwner owner = UiContextOwner::popup;
    bool initially_hidden = false;
    bool modal = false;
};

// The source order is the authored InGame.xml order, with WorldView.xml
// contexts inserted beneath the WorldView declaration. Multiplayer/debug
// contexts are deliberately outside the single-player registry.
class UiContextRegistry {
  public:
    static UiContextRegistry from_documents(const content::XmlNode &in_game,
                                             const content::XmlNode &world_view);
    static UiContextRegistry from_resources(const assets::ResourceMountTable &resources);

    const std::vector<UiContextDescriptor> &contexts() const { return contexts_; }
    const UiContextDescriptor *find(std::string_view id) const;

  private:
    std::vector<UiContextDescriptor> contexts_;
};

struct UiOpenScreen { std::string id; };
struct UiCloseScreen { std::string id; };

// Each screen defines its own concrete payload type. Dispatch is type-safe at
// both ends without growing a shared enum for every BNW gameplay command.
class UiGameplayCommand {
  public:
    template <class T> static UiGameplayCommand make(std::string source_id, T value) {
        return UiGameplayCommand(std::move(source_id), std::type_index(typeid(T)),
                                 std::make_shared<const T>(std::move(value)));
    }

    const std::string &source_id() const { return source_id_; }
    std::type_index type() const { return type_; }
    template <class T> const T *as() const {
        return type_ == std::type_index(typeid(T)) ? static_cast<const T *>(payload_.get()) : nullptr;
    }

  private:
    UiGameplayCommand(std::string source_id, std::type_index type,
                      std::shared_ptr<const void> payload);
    std::string source_id_;
    std::type_index type_;
    std::shared_ptr<const void> payload_;
};

using UiCommand = std::variant<UiOpenScreen, UiCloseScreen, UiGameplayCommand>;

enum class UiCommandStatus { ready, navigation, performed, unavailable, invalid, pending_refresh, uncertain };
struct UiCommandResult {
    UiCommandStatus status = UiCommandStatus::invalid;
    std::string reason;
};

class UiGameplayBackend {
  public:
    virtual ~UiGameplayBackend() = default;
    virtual UiCommandResult availability(const UiGameplayCommand &command,
                                          const UiFrameSnapshot &frame) const = 0;
    virtual UiCommandResult execute(const UiGameplayCommand &command,
                                     const UiFrameSnapshot &frame) = 0;
};

// Owns screen focus and popup order. The live game owns the frame and backend.
class UiScreenController {
  public:
    explicit UiScreenController(UiContextRegistry registry);
    UiCommandResult dispatch(const UiCommand &command, const UiFrameSnapshot &frame,
                             UiGameplayBackend *backend = nullptr);
    bool visible(std::string_view id) const;
    std::optional<std::string> focused_screen() const;
    const UiContextRegistry &registry() const { return registry_; }

  private:
    UiContextRegistry registry_;
    std::vector<std::string> opened_;
};

} // namespace civ5::ui
