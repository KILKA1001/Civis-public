#include "civ5/ui/bnw_notification_popup_owner.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <string_view>

namespace civ5::ui {
namespace {
// Selected Expansion2 CvEnums.h:597. BNW notification save version 2 writes
// the hash; versions 0/1 write legacy indices and are deliberately blocked.
constexpr std::uint32_t tech_notification_hash = 0x5acfa6f7u;
constexpr std::size_t bnw_notification_slots = 100;

std::optional<std::int64_t> field(const saves::PlayerAdministrationRecord &record,
                                  std::string_view name) {
    std::optional<std::int64_t> result;
    for (const auto &entry : record.fields) {
        if (entry.name && name == entry.name) {
            if (result) return std::nullopt;
            result = entry.value;
        }
    }
    return result;
}

std::optional<std::string> bytes(const saves::PlayerAdministrationRecord &record,
                                 std::string_view name) {
    const saves::PlayerAdministrationBlob *found = nullptr;
    for (const auto &blob : record.blobs) {
        if (blob.name && name == blob.name) {
            if (found) return std::nullopt;
            found = &blob;
        }
    }
    if (!found || found->bytes.size() >= 512 ||
        std::find(found->bytes.begin(), found->bytes.end(), 0) != found->bytes.end())
        return std::nullopt;
    return std::string(found->bytes.begin(), found->bytes.end());
}

struct Notification {
    std::int64_t lookup = -1, player = -1, data = -1, extra = -1;
    std::int32_t x = -1, y = -1, turn = -1;
    std::uint32_t type = 0;
    bool dismissed = true;
    std::string message;
};

struct RingRead {
    std::vector<Notification> entries;
    std::string error;
};

RingRead read_ring(const runtime::LiveSession &session, std::int64_t player_id) {
    const auto &state = session.state();
    if (state.setup.profile != content::TechnologyProfile::bnw ||
        !state.world.all_players ||
        state.world.all_players->layout != saves::GameStateLayout::expansion2_version1 ||
        !state.setup.players.contains(player_id) ||
        !state.setup.players.at(player_id).alive)
        return {{}, "Live BNW player or decoded notification roster is unavailable"};
    const saves::SavedPlayerObservation *player = nullptr;
    for (const auto &candidate : state.world.all_players->players) {
        if (candidate.saved_player_id == player_id) {
            if (player) return {{}, "Decoded player ID is duplicated"};
            player = &candidate;
        }
    }
    if (!player || player->administration.layout !=
                       saves::GameStateLayout::expansion2_version1)
        return {{}, "Decoded BNW player administration is unavailable"};
    const saves::PlayerAdministrationRecord *ring = nullptr;
    for (const auto &block : player->administration.blocks) {
        if (block.name && std::string_view(block.name) == "notifications") {
            if (ring) return {{}, "Notification administration block is duplicated"};
            ring = &block;
        }
    }
    if (!ring || field(*ring, "has_notifications") != 1 ||
        field(*ring, "version") != 2 || field(*ring, "m_ePlayer") != player_id ||
        ring->children.size() != bnw_notification_slots)
        return {{}, "Version-2 BNW notification ring is incomplete"};
    const auto begin = field(*ring, "m_iNotificationsBeginIndex");
    const auto end = field(*ring, "m_iNotificationsEndIndex");
    const auto next_lookup = field(*ring, "m_iCurrentLookupIndex");
    if (!begin || !end || !next_lookup || *begin < 0 || *end < 0 ||
        *begin >= static_cast<std::int64_t>(bnw_notification_slots) ||
        *end >= static_cast<std::int64_t>(bnw_notification_slots) ||
        *next_lookup < 0)
        return {{}, "BNW notification ring indices are invalid"};

    RingRead result;
    std::set<std::int64_t> lookup_ids;
    for (auto index = *begin; index != *end;
         index = (index + 1) % static_cast<std::int64_t>(bnw_notification_slots)) {
        const auto &row = ring->children[static_cast<std::size_t>(index)];
        const auto lookup = field(row, "m_iLookupIndex");
        const auto owner = field(row, "m_ePlayerID");
        const auto type = field(row, "m_eNotificationType_bits");
        const auto data = field(row, "m_iGameDataIndex");
        const auto extra = field(row, "m_iExtraGameData");
        const auto x = field(row, "m_iX");
        const auto y = field(row, "m_iY");
        const auto turn = field(row, "m_iTurn");
        const auto dismissed = field(row, "m_bDismissed");
        const auto message = bytes(row, "m_strMessage");
        if (!row.name || std::string_view(row.name) != "notification" ||
            row.index != static_cast<std::uint64_t>(index) ||
            !lookup || *lookup < 0 || *lookup >= *next_lookup ||
            !lookup_ids.insert(*lookup).second ||
            !owner || *owner != player_id || !type || *type < 0 ||
            *type > UINT32_MAX || !data || !extra || !x || !y || !turn ||
            *turn < 0 || *turn > state.setup.game_turn ||
            !dismissed || (*dismissed != 0 && *dismissed != 1) || !message)
            return {{}, "BNW notification ring contains an invalid active slot"};
        result.entries.push_back(Notification{*lookup, *owner, *data, *extra,
            static_cast<std::int32_t>(*x), static_cast<std::int32_t>(*y),
            static_cast<std::int32_t>(*turn), static_cast<std::uint32_t>(*type),
            *dismissed != 0, std::move(*message)});
    }
    return result;
}
} // namespace

BnwNotificationPopupOwner::BnwNotificationPopupOwner(
    const runtime::LiveSession &session, std::int64_t active_player_id)
    : session_(&session), active_player_id_(active_player_id) {}

std::optional<std::string> BnwNotificationPopupOwner::tech_unavailable_reason(
    std::int64_t lookup_id, std::int32_t expected_turn,
    const BnwNotificationActivationServices &services) const {
    const auto &state = session_->state();
    if (active_player_id_ < 0 || active_player_id_ > INT32_MAX ||
        expected_turn < 0 || state.setup.game_turn != expected_turn ||
        lookup_id < 0 || lookup_id >= INT32_MAX)
        return "Notification player, turn or lookup is stale";
    if (popup_ || attempted_lookups_.contains(lookup_id) ||
        next_sequence_ > static_cast<std::uint64_t>(INT64_MAX))
        return "Notification popup is pending or activation was already attempted";
    if (!services.activate_notification || !services.minimap_notification ||
        !services.add_popup)
        return "BNW notification activation requires UI and AddPopup services";
    const auto ring = read_ring(*session_, active_player_id_);
    if (!ring.error.empty())
        return ring.error;
    const auto it = std::find_if(ring.entries.begin(), ring.entries.end(),
        [lookup_id](const Notification &entry) { return entry.lookup == lookup_id; });
    if (it == ring.entries.end() || it->dismissed ||
        it->type != tech_notification_hash)
        return "Lookup is not an active BNW technology notification";
    return std::nullopt;
}

BnwNotificationActivationResult BnwNotificationPopupOwner::activate_tech(
    std::int64_t lookup_id, std::int32_t expected_turn,
    const BnwNotificationActivationServices &services) {
    if (const auto reason = tech_unavailable_reason(lookup_id, expected_turn, services))
        return {BnwNotificationActivationStatus::blocked, *reason};
    const auto ring = read_ring(*session_, active_player_id_);
    const auto it = std::find_if(ring.entries.begin(), ring.entries.end(),
        [lookup_id](const Notification &entry) { return entry.lookup == lookup_id; });
    if (!ring.error.empty() || it == ring.entries.end() || it->dismissed)
        return {BnwNotificationActivationStatus::blocked,
                "Notification changed during activation"};

    // SDK CvNotifications.cpp:800-837: ActivateNotification, minimap pulse,
    // then AddPopup(CHOOSETECH, player, gameData, extraData), copying message.
    BnwNotificationPopupInfo popup{"BUTTONPOPUP_CHOOSETECH", active_player_id_,
        it->data, it->extra, 0, false, false, it->message, lookup_id,
        it->x, it->y, it->turn, expected_turn, next_sequence_};
    attempted_lookups_.insert(lookup_id);
    try {
        services.activate_notification(it->lookup, it->type, it->message,
            it->x, it->y, it->data, it->extra, it->player);
        services.minimap_notification(it->x, it->y, it->lookup + 1);
        services.add_popup(popup);
    } catch (...) {
        return {BnwNotificationActivationStatus::uncertain,
                "BNW notification activation may have partially reached the UI"};
    }
    popup_ = std::move(popup);
    ++next_sequence_;
    return {BnwNotificationActivationStatus::published, {}};
}

std::optional<LiveWorldViewNotificationFeed>
BnwNotificationPopupOwner::active_notifications() const {
    const auto &state = session_->state();
    if (state.setup.game_turn < 0 || state.setup.game_turn > INT32_MAX)
        return std::nullopt;
    const auto ring = read_ring(*session_, active_player_id_);
    if (!ring.error.empty()) return std::nullopt;
    LiveWorldViewNotificationFeed feed;
    feed.player_id = active_player_id_;
    feed.turn = static_cast<std::int32_t>(state.setup.game_turn);
    feed.owner_sequence = next_sequence_;
    for (const auto &entry : ring.entries) {
        if (entry.dismissed || entry.type != tech_notification_hash) continue;
        if (entry.message.empty() || entry.message.starts_with("TXT_KEY_"))
            return std::nullopt;
        feed.events.push_back(WorldViewNotification{
            entry.lookup, entry.message, {}, true, std::nullopt});
    }
    return feed;
}

std::optional<LivePopupEventFeed> BnwNotificationPopupOwner::active_popups() const {
    if (!popup_ || session_->state().setup.game_turn != popup_->turn)
        return std::nullopt;
    const auto ring = read_ring(*session_, active_player_id_);
    if (!ring.error.empty() || std::none_of(ring.entries.begin(), ring.entries.end(),
        [&](const Notification &entry) {
            return entry.lookup == popup_->notification_lookup && !entry.dismissed &&
                   entry.type == tech_notification_hash &&
                   entry.player == popup_->data1 && entry.data == popup_->data2 &&
                   entry.extra == popup_->data3 && entry.message == popup_->text &&
                   entry.x == popup_->notification_x &&
                   entry.y == popup_->notification_y &&
                   entry.turn == popup_->notification_turn;
        })) return std::nullopt;
    PopupEvent event;
    event.context_id = "TechPopup";
    event.event_id = static_cast<std::int64_t>(popup_->owner_sequence);
    return LivePopupEventFeed{active_player_id_, popup_->turn,
                              popup_->owner_sequence, {std::move(event)}};
}

std::optional<ProgressionTechPopupEvent>
BnwNotificationPopupOwner::active_tech_popup() const {
    if (!active_popups()) return std::nullopt;
    return ProgressionTechPopupEvent{active_player_id_,
        ProgressionTechPopupMode::research, std::nullopt,
        popup_->owner_sequence};
}

bool BnwNotificationPopupOwner::close(std::uint64_t owner_sequence) {
    if (!popup_ || !owner_sequence || popup_->owner_sequence != owner_sequence)
        return false;
    popup_.reset();
    return true;
}

void bind_bnw_notification_frame_sources(LiveSessionUiSources &sources,
                                         BnwNotificationPopupOwner &owner) {
    auto *bound = &owner;
    const auto player = owner.player_id();
    sources.active_notifications = [bound, player](const runtime::LiveSession &session)
        -> std::optional<LiveWorldViewNotificationFeed> {
        if (!bound->owns(session, player)) return std::nullopt;
        return bound->active_notifications();
    };
    sources.active_popups = [bound, player](const runtime::LiveSession &session)
        -> std::optional<LivePopupEventFeed> {
        if (!bound->owns(session, player)) return std::nullopt;
        return bound->active_popups();
    };
    sources.active_tech_popup = [bound, player](const runtime::LiveSession &session)
        -> std::optional<ProgressionTechPopupEvent> {
        if (!bound->owns(session, player)) return std::nullopt;
        return bound->active_tech_popup();
    };
}

} // namespace civ5::ui
