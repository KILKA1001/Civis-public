#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/ui/bnw_owned_notification_publication_owner.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_owned_ui_context.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/ui/ideology_layout.hpp"
#include "civ5/ui/system_screens.hpp"
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {

bool contains(const WorldViewRect &bounds, std::int32_t x, std::int32_t y) {
    return bounds.width > 0 && bounds.height > 0 &&
           static_cast<std::int64_t>(x) >= bounds.x &&
           static_cast<std::int64_t>(y) >= bounds.y &&
           static_cast<std::int64_t>(x) < static_cast<std::int64_t>(bounds.x) + bounds.width &&
           static_cast<std::int64_t>(y) < static_cast<std::int64_t>(bounds.y) + bounds.height;
}

std::optional<ProgressionScreen> focused_progression(const UiScreenController &screens) {
    const auto focused = screens.focused_screen();
    if (!focused) return std::nullopt;
    for (const auto candidate : {ProgressionScreen::technologies,
                                 ProgressionScreen::policies,
                                 ProgressionScreen::ideology,
                                 ProgressionScreen::technology_popup})
        if (*focused == progression_context_id(candidate)) return candidate;
    return std::nullopt;
}

} // namespace

LiveUiHost::LiveUiHost(UiContextRegistry registry, FrameProvider provider,
                       UiGameplayBackend *backend, LiveUiSources sources)
    : owned_screens_(std::in_place, std::move(registry)),
      screens_(*owned_screens_), provider_(std::move(provider)), backend_(backend),
      sources_(std::move(sources)) {
    if (!provider_) throw std::invalid_argument("Live UI frame provider is missing");
}

LiveUiHost::LiveUiHost(UiScreenController &screens, FrameProvider provider,
                       UiGameplayBackend *backend, LiveUiSources sources)
    : screens_(screens), provider_(std::move(provider)), backend_(backend),
      sources_(std::move(sources)) {
    if (!provider_) throw std::invalid_argument("Live UI frame provider is missing");
}

std::optional<std::pair<std::int64_t,std::int64_t>> LiveUiHost::selected_city()const {
    if(city_selection_control_)return city_selection_control_->city();
    return selected_city_;
}
BnwClientUnitDirtySnapshot LiveUiHost::unit_dirty_state()const{
    const auto storage=owned_plot_yield_ui_sources_;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(!city_selection_control_)throw std::logic_error("Live UI has no genuine selected unit dirty owner");
    city_selection_control_->require_current();
    if(selected_owner_lifetime_.expired())throw std::logic_error("Actual unit dirty UI owner lifetime retired");
    const auto result=city_selection_control_->unit_dirty_state();
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}
std::vector<BnwClientUnitDirtyEvent> LiveUiHost::take_unit_dirty_events(){
    const auto storage=owned_plot_yield_ui_sources_;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    (void)unit_dirty_state();const auto result=city_selection_control_->take_unit_dirty_events();
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}
void LiveUiHost::set_selected_city(std::optional<std::pair<std::int64_t,std::int64_t>> city) {
    if(city_selection_control_)city_selection_control_->select(city);
    else selected_city_=city;
}
LiveUiFrame LiveUiHost::begin_frame(std::int32_t viewport_width,
                                   std::int32_t viewport_height) {
    const auto storage=owned_plot_yield_ui_sources_;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    if(city_selection_control_)city_selection_control_->require_current();
    const auto selected_lifetime=selected_owner_lifetime_.lock();
    if(city_selection_control_&&!selected_lifetime)throw std::logic_error("Actual UI owner lifetime retired");
    if (viewport_width <= 0 || viewport_height <= 0)
        throw std::invalid_argument("Live UI viewport must be positive");
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;
    LiveUiFrame frame{provider_(), ++sequence_, this, {}, {}, {}, {}, {}, {}, {}, {}, false, false};
    if(notification_publication_owner_){
        BnwEngineUiTurnOwner::merge_notification_frame_from_owned_weak(engine_ui_turn_owner_,*this,frame);
    }
    if(city_selection_control_){
        const auto* flags=frame.snapshot.find<BnwClientUnitDirtySnapshot>();
        if(!flags||*flags!=unit_dirty_state())
            throw std::logic_error("Actual unit dirty publication does not match current client storage");
    }
    if (const auto *popup = frame.snapshot.find<ProgressionTechPopupState>();
        popup && frame.snapshot.active_player_id() && frame.snapshot.turn() &&
        popup->player_id == *frame.snapshot.active_player_id() && popup->token &&
        sources_.tech_popup_open_tree &&
        attempted_tech_tree_event_ !=
            std::optional<std::pair<std::int64_t, std::uint64_t>>{
                std::pair{popup->player_id, popup->token}})
        frame.technology_tree_transition_ready = true;
    const auto selected_city= this->selected_city();
    if (selected_city) {
        const auto *cities = frame.snapshot.find<CityScreenFrame>();
        if (!cities || !frame.snapshot.active_player_id() ||
            cities->player_id != *frame.snapshot.active_player_id() ||
            cities->player_id != selected_city->first ||
            std::none_of(cities->cities.begin(), cities->cities.end(),
                [&](const CityScreenCity &city) {
                    return city.owner == cities->player_id &&
                        city.id == selected_city->second;
                })) {
            set_selected_city(std::nullopt);
        } else {
            auto selected = *cities;
            selected.selected_city_id = selected_city->second;
            frame.snapshot = frame.snapshot.with<CityScreenFrame>(
                std::make_shared<const CityScreenFrame>(std::move(selected)));
        }
    }
    if (sources_.audio_bridge || sources_.civis_audio_transition) {
        const auto *world = frame.snapshot.find<WorldViewState>();
        if (world && frame.snapshot.active_player_id() &&
            world->player_id == *frame.snapshot.active_player_id()) {
            std::vector<const UiAuthoredAudioCue *> cues;
            for (const auto &notification : world->notifications)
                if (notification.audio_cue) cues.push_back(&*notification.audio_cue);
            std::stable_sort(cues.begin(), cues.end(),
                [](const auto *left, const auto *right) {
                    return left->sequence < right->sequence;
                });
            for (const auto *cue : cues) emit_audio(UiAudioKind::notification, *cue);
        }
    }
    if (sources_.pedia_entries) {
        SystemScreenFrame state;
        if (const auto *published = frame.snapshot.find<SystemScreenFrame>())
            state = *published;
        state.pedia_entries = *sources_.pedia_entries;
        state.selected_pedia_id = selected_pedia_id_;
        state.pedia_page = pedia_page_;
        state.pedia_category_tab = pedia_category_tab_;
        state.pedia_category_session_owned = !state.pedia_entries.empty();
        state.pedia_search = pedia_search_;
        state.pedia_history = pedia_history_;
        state.pedia_history_index = pedia_history_index_;
        state.pedia_article_first_line = pedia_article_first_line_;
        state.pedia_article_layout = civilopedia_article_layout(viewport_width,
                                                                 viewport_height);
        // Viewport changes may shorten an article. Clamp the local offset to
        // the selected article in this immutable frame before publishing it.
        state.pedia_article_first_line = std::min(state.pedia_article_first_line,
                                                  civilopedia_article_max_scroll(state));
        pedia_article_first_line_ = state.pedia_article_first_line;
        frame.snapshot = frame.snapshot.with<SystemScreenFrame>(
            std::make_shared<const SystemScreenFrame>(std::move(state)));
    }
    if (screens_.focused_screen() ==
        std::optional<std::string>{"ChoosePantheon"}) {
        const auto *events = frame.snapshot.find<PopupFrame>();
        const PopupEvent *owner_event = nullptr;
        if (events)
            for (const auto &candidate : events->events)
                if (candidate.context_id == "ChoosePantheon") {
                    owner_event = &candidate;
                    break;
                }
        if (!owner_event ||
            !valid_choose_pantheon_owner(frame.snapshot, *owner_event)) {
            (void)screens_.dispatch(UiCloseScreen{"ChoosePantheon"},
                                    frame.snapshot);
            pantheon_confirm_.reset();
        }
    }
    if (screens_.focused_screen() ==
        std::optional<std::string>{"ChooseReligion"}) {
        const auto *events = frame.snapshot.find<PopupFrame>();
        const PopupEvent *owner_event = nullptr;
        if (events)
            for (const auto &candidate : events->events)
                if (candidate.context_id == "ChooseReligion") {
                    owner_event = &candidate;
                    break;
                }
        if (!owner_event ||
            !valid_choose_religion_owner(frame.snapshot, *owner_event)) {
            (void)screens_.dispatch(UiCloseScreen{"ChooseReligion"},
                                    frame.snapshot);
            religion_selection_.reset();
        }
    }
    const auto focused_popup = screens_.focused_screen();
    const auto *popup_context = focused_popup
        ? screens_.registry().find(*focused_popup) : nullptr;
    const auto *popup_events = frame.snapshot.find<PopupFrame>();
    if (popup_context && popup_context->owner == UiContextOwner::popup &&
        popup_context->modal && popup_events && frame.snapshot.active_player_id() &&
        popup_events->player_id == *frame.snapshot.active_player_id()) {
        const auto event = std::find_if(popup_events->events.begin(), popup_events->events.end(),
            [&](const PopupEvent &value) { return value.context_id == *focused_popup; });
        if (event != popup_events->events.end()) {
            if (*focused_popup == "SetCityName" || *focused_popup == "SetUnitName") {
                if (!popup_text_ || popup_text_->context_id != *focused_popup ||
                    popup_text_->player_id != popup_events->player_id ||
                    popup_text_->event_id != event->event_id)
                    popup_text_ = PopupTextView{*focused_popup,
                        popup_events->player_id, event->event_id,
                        event->entered_text.value_or("")};
                frame.snapshot = frame.snapshot.with<PopupTextView>(
                    std::make_shared<const PopupTextView>(*popup_text_));
            }
            if (!popup_scroll_ || popup_scroll_->context_id != *focused_popup ||
                popup_scroll_->player_id != popup_events->player_id ||
                popup_scroll_->event_id != event->event_id ||
                popup_scroll_->choice_count != event->choices.size() ||
                popup_scroll_->viewport_width != viewport_width ||
                popup_scroll_->viewport_height != viewport_height)
                popup_scroll_ = PopupScrollState{*focused_popup, popup_events->player_id,
                    event->event_id, event->choices.size(), 0,
                    viewport_width, viewport_height};
            if (event->choices.empty()) popup_scroll_->first_visible = 0;
            else popup_scroll_->first_visible = std::min(
                popup_scroll_->first_visible, event->choices.size() - 1);
            frame.snapshot = frame.snapshot.with<PopupScrollView>(
                std::make_shared<const PopupScrollView>(PopupScrollView{
                    *focused_popup, popup_events->player_id, event->event_id,
                    popup_scroll_->first_visible}));
        }
    }
    if (focused_popup == std::optional<std::string>{"GoodyHutPopup"} &&
        popup_events && sources_.active_goody_hut_reward &&
        sources_.goody_hut_processed) {
        const auto event = std::find_if(popup_events->events.begin(),
            popup_events->events.end(), [](const PopupEvent &candidate) {
                return candidate.context_id == "GoodyHutPopup";
            });
        if (event != popup_events->events.end() &&
            valid_goody_hut_reward_owner(frame.snapshot, *event) &&
            attempted_goody_hut_close_ !=
                std::optional<std::pair<std::int64_t, std::uint64_t>>{
                    std::pair{event->goody_hut_reward->player_id,
                        event->goody_hut_reward->token}}) {
            try {
                const auto current = sources_.active_goody_hut_reward();
                const auto &owner = *event->goody_hut_reward;
                if (current && current->player_id == owner.player_id &&
                    current->turn == owner.turn &&
                    current->owner_sequence == owner.owner_sequence &&
                    current->token == owner.token &&
                    current->event_id == owner.event_id &&
                    current->goody_type == owner.goody_type &&
                    current->reward_amount == owner.reward_amount &&
                    current->selected_source_path == owner.selected_source_path)
                    frame.snapshot = frame.snapshot.with<GoodyHutCloseView>(
                        std::make_shared<const GoodyHutCloseView>(
                            GoodyHutCloseView{owner.player_id, owner.event_id,
                                owner.turn, owner.owner_sequence, owner.token,
                                owner.selected_source_path}));
            } catch (...) {
                // A failing native owner can never make the close hit ready.
            }
        }
    }
    if (!focused_popup || (*focused_popup != "SetCityName" &&
                           *focused_popup != "SetUnitName") ||
        !frame.snapshot.find<PopupTextView>()) popup_text_.reset();
    if (pantheon_confirm_) {
        const PopupEvent *current_event = nullptr;
        if (focused_popup == std::optional<std::string>{"ChoosePantheon"} &&
            popup_events)
            for (const auto &candidate : popup_events->events)
                if (candidate.context_id == "ChoosePantheon") {
                    current_event = &candidate;
                    break;
                }
        if (!current_event ||
            !valid_choose_pantheon_owner(frame.snapshot, *current_event) ||
            !valid_pantheon_visible_rows(*current_event) ||
            !valid_pantheon_confirm_geometry(
                *current_event->choose_pantheon) ||
            pantheon_confirm_->player_id != popup_events->player_id ||
            pantheon_confirm_->turn != *frame.snapshot.turn() ||
            pantheon_confirm_->owner_sequence != popup_events->owner_sequence ||
            pantheon_confirm_->event_id != current_event->event_id ||
            pantheon_confirm_->selected_source_path !=
                current_event->choose_pantheon->selected_source_path ||
            pantheon_confirm_->viewport_width != viewport_width ||
            pantheon_confirm_->viewport_height != viewport_height ||
            std::none_of(current_event->choices.begin(),
                current_event->choices.end(), [&](const PopupChoice &choice) {
                    return choice.id == pantheon_confirm_->belief_id &&
                        choice.available;
                }) ||
            !current_event->choose_pantheon->confirm_text_by_choice.contains(
                pantheon_confirm_->belief_id) ||
            current_event->choose_pantheon->confirm_text_by_choice.at(
                pantheon_confirm_->belief_id).empty())
            pantheon_confirm_.reset();
        else frame.snapshot = frame.snapshot.with<PantheonConfirmView>(
            std::make_shared<const PantheonConfirmView>(*pantheon_confirm_));
    }
    if (religion_selection_) {
        const PopupEvent *current_event = nullptr;
        if (focused_popup == std::optional<std::string>{"ChooseReligion"} &&
            popup_events)
            for (const auto &candidate : popup_events->events)
                if (candidate.context_id == "ChooseReligion") {
                    current_event = &candidate;
                    break;
                }
        if (!current_event ||
            !valid_choose_religion_owner(frame.snapshot, *current_event) ||
            current_event->choose_religion->mode !=
                ChooseReligionMode::founding ||
            religion_selection_->player_id != popup_events->player_id ||
            religion_selection_->turn != *frame.snapshot.turn() ||
            religion_selection_->owner_sequence != popup_events->owner_sequence ||
            religion_selection_->event_id != current_event->event_id ||
            religion_selection_->selected_source_path !=
                current_event->choose_religion->selected_source_path ||
            religion_selection_->viewport_width != viewport_width ||
            religion_selection_->viewport_height != viewport_height ||
            (religion_selection_->editing_name &&
             (!current_event->choose_religion->name_geometry ||
              current_event->choose_religion->name_geometry->viewport_width !=
                  viewport_width ||
              current_event->choose_religion->name_geometry->viewport_height !=
                  viewport_height ||
              !valid_religion_name_geometry(
                  *current_event->choose_religion->name_geometry))) ||
            std::none_of(current_event->choices.begin(),
                current_event->choices.end(), [&](const PopupChoice &choice) {
                    return choice.id == religion_selection_->religion_id &&
                        choice.available &&
                        choice.label == religion_selection_->default_name &&
                        choice.icon_atlas == "RELIGION_ATLAS_WHITE" &&
                        choice.portrait_index >= 0 &&
                        choice.portrait_index < 15;
                }))
            religion_selection_.reset();
        else frame.snapshot = frame.snapshot.with<ReligionSelectionView>(
            std::make_shared<const ReligionSelectionView>(*religion_selection_));
    }
    if (religion_selection_ && religion_selection_->editing_name) {
        popup_text_ = PopupTextView{"ChooseReligion",
            religion_selection_->player_id, religion_selection_->event_id,
            religion_selection_->draft};
        frame.snapshot = frame.snapshot.with<PopupTextView>(
            std::make_shared<const PopupTextView>(*popup_text_));
    }
    if (screens_.focused_screen() == std::optional<std::string>{"ProductionPopup"} &&
        sources_.production_backend)
        if (auto choices = sources_.production_backend->capture_choices(frame.snapshot)) {
            if (sources_.production_popup_source) {
                choices->visible_rows = production_popup_visible_rows(
                    *sources_.production_popup_source, viewport_height);
                choices->selected_source_path =
                    sources_.production_popup_source->selected_source_path;
                if (choices->visible_rows) {
                    if (!production_scroll_ ||
                        production_scroll_->player_id != choices->player_id ||
                        production_scroll_->city_id != choices->city_id ||
                        production_scroll_->selected_source_path !=
                            choices->selected_source_path)
                        production_scroll_ = ProductionScrollState{
                            choices->player_id, choices->city_id,
                            choices->selected_source_path, 0};
                    const auto max_first = choices->entries.size() > choices->visible_rows
                        ? choices->entries.size() - choices->visible_rows : 0;
                    production_scroll_->first_visible = std::min(
                        production_scroll_->first_visible, max_first);
                    choices->first_visible = production_scroll_->first_visible;
                }
            }
            frame.snapshot = frame.snapshot.with<ProductionPopupChoices>(
                std::make_shared<const ProductionPopupChoices>(std::move(*choices)));
        }
    if (sources_.world_skin && screens_.registry().find("CityList")) {
        const auto *world = frame.snapshot.find<WorldViewState>();
        const auto *cities = frame.snapshot.find<CityScreenFrame>();
        if (world && cities && frame.snapshot.active_player_id() &&
            world->player_id == *frame.snapshot.active_player_id() &&
            cities->player_id == world->player_id) {
            auto projected = *world;
            projected.city_list_launcher = !screens_.visible("CityList") &&
                sources_.world_skin->find("CityList", "ScrollPanel") &&
                sources_.world_skin->find("CityList", "Root") &&
                sources_.world_skin->find("CityList", "Population") &&
                sources_.world_skin->find("CityList", "CityName") &&
                sources_.world_skin->find("CityList", "CloseButton");
            frame.snapshot = frame.snapshot.with(
                std::make_shared<const WorldViewState>(std::move(projected)));
        }
    }
    if (sources_.world_skin)
        frame.world_view = plan_world_view(*sources_.world_skin, screens_.registry(),
                                           frame.snapshot, viewport_width, viewport_height,
                                           backend_);
    if (sources_.top_panel_layout) {
        frame.top_panel_bars = plan_top_panel(*sources_.top_panel_layout,
                                               static_cast<std::uint32_t>(viewport_width),
                                               static_cast<std::uint32_t>(viewport_height));
        frame.top_panel = present_top_panel_frame(frame.snapshot,
                                                   sources_.top_panel_localizer);
    }
    if (sources_.minimap_layout)
        frame.minimap = plan_minimap_panel(*sources_.minimap_layout,
                                           static_cast<std::uint32_t>(viewport_width),
                                           static_cast<std::uint32_t>(viewport_height));
    if (const auto focused = focused_progression(screens_)) {
        const auto index = static_cast<std::size_t>(*focused);
        if (sources_.progression[index])
            frame.progression = plan_progression_screen(*sources_.progression[index],
                                                         *focused, frame.snapshot);
        if (*focused == ProgressionScreen::technologies && frame.progression &&
            frame.snapshot.active_player_id() && frame.snapshot.turn() &&
            sources_.active_tech_tree_event && sources_.tech_tree_processed) {
            const auto *event = frame.snapshot.find<ProgressionTechnologyTreeEvent>();
            const auto *source = sources_.progression[index];
            if (event && source && event->token &&
                event->player_id == *frame.snapshot.active_player_id() &&
                event->turn == *frame.snapshot.turn() &&
                frame.progression->selected_source_path == source->selected_source_path &&
                attempted_tech_tree_close_ !=
                    std::optional<std::pair<std::int64_t, std::uint64_t>>{
                        std::pair{event->player_id, event->token}}) {
                try {
                    const auto current = sources_.active_tech_tree_event();
                    frame.technology_tree_close_ready = current &&
                        current->player_id == event->player_id &&
                        current->turn == event->turn && current->token == event->token;
                } catch (const std::exception &) {}
                catch (...) {}
            }
        }
    }
    if (screens_.focused_screen() == std::optional<std::string>{"SocialPolicyPopup"} &&
        frame.progression && frame.snapshot.active_player_id() && frame.snapshot.turn()) {
        const auto player = *frame.snapshot.active_player_id();
        const auto turn = *frame.snapshot.turn();
        const auto &source = frame.progression->selected_source_path;
        if (!policy_scroll_ || policy_scroll_->player_id != player ||
            policy_scroll_->turn != turn ||
            policy_scroll_->viewport_width != viewport_width ||
            policy_scroll_->viewport_height != viewport_height ||
            policy_scroll_->selected_source_path != source)
            policy_scroll_ = LiveUiFrame::PolicyScrollView{
                player, turn, viewport_width, viewport_height, source, {}};
        frame.policy_scroll = *policy_scroll_;
    }
    if (screens_.focused_screen() == std::optional<std::string>{"TechPopup"} &&
        frame.progression && sources_.tech_popup_scroll &&
        frame.snapshot.active_player_id() && frame.snapshot.turn()) {
        const auto *event = frame.snapshot.find<ProgressionTechPopupState>();
        const auto *source = sources_.progression[static_cast<std::size_t>(
            ProgressionScreen::technology_popup)];
        if (event && source && event->token &&
            event->unavailable_reason.empty() &&
            event->player_id == *frame.snapshot.active_player_id() &&
            !source->selected_source_path.empty() &&
            frame.progression->selected_source_path == source->selected_source_path) {
            std::optional<TechPopupScrollEvidence> observed;
            try {
                observed = sources_.tech_popup_scroll(frame.snapshot,
                    viewport_width, viewport_height);
            } catch (const std::exception &) { observed.reset(); }
            catch (...) { observed.reset(); }
            if (observed && observed->player_id == event->player_id &&
                observed->turn == *frame.snapshot.turn() &&
                observed->frame == frame.snapshot.frame() &&
                observed->event_token == event->token &&
                observed->selected_source_path == source->selected_source_path &&
                observed->viewport_width == viewport_width &&
                observed->viewport_height == viewport_height &&
                (observed->scroll_panel_height == viewport_height - 300 ||
                 observed->scroll_panel_height == viewport_height - 244) &&
                observed->scroll_panel_height > 0 &&
                observed->vertical_offset_pixels >= 0 &&
                observed->vertical_offset_pixels <= 1000000 &&
                observed->choice_count == frame.progression->items.size())
                frame.tech_popup_scroll = std::move(observed);
        }
    }
    if (screens_.focused_screen() == std::optional<std::string>{"ChooseIdeology"} &&
        frame.snapshot.active_player_id() && frame.snapshot.turn()) {
        const auto *event = frame.snapshot.find<ProgressionIdeologyPopupEvent>();
        const auto *source = sources_.progression[static_cast<std::size_t>(
            ProgressionScreen::ideology)];
        if (event && source && event->token &&
            event->player_id == *frame.snapshot.active_player_id()) {
            const auto player = *frame.snapshot.active_player_id();
            const auto turn = *frame.snapshot.turn();
            const auto &path = source->selected_source_path;
            if (!ideology_view_ || ideology_view_->player_id != player ||
                ideology_view_->turn != turn ||
                ideology_view_->event_token != event->token ||
                ideology_view_->selected_source_path != path ||
                ideology_view_->viewport_width != viewport_width ||
                ideology_view_->viewport_height != viewport_height)
                ideology_view_ = LiveUiFrame::IdeologyView{
                    player, -1, turn, viewport_width, viewport_height,
                    event->token, path, 0, 0,
                    LiveUiFrame::IdeologyOverlay::none, {}};
            const auto *evidence = frame.snapshot.find<IdeologyLayoutEvidence>();
            if (!evidence || evidence->player_id != player ||
                evidence->turn != turn || evidence->frame != frame.snapshot.frame() ||
                evidence->event_token != event->token ||
                evidence->selected_source_path != path ||
                (ideology_view_->branch_id >= 0 &&
                 std::none_of(evidence->branches.begin(), evidence->branches.end(),
                     [&](const auto &branch) {
                         return branch.branch_id == ideology_view_->branch_id;
                     }))) {
                ideology_view_->overlay = LiveUiFrame::IdeologyOverlay::none;
                ideology_view_->branch_id = -1;
                ideology_view_->list_offset = 0;
                ideology_view_->tenet_offset = 0;
                ideology_view_->tenet_collapsed.fill(false);
            }
            frame.ideology_view = *ideology_view_;
        } else ideology_view_.reset();
    } else ideology_view_.reset();
    if(city_selection_control_){
        city_selection_control_->require_current();
        const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
        if(!stamp||stamp->identity.selection_revision!=city_selection_control_->revision()||
            stamp->identity.ui_source_revision!=city_selection_control_->world_revision())
            throw std::logic_error("Actual selection/world changed while UI frame was assembled");
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    publish_major_tech_dirty(frame);
    // A reentered frame must not overwrite a newer real issued sequence.
    // Retain the exact completed bundle; do not sanitize or reconstruct it.
    if(frame.sequence==sequence_){
        auto issued=std::make_shared<const UiFrameSnapshot>(frame.snapshot);
        if(frame.sequence==sequence_){
            issued_plot_yield_snapshot_=std::move(issued);
            issued_plot_yield_sequence_=frame.sequence;
            issued_plot_yield_selection_=this->selected_city();
        }
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return frame;
}

void LiveUiHost::require_issued_frame_finality(const LiveUiFrame& frame)const {
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(owned_plot_yield_ui_sources_);
    if(plot_yield_ui_thread_!=std::this_thread::get_id() || frame.owner!=this ||
       !issued_plot_yield_snapshot_ || frame.sequence!=sequence_ ||
       issued_plot_yield_sequence_!=sequence_ || selected_city()!=issued_plot_yield_selection_ ||
       !frame.snapshot.same_payload_storage(*issued_plot_yield_snapshot_))
        throw std::runtime_error("Owned UI final actual host issue changed");
}

bool LiveUiHost::current(const LiveUiFrame &frame) const {
    const auto storage=owned_plot_yield_ui_sources_;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    if(city_selection_control_)city_selection_control_->require_current();
    const auto selected_lifetime=selected_owner_lifetime_.lock();
    if(city_selection_control_&&!selected_lifetime)throw std::logic_error("Actual UI owner lifetime retired");
    if (frame.owner != this || frame.sequence != sequence_) return false;
    if(city_selection_control_){
        const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
        if(!stamp||stamp->identity.selection_revision!=city_selection_control_->revision()||
            stamp->identity.ui_source_revision!=city_selection_control_->world_revision())return false;
    }
    const bool owners=ui_owners_current(frame.snapshot);
    if(city_selection_control_){
        city_selection_control_->require_current();
        const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
        if(!stamp||stamp->identity.selection_revision!=city_selection_control_->revision()||
            stamp->identity.ui_source_revision!=city_selection_control_->world_revision())return false;
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(city_selection_control_){
        city_selection_control_->require_current();
        const auto* stamp=frame.snapshot.find<BnwClientFrameStamp>();
        if(!stamp||stamp->identity.selection_revision!=city_selection_control_->revision()||
            stamp->identity.ui_source_revision!=city_selection_control_->world_revision())return false;
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return owners&&frame.sequence==sequence_;
}

UiCommandResult LiveUiHost::route(const UiCommand &command, const LiveUiFrame &frame) {
    if(city_selection_control_)city_selection_control_->require_current();
    const auto selected_lifetime=selected_owner_lifetime_.lock();
    if(city_selection_control_&&!selected_lifetime)throw std::logic_error("Actual UI owner lifetime retired");
    if (!current(frame)) return {UiCommandStatus::invalid, "UI frame is stale"};
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "LoadMenu" &&
        screens_.focused_screen() == std::optional<std::string>{"GameMenu"})
        return {UiCommandStatus::unavailable,
                "GameMenu LoadMenu needs the native QueuePopup transition"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *operation = gameplay->as<SystemOperation>();
            operation && operation->kind == SystemOperationKind::quick_save) {
            const auto *state = frame.snapshot.find<SystemScreenFrame>();
            if (gameplay->source_id() != "GameMenu" ||
                screens_.focused_screen() != std::optional<std::string>{"GameMenu"} ||
                !state || state->game_menu_quick_save_visible != true)
                return {UiCommandStatus::unavailable,
                        "Quick Save is hidden or has no active GameMenu owner"};
        }
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *operation = gameplay->as<SystemOperation>();
            operation && operation->kind == SystemOperationKind::open_load_menu) {
            const auto *state = frame.snapshot.find<SystemScreenFrame>();
            if (gameplay->source_id() != "GameMenu" ||
                screens_.focused_screen() != std::optional<std::string>{"GameMenu"} ||
                !state || state->game_menu_load_game_visible != true)
                return {UiCommandStatus::unavailable,
                        "Load Game is hidden or has no active GameMenu owner"};
            return {UiCommandStatus::unavailable,
                    "Native LoadMenu popup transition is not connected"};
        }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "GreatPersonRewardPopup")
        return {UiCommandStatus::unavailable,
            "Native great person reward popup owner is not connected"};
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "BarbarianCampPopup")
        return {UiCommandStatus::unavailable,
            "Native barbarian camp reward popup owner is not connected"};
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "NaturalWonderPopup")
        return {UiCommandStatus::unavailable,
            "Native natural wonder reward popup owner is not connected"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<GoodyHutClose>()) {
            const auto *queue = frame.snapshot.find<PopupFrame>();
            const PopupEvent *event = nullptr;
            if (queue)
                for (const auto &candidate : queue->events)
                    if (candidate.context_id == "GoodyHutPopup") {
                        event = &candidate;
                        break;
                    }
            const auto *view = frame.snapshot.find<GoodyHutCloseView>();
            if (gameplay->source_id() != "GoodyHutPopup" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"GoodyHutPopup"} ||
                !event || !valid_goody_hut_reward_owner(frame.snapshot, *event) ||
                !view || !sources_.active_goody_hut_reward ||
                !sources_.goody_hut_processed ||
                action->owner_sequence != queue->owner_sequence ||
                action->token != event->goody_hut_reward->token ||
                action->event_id != event->event_id ||
                view->player_id != event->goody_hut_reward->player_id ||
                view->turn != event->goody_hut_reward->turn ||
                view->owner_sequence != action->owner_sequence ||
                view->token != action->token ||
                view->event_id != action->event_id ||
                view->selected_source_path !=
                    event->goody_hut_reward->selected_source_path ||
                attempted_goody_hut_close_ ==
                    std::optional<std::pair<std::int64_t, std::uint64_t>>{
                        std::pair{view->player_id, view->token}})
                return {UiCommandStatus::unavailable,
                    "Current native goody hut reward owner is unavailable"};
            const auto owner = *event->goody_hut_reward;
            try {
                const auto current = sources_.active_goody_hut_reward();
                if (!current || current->player_id != owner.player_id ||
                    current->turn != owner.turn ||
                    current->owner_sequence != owner.owner_sequence ||
                    current->token != owner.token ||
                    current->event_id != owner.event_id ||
                    current->goody_type != owner.goody_type ||
                    current->reward_amount != owner.reward_amount ||
                    current->selected_source_path != owner.selected_source_path)
                    return {UiCommandStatus::unavailable,
                        "Native goody hut reward changed before close"};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "Native goody hut reward cannot be verified"};
            }
            attempted_goody_hut_close_ = std::pair{owner.player_id, owner.token};
            ++sequence_; // A partially processed native event cannot be retried.
            UiCommandResult acknowledged;
            try { acknowledged = sources_.goody_hut_processed(owner); }
            catch (...) {
                return {UiCommandStatus::unavailable,
                    "Goody hut popup close is uncertain; do not retry"};
            }
            if (acknowledged.status != UiCommandStatus::performed)
                return {UiCommandStatus::unavailable,
                    "Goody hut popup close was not confirmed; do not retry"};
            try {
                if (sources_.active_goody_hut_reward())
                    return {UiCommandStatus::unavailable,
                        "Goody hut popup acknowledgement is not observable"};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "Goody hut popup acknowledgement is uncertain"};
            }
            const auto closed = screens_.dispatch(
                UiCloseScreen{"GoodyHutPopup"}, frame.snapshot);
            return closed.status == UiCommandStatus::navigation ? closed
                : UiCommandResult{UiCommandStatus::unavailable,
                    "Goody hut reward acknowledged, but local focus close failed"};
        }
    if (const auto *close = std::get_if<UiCloseScreen>(&command);
        close && close->id == "GoodyHutPopup")
        return {UiCommandStatus::unavailable,
            "Goody hut reward close requires native popup acknowledgement"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<ProgressionCloseTechnologyTree>()) {
            const auto *event = frame.snapshot.find<ProgressionTechnologyTreeEvent>();
            if (gameplay->source_id() != "TechTree" ||
                screens_.focused_screen() != std::optional<std::string>{"TechTree"} ||
                !frame.technology_tree_close_ready || !event ||
                !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
                !event->token || action->event_token != event->token ||
                event->player_id != *frame.snapshot.active_player_id() ||
                event->turn != *frame.snapshot.turn() ||
                !sources_.active_tech_tree_event || !sources_.tech_tree_processed ||
                attempted_tech_tree_close_ ==
                    std::optional<std::pair<std::int64_t, std::uint64_t>>{
                        std::pair{event->player_id, event->token}})
                return {UiCommandStatus::unavailable,
                        "Active BNW technology tree popup owner is unavailable"};
            try {
                const auto current = sources_.active_tech_tree_event();
                if (!current || current->player_id != event->player_id ||
                    current->turn != event->turn || current->token != event->token)
                    return {UiCommandStatus::unavailable,
                            "BNW technology tree popup event changed"};
            } catch (const std::exception &) {
                return {UiCommandStatus::unavailable,
                        "BNW technology tree popup event cannot be verified"};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                        "BNW technology tree popup event cannot be verified"};
            }
            attempted_tech_tree_close_ = std::pair{event->player_id, event->token};
            ++sequence_; // The owner callback can partially process the popup.
            UiCommandResult acknowledged;
            try { acknowledged = sources_.tech_tree_processed(*event); }
            catch (const std::exception &error) {
                return {UiCommandStatus::unavailable,
                    std::string("BNW technology tree close is uncertain; do not retry: ") +
                        error.what()};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "BNW technology tree close is uncertain; do not retry"};
            }
            if (acknowledged.status != UiCommandStatus::performed)
                return {UiCommandStatus::unavailable,
                    "BNW technology tree close was not confirmed; do not retry this event"};
            try {
                const auto current = sources_.active_tech_tree_event();
                if (current && current->player_id == event->player_id &&
                    current->token == event->token)
                    return {UiCommandStatus::unavailable,
                        "BNW technology tree popup acknowledgement is not observable"};
                if (current)
                    return {UiCommandStatus::unavailable,
                        "A new BNW technology tree popup needs owner reconciliation"};
            } catch (const std::exception &) {
                return {UiCommandStatus::unavailable,
                    "BNW technology tree popup acknowledgement is uncertain"};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "BNW technology tree popup acknowledgement is uncertain"};
            }
            if (sources_.tech_tree_local_dequeue_observed) {
                try {
                    if (!sources_.tech_tree_local_dequeue_observed(*event) ||
                        screens_.visible("TechTree") ||
                        screens_.focused_screen() == std::optional<std::string>{"TechTree"})
                        return {UiCommandStatus::unavailable,
                            "TechTree local dequeue/parent/source owners are uncertain; do not retry"};
                    return {UiCommandStatus::navigation, "Observed SAME-controller local TechTree dequeue"};
                } catch (...) {
                    return {UiCommandStatus::unavailable,
                        "TechTree local post-ack proof failed; do not retry"};
                }
            }
            const auto closed = screens_.dispatch(UiCloseScreen{"TechTree"}, frame.snapshot);
            return closed.status == UiCommandStatus::navigation ? closed
                : UiCommandResult{UiCommandStatus::unavailable,
                    "BNW technology tree acknowledged, but local focus close failed"};
        }
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<ProgressionOpenTechnologyTree>()) {
            const auto *popup = frame.snapshot.find<ProgressionTechPopupState>();
            if (gameplay->source_id() != "TechPopup" ||
                screens_.focused_screen() != std::optional<std::string>{"TechPopup"} ||
                !screens_.registry().find("TechTree") || !popup ||
                !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
                popup->player_id != *frame.snapshot.active_player_id() ||
                !popup->token || action->event_token != popup->token ||
                !frame.technology_tree_transition_ready ||
                !sources_.tech_popup_open_tree ||
                attempted_tech_tree_event_ ==
                    std::optional<std::pair<std::int64_t, std::uint64_t>>{
                        std::pair{popup->player_id, popup->token}})
                return {UiCommandStatus::unavailable,
                        "Active BNW technology popup transition is unavailable"};
            attempted_tech_tree_event_ = std::pair{popup->player_id, popup->token};
            ++sequence_; // No second dispatch of a potentially partial event.
            const ProgressionTechPopupEvent event{popup->player_id, popup->mode,
                popup->steal_target_player, popup->token};
            UiCommandResult acknowledged;
            try { acknowledged = sources_.tech_popup_open_tree(event); }
            catch (const std::exception &error) {
                return {UiCommandStatus::unavailable,
                    std::string("BNW popup transition is uncertain; do not retry: ") +
                        error.what()};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "BNW popup transition is uncertain; do not retry"};
            }
            if (acknowledged.status != UiCommandStatus::performed)
                return {UiCommandStatus::unavailable,
                    "BNW popup transition was not confirmed; do not retry this event"};
            const auto closed = screens_.dispatch(UiCloseScreen{"TechPopup"}, frame.snapshot);
            if (closed.status != UiCommandStatus::navigation)
                return {UiCommandStatus::unavailable,
                    "BNW popup acknowledged, but local screen close failed"};
            const auto opened = screens_.dispatch(UiOpenScreen{"TechTree"}, frame.snapshot);
            return opened.status == UiCommandStatus::navigation ? opened
                : UiCommandResult{UiCommandStatus::unavailable,
                    "BNW popup acknowledged, but local TechTree open failed"};
        }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "TechPopup") {
        const auto *popup = frame.snapshot.find<ProgressionTechPopupState>();
        if (!popup || !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
            popup->player_id != *frame.snapshot.active_player_id() ||
            !popup->token)
            return {UiCommandStatus::unavailable,
                    "Active BNW technology choice event is unavailable"};
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "GoodyHutPopup") {
        const auto *queue = frame.snapshot.find<PopupFrame>();
        const PopupEvent *event = nullptr;
        if (queue)
            for (const auto &candidate : queue->events)
                if (candidate.context_id == "GoodyHutPopup") {
                    event = &candidate;
                    break;
                }
        if (!event || !valid_goody_hut_reward_owner(frame.snapshot, *event))
            return {UiCommandStatus::unavailable,
                "Current goody hut reward popup event is unavailable"};
        const auto &owner = *event->goody_hut_reward;
        if (attempted_goody_hut_close_ ==
            std::optional<std::pair<std::int64_t, std::uint64_t>>{
                std::pair{owner.player_id, owner.token}})
            return {UiCommandStatus::unavailable,
                "Goody hut reward event was already processed or attempted"};
        if (sources_.active_goody_hut_reward) {
            try {
                const auto current = sources_.active_goody_hut_reward();
                if (!current || current->player_id != owner.player_id ||
                    current->turn != owner.turn ||
                    current->owner_sequence != owner.owner_sequence ||
                    current->token != owner.token ||
                    current->event_id != owner.event_id ||
                    current->goody_type != owner.goody_type ||
                    current->reward_amount != owner.reward_amount ||
                    current->selected_source_path != owner.selected_source_path)
                    return {UiCommandStatus::unavailable,
                        "Native goody hut reward changed before opening"};
            } catch (...) {
                return {UiCommandStatus::unavailable,
                    "Native goody hut reward cannot be verified"};
            }
        }
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "ChooseIdeology") {
        const auto *event = frame.snapshot.find<ProgressionIdeologyPopupEvent>();
        if (!event || !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
            event->player_id != *frame.snapshot.active_player_id() || !event->token)
            return {UiCommandStatus::unavailable,
                    "Active BNW ideology choice event is unavailable"};
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "ChooseReligion") {
        const auto *events = frame.snapshot.find<PopupFrame>();
        const PopupEvent *event = nullptr;
        if (events)
            for (const auto &candidate : events->events)
                if (candidate.context_id == "ChooseReligion") {
                    event = &candidate;
                    break;
                }
        if (!event || !valid_choose_religion_owner(frame.snapshot, *event))
            return {UiCommandStatus::unavailable,
                "Active BNW religion popup owner event is unavailable"};
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "ChooseArchaeology") {
        const auto *events = frame.snapshot.find<PopupFrame>();
        const PopupEvent *event = nullptr;
        if (events)
            for (const auto &candidate : events->events)
                if (candidate.context_id == "ChooseArchaeology") {
                    event = &candidate;
                    break;
                }
        if (!event || !valid_choose_archaeology_owner(frame.snapshot, *event))
            return {UiCommandStatus::unavailable,
                "Active BNW archaeology popup owner event is unavailable"};
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "ChoosePantheon") {
        const auto *events = frame.snapshot.find<PopupFrame>();
        const PopupEvent *event = nullptr;
        if (events)
            for (const auto &candidate : events->events)
                if (candidate.context_id == "ChoosePantheon") {
                    event = &candidate;
                    break;
                }
        if (!event || !valid_choose_pantheon_owner(frame.snapshot, *event))
            return {UiCommandStatus::unavailable,
                "Active BNW pantheon popup owner event is unavailable"};
    }
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<ReligionUiAction>()) {
            const auto *events = frame.snapshot.find<PopupFrame>();
            const PopupEvent *event = nullptr;
            if (events)
                for (const auto &candidate : events->events)
                    if (candidate.context_id == "ChooseReligion") {
                        event = &candidate;
                        break;
                    }
            if (gameplay->source_id() != "ChooseReligion" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"ChooseReligion"} ||
                !event || !valid_choose_religion_owner(frame.snapshot, *event) ||
                action->owner_sequence != events->owner_sequence ||
                action->event_id != event->event_id)
                return {UiCommandStatus::invalid,
                    "Religion symbol owner or frame is stale"};
            if (action->kind == ReligionUiActionKind::found_unavailable)
                return {UiCommandStatus::unavailable,
                    "Religion name, beliefs and GameCore confirmation are unavailable"};
            if (action->kind != ReligionUiActionKind::select_symbol) {
                const auto *selected =
                    frame.snapshot.find<ReligionSelectionView>();
                const auto *geometry = event->choose_religion->name_geometry
                    ? &*event->choose_religion->name_geometry : nullptr;
                if (!selected || !religion_selection_ ||
                    selected->religion_id != action->religion_id ||
                    religion_selection_->religion_id != action->religion_id ||
                    !geometry || geometry->viewport_width != viewport_width_ ||
                    geometry->viewport_height != viewport_height_ ||
                    !valid_religion_name_geometry(*geometry))
                    return {UiCommandStatus::unavailable,
                        "Current measured religion name controls are unavailable"};
                if (action->kind == ReligionUiActionKind::open_name) {
                    if (religion_selection_->editing_name)
                        return {UiCommandStatus::unavailable,
                            "Religion name dialog is already open"};
                    religion_selection_->editing_name = true;
                    religion_selection_->draft = religion_selection_->name;
                    religion_selection_->name_error = false;
                } else {
                    if (!religion_selection_->editing_name)
                        return {UiCommandStatus::invalid,
                            "Religion name dialog is not current"};
                    if (action->kind == ReligionUiActionKind::cancel_name) {
                        religion_selection_->editing_name = false;
                        religion_selection_->draft = religion_selection_->name;
                        religion_selection_->name_error = false;
                        popup_text_.reset();
                    } else if (action->kind ==
                               ReligionUiActionKind::default_name) {
                        religion_selection_->draft =
                            religion_selection_->default_name;
                        religion_selection_->name_error = false;
                        if (popup_text_)
                            popup_text_->text = religion_selection_->draft;
                    } else if (action->kind ==
                               ReligionUiActionKind::accept_name) {
                        if (!valid_popup_name(religion_selection_->draft, 16)) {
                            religion_selection_->name_error = true;
                            ++sequence_;
                            return {UiCommandStatus::unavailable,
                                "Enter a nonempty valid religion name"};
                        }
                        religion_selection_->name = religion_selection_->draft;
                        religion_selection_->editing_name = false;
                        religion_selection_->name_error = false;
                        popup_text_.reset();
                    } else return {UiCommandStatus::invalid,
                        "Unknown religion name action"};
                }
                ++sequence_;
                return {UiCommandStatus::navigation, {}};
            }
            if (action->kind != ReligionUiActionKind::select_symbol ||
                event->choose_religion->mode != ChooseReligionMode::founding)
                return {UiCommandStatus::unavailable,
                    "Religion symbol selection is unavailable in this mode"};
            if (religion_selection_ && religion_selection_->editing_name)
                return {UiCommandStatus::unavailable,
                    "Religion name dialog owns input focus"};
            const auto *view = frame.snapshot.find<PopupScrollView>();
            if (!view || view->player_id != events->player_id ||
                view->context_id != "ChooseReligion" ||
                view->event_id != event->event_id ||
                view->first_visible >= event->choices.size())
                return {UiCommandStatus::unavailable,
                    "Current religion symbol strip is unavailable"};
            const auto first = view->first_visible;
            const auto last = std::min(first + std::size_t{13},
                                       event->choices.size());
            const auto choice = std::find_if(event->choices.begin() + first,
                event->choices.begin() + last,
                [&](const PopupChoice &candidate) {
                    return candidate.id == action->religion_id &&
                        candidate.available &&
                        candidate.icon_atlas == "RELIGION_ATLAS_WHITE" &&
                        candidate.portrait_index >= 0 &&
                        candidate.portrait_index < 15;
                });
            if (choice == event->choices.begin() + last ||
                (religion_selection_ &&
                 religion_selection_->religion_id == action->religion_id))
                return {UiCommandStatus::unavailable,
                    "Current religion symbol is unavailable"};
            const auto &owner = *event->choose_religion;
            religion_selection_ = ReligionSelectionView{owner.player_id,
                owner.turn, owner.owner_sequence, event->event_id,
                action->religion_id, owner.selected_source_path,
                viewport_width_, viewport_height_, choice->label,
                choice->label, choice->label, false, false};
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<PantheonUiAction>()) {
            const auto *events = frame.snapshot.find<PopupFrame>();
            const PopupEvent *event = nullptr;
            if (events)
                for (const auto &candidate : events->events)
                    if (candidate.context_id == "ChoosePantheon") {
                        event = &candidate;
                        break;
                    }
            if (gameplay->source_id() != "ChoosePantheon" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"ChoosePantheon"} ||
                !event || !valid_choose_pantheon_owner(frame.snapshot, *event) ||
                action->owner_sequence != events->owner_sequence ||
                action->event_id != event->event_id)
                return {UiCommandStatus::invalid,
                    "Pantheon confirmation owner or frame is stale"};
            const auto &owner = *event->choose_pantheon;
            if (action->kind == PantheonUiActionKind::confirm_unavailable)
                return {UiCommandStatus::unavailable,
                    "Pantheon GameCore confirmation is unavailable"};
            if (action->kind == PantheonUiActionKind::cancel_confirmation) {
                const auto *view = frame.snapshot.find<PantheonConfirmView>();
                if (!view || !pantheon_confirm_ ||
                    view->belief_id != action->belief_id ||
                    view->belief_id != pantheon_confirm_->belief_id ||
                    view->owner_sequence != owner.owner_sequence ||
                    view->selected_source_path != owner.selected_source_path)
                    return {UiCommandStatus::invalid,
                        "Pantheon confirmation is no longer current"};
                pantheon_confirm_.reset();
                ++sequence_;
                return {UiCommandStatus::navigation, {}};
            }
            if (action->kind != PantheonUiActionKind::select_belief ||
                pantheon_confirm_ || owner.viewport_width != viewport_width_ ||
                owner.viewport_height != viewport_height_ ||
                !valid_pantheon_visible_rows(*event) ||
                !valid_pantheon_confirm_geometry(owner))
                return {UiCommandStatus::unavailable,
                    "Current pantheon row or confirmation geometry is unavailable"};
            const auto choice = std::find_if(event->choices.begin(),
                event->choices.end(), [&](const PopupChoice &value) {
                    return value.id == action->belief_id && value.available;
                });
            const auto row = std::find_if(owner.visible_choice_bounds.begin(),
                owner.visible_choice_bounds.end(), [&](const auto &value) {
                    return value.first == action->belief_id;
                });
            const auto text = owner.confirm_text_by_choice.find(action->belief_id);
            const WorldViewRect clip{0, 143, 418, viewport_height_ - 198};
            if (choice == event->choices.end() ||
                row == owner.visible_choice_bounds.end() ||
                text == owner.confirm_text_by_choice.end() ||
                text->second.empty() || row->second.width <= 0 ||
                row->second.height < 40 ||
                row->second.x < clip.x || row->second.y < clip.y ||
                static_cast<std::int64_t>(row->second.x) + row->second.width >
                    clip.x + clip.width ||
                static_cast<std::int64_t>(row->second.y) + row->second.height >
                    clip.y + clip.height)
                return {UiCommandStatus::unavailable,
                    "Current pantheon belief row is unavailable"};
            pantheon_confirm_ = PantheonConfirmView{owner.player_id, owner.turn,
                owner.owner_sequence, event->event_id, action->belief_id,
                owner.selected_source_path, viewport_width_, viewport_height_};
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *decision = gameplay->as<PopupDecision>(); decision &&
            decision->context_id == "ChooseReligion")
            return {UiCommandStatus::unavailable,
                "Religion name, beliefs and confirmation are unavailable"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *decision = gameplay->as<PopupDecision>(); decision &&
            decision->context_id == "ChooseArchaeology")
            return {UiCommandStatus::unavailable,
                "Archaeology confirmation and world effects are unavailable"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *decision = gameplay->as<PopupDecision>(); decision &&
            decision->context_id == "ChoosePantheon")
            return {UiCommandStatus::unavailable,
                "Pantheon confirmation and GameCore effects are unavailable"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (const auto *action = gameplay->as<IdeologyUiAction>()) {
            const auto *event = frame.snapshot.find<ProgressionIdeologyPopupEvent>();
            const auto *evidence = frame.snapshot.find<IdeologyLayoutEvidence>();
            const auto *source = sources_.progression[static_cast<std::size_t>(
                ProgressionScreen::ideology)];
            const auto *progression = progression_for_active_player(frame.snapshot);
            if (gameplay->source_id() != "ChooseIdeology" ||
                screens_.focused_screen() != std::optional<std::string>{"ChooseIdeology"} ||
                !event || !evidence || !source || !progression ||
                !frame.ideology_view || !ideology_view_ ||
                !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
                action->event_token != event->token ||
                event->player_id != *frame.snapshot.active_player_id() ||
                evidence->player_id != event->player_id ||
                evidence->turn != *frame.snapshot.turn() ||
                evidence->frame != frame.snapshot.frame() ||
                evidence->event_token != event->token ||
                evidence->selected_source_path != source->selected_source_path ||
                frame.ideology_view->player_id != ideology_view_->player_id ||
                frame.ideology_view->turn != ideology_view_->turn ||
                frame.ideology_view->event_token != ideology_view_->event_token ||
                frame.ideology_view->selected_source_path !=
                    ideology_view_->selected_source_path ||
                frame.ideology_view->overlay != ideology_view_->overlay ||
                frame.ideology_view->branch_id != ideology_view_->branch_id ||
                frame.ideology_view->tenet_collapsed !=
                    ideology_view_->tenet_collapsed)
                return {UiCommandStatus::unavailable,
                        "Live BNW ideology overlay state is unavailable"};
            if (action->kind == IdeologyUiActionKind::toggle_tenet_level) {
                if (ideology_view_->overlay != LiveUiFrame::IdeologyOverlay::tenets ||
                    action->branch_id != ideology_view_->branch_id ||
                    action->level < 1 || action->level > 3 ||
                    !evidence->tenet_help)
                    return {UiCommandStatus::unavailable,
                            "Current BNW tenet header is unavailable"};
                const auto layout = build_ideology_layout(*source, frame.snapshot,
                    frame.ideology_view->viewport_width,
                    frame.ideology_view->viewport_height, &*frame.ideology_view);
                const bool authored_hit = std::any_of(layout.hits.begin(),
                    layout.hits.end(), [&](const auto &hit) {
                        const auto *payload = std::get_if<UiGameplayCommand>(&hit.command);
                        const auto *candidate = payload
                            ? payload->template as<IdeologyUiAction>() : nullptr;
                        return hit.enabled && candidate &&
                            candidate->kind == IdeologyUiActionKind::toggle_tenet_level &&
                            candidate->branch_id == action->branch_id &&
                            candidate->event_token == action->event_token &&
                            candidate->level == action->level;
                    });
                if (!authored_hit)
                    return {UiCommandStatus::unavailable,
                            "Selected BNW tenet header geometry is unresolved"};
                auto &collapsed = ideology_view_->tenet_collapsed[
                    static_cast<std::size_t>(action->level - 1)];
                collapsed = !collapsed;
            } else if (action->kind == IdeologyUiActionKind::close_overlay) {
                if (ideology_view_->overlay == LiveUiFrame::IdeologyOverlay::none ||
                    action->branch_id != ideology_view_->branch_id)
                    return {UiCommandStatus::invalid,
                            "Ideology overlay close target is stale"};
                ideology_view_->overlay = LiveUiFrame::IdeologyOverlay::none;
                ideology_view_->branch_id = -1;
                ideology_view_->tenet_offset = 0;
            } else {
                if (action->kind != IdeologyUiActionKind::select_branch &&
                    action->kind != IdeologyUiActionKind::view_tenets)
                    return {UiCommandStatus::invalid,
                            "Unknown ideology overlay action"};
                const auto owner_branch = std::find_if(evidence->branches.begin(),
                    evidence->branches.end(), [&](const auto &branch) {
                        return branch.branch_id == action->branch_id;
                    });
                const auto active_branch = std::find_if(progression->branches.begin(),
                    progression->branches.end(), [&](const auto &branch) {
                        return branch.id == action->branch_id && branch.ideology;
                    });
                if (ideology_view_->overlay != LiveUiFrame::IdeologyOverlay::none ||
                    owner_branch == evidence->branches.end() ||
                    active_branch == progression->branches.end())
                    return {UiCommandStatus::unavailable,
                            "Selected ideology branch is unavailable"};
                ideology_view_->branch_id = action->branch_id;
                ideology_view_->overlay = action->kind == IdeologyUiActionKind::view_tenets
                    ? LiveUiFrame::IdeologyOverlay::tenets
                    : LiveUiFrame::IdeologyOverlay::confirmation;
            }
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "ProductionPopup") {
        const auto* city=::civ5::ui::selected_city(frame.snapshot);
        if(!city||!frame.snapshot.active_player_id()||city->owner!=*frame.snapshot.active_player_id()||
           !city->queue_resolved||!sources_.production_backend||!sources_.production_popup_source||
           (screens_.focused_screen()!=std::optional<std::string>{"CityView"}&&
            screens_.focused_screen()!=std::optional<std::string>{"ProductionPopup"}))
            return {UiCommandStatus::unavailable,"ProductionPopup needs the actual selected city/context/backend"};
    }
    if (const auto *open = std::get_if<UiOpenScreen>(&command);
        open && open->id == "CityView" && !::civ5::ui::selected_city(frame.snapshot))
        return {UiCommandStatus::unavailable, "No selected owned live city"};
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command)) {
        if (const auto *action = gameplay->as<WorldViewAction>();
            action && action->kind == WorldViewActionKind::open_city) {
            const bool from_banner = gameplay->source_id() == "CityBannerManager" &&
                !screens_.focused_screen();
            const bool from_list = gameplay->source_id() == "CityList" &&
                screens_.focused_screen() == std::optional<std::string>{"CityList"};
            if ((!from_banner && !from_list) || action->id < 0 ||
                !screens_.registry().find("CityView"))
                return {UiCommandStatus::invalid, "Live city source cannot open CityView"};
            const auto *world = frame.snapshot.find<WorldViewState>();
            const auto *cities = frame.snapshot.find<CityScreenFrame>();
            if (!world || !cities || !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                cities->player_id != world->player_id)
                return {UiCommandStatus::unavailable, "Owned live city is unavailable"};
            const auto banner = std::find_if(world->city_banners.begin(),
                world->city_banners.end(), [&](const WorldViewCityBanner &item) {
                    return item.id == action->id && item.visible &&
                        item.owned_by_active_player;
                });
            const auto city = std::find_if(cities->cities.begin(), cities->cities.end(),
                [&](const CityScreenCity &item) {
                    return item.id == action->id && item.owner == cities->player_id &&
                        item.name && !item.name->empty() && item.population > 0 &&
                        (!from_banner || (banner != world->city_banners.end() &&
                            *item.name == banner->label));
                });
            if ((from_banner && banner == world->city_banners.end()) ||
                city == cities->cities.end())
                return {UiCommandStatus::unavailable, from_banner
                    ? "City banner does not match the owned live city"
                    : "City list entry does not match the owned live city"};
            const auto result = screens_.dispatch(UiOpenScreen{"CityView"}, frame.snapshot);
            if (result.status == UiCommandStatus::navigation) {
                set_selected_city(std::pair{cities->player_id, action->id});
                ++sequence_;
            }
            return result;
        }
        if (const auto *edit = gameplay->as<PopupTextEdit>()) {
            const auto *view = frame.snapshot.find<PopupTextView>();
            if (gameplay->source_id() != edit->context_id ||
                screens_.focused_screen() !=
                    std::optional<std::string>{edit->context_id} ||
                !view || !popup_text_ || !frame.snapshot.active_player_id() ||
                view->player_id != *frame.snapshot.active_player_id() ||
                view->context_id != edit->context_id ||
                view->event_id != edit->event_id ||
                popup_text_->player_id != view->player_id ||
                popup_text_->event_id != view->event_id ||
                popup_text_->text != edit->previous_text ||
                view->text != edit->previous_text)
                return {UiCommandStatus::invalid, "Popup text event, player or frame is stale"};
            const bool religion_name = edit->context_id == "ChooseReligion";
            if (religion_name &&
                (!religion_selection_ || !religion_selection_->editing_name ||
                 religion_selection_->event_id != edit->event_id ||
                 religion_selection_->draft != edit->previous_text))
                return {UiCommandStatus::invalid,
                    "Religion name draft or owner is stale"};
            if (!edit->text.empty() &&
                !valid_popup_name(edit->text, religion_name ? 16 : 15))
                return {UiCommandStatus::invalid, "Popup name exceeds the authored limit or contains invalid text"};
            if (edit->text == popup_text_->text)
                return {UiCommandStatus::unavailable, "Popup name is unchanged"};
            popup_text_->text = edit->text;
            if (religion_name) {
                religion_selection_->draft = edit->text;
                religion_selection_->name_error = false;
            }
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
        if (const auto *decision = gameplay->as<PopupDecision>(); decision &&
            (decision->context_id == "SetCityName" ||
             decision->context_id == "SetUnitName")) {
            const auto *view = frame.snapshot.find<PopupTextView>();
            if (gameplay->source_id() != decision->context_id ||
                screens_.focused_screen() !=
                    std::optional<std::string>{decision->context_id} ||
                !view || !frame.snapshot.active_player_id() ||
                view->player_id != *frame.snapshot.active_player_id() ||
                view->context_id != decision->context_id ||
                view->event_id != decision->event_id ||
                !decision->entered_text ||
                *decision->entered_text != view->text)
                return {UiCommandStatus::invalid, "Popup name decision is stale"};
            if (!valid_popup_name(view->text))
                return {UiCommandStatus::unavailable, "Enter a valid name"};
        }
        if (const auto *choice = gameplay->as<CityChooseProduction>()) {
            const auto *choices = frame.snapshot.find<ProductionPopupChoices>();
            if (gameplay->source_id() != "ProductionPopup" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"ProductionPopup"} ||
                !choices || !production_scroll_ ||
                !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
                choices->backend_owner != sources_.production_backend ||
                choices->player_id != *frame.snapshot.active_player_id() ||
                choices->turn != *frame.snapshot.turn() ||
                choices->city_id != choice->city_id ||
                production_scroll_->player_id != choices->player_id ||
                production_scroll_->city_id != choices->city_id ||
                production_scroll_->selected_source_path !=
                    choices->selected_source_path ||
                production_scroll_->first_visible != choices->first_visible ||
                !choices->visible_rows)
                return {UiCommandStatus::unavailable,
                        "Production popup choice source or frame is unavailable"};
            const auto found = std::find_if(choices->entries.begin(), choices->entries.end(),
                [&](const ProductionPopupChoice &entry) {
                    return entry.order_type == choice->order_type &&
                           entry.item_id == choice->item_id;
                });
            const auto index = static_cast<std::size_t>(
                std::distance(choices->entries.begin(), found));
            if (found == choices->entries.end() ||
                index < choices->first_visible ||
                index - choices->first_visible >= choices->visible_rows ||
                found->availability.status != UiCommandStatus::ready)
                return {UiCommandStatus::unavailable,
                        "Production choice is not available in this UI frame"};
        }
        if (const auto *scroll = gameplay->as<PopupScroll>()) {
            if (scroll->context_id == "ChooseArchaeology" ||
                scroll->context_id == "ChoosePantheon")
                return {UiCommandStatus::unavailable,
                    "Native popup scroll movement is unavailable"};
            if (scroll->context_id == "ChooseReligion" &&
                religion_selection_ && religion_selection_->editing_name)
                return {UiCommandStatus::unavailable,
                    "Religion name dialog owns input focus"};
            const auto *view = frame.snapshot.find<PopupScrollView>();
            const auto *events = frame.snapshot.find<PopupFrame>();
            if (!view || !events || !popup_scroll_ ||
                gameplay->source_id() != scroll->context_id ||
                screens_.focused_screen() != std::optional<std::string>{scroll->context_id} ||
                view->context_id != scroll->context_id ||
                view->event_id != scroll->event_id ||
                view->first_visible != scroll->first_visible ||
                popup_scroll_->context_id != scroll->context_id ||
                popup_scroll_->event_id != scroll->event_id ||
                popup_scroll_->player_id != view->player_id ||
                popup_scroll_->first_visible != view->first_visible ||
                events->player_id != view->player_id)
                return {UiCommandStatus::invalid, "Popup scroll source or frame is stale"};
            const auto event = std::find_if(events->events.begin(), events->events.end(),
                [&](const PopupEvent &value) {
                    return value.context_id == scroll->context_id &&
                           value.event_id == scroll->event_id;
                });
            if (event == events->events.end() ||
                event->choices.size() != popup_scroll_->choice_count)
                return {UiCommandStatus::invalid, "Popup choices changed"};
            if (scroll->direction == PopupScrollDirection::up) {
                if (!popup_scroll_->first_visible)
                    return {UiCommandStatus::unavailable, "Already at the first choice"};
                --popup_scroll_->first_visible;
            } else if (scroll->direction == PopupScrollDirection::down) {
                if (popup_scroll_->first_visible + 1 >= event->choices.size())
                    return {UiCommandStatus::unavailable, "Already at the last choice"};
                ++popup_scroll_->first_visible;
            } else return {UiCommandStatus::invalid, "Unknown popup scroll direction"};
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
        if (const auto *pedia = gameplay->as<SystemPediaAction>()) {
            if (gameplay->source_id() != "CivilopediaScreen" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"CivilopediaScreen"})
                return {UiCommandStatus::invalid, "Civilopedia does not own focus"};
            const auto *state = frame.snapshot.find<SystemScreenFrame>();
            if (!sources_.pedia_entries || !state ||
                !state->pedia_category_session_owned ||
                state->selected_pedia_id != selected_pedia_id_ ||
                state->pedia_page != pedia_page_ ||
                state->pedia_category_tab != pedia_category_tab_ ||
                state->pedia_search != pedia_search_ ||
                state->pedia_history != pedia_history_ ||
                state->pedia_history_index != pedia_history_index_ ||
                state->pedia_article_first_line != pedia_article_first_line_)
                return {UiCommandStatus::unavailable,
                    "Civilopedia has no matching live session owner"};
            auto updated = *state;
            const auto applied = apply_civilopedia_action(updated, *pedia);
            if (applied.status != UiCommandStatus::performed) return applied;
            if (updated.selected_pedia_id == state->selected_pedia_id &&
                updated.pedia_page == state->pedia_page &&
                updated.pedia_category_tab == state->pedia_category_tab &&
                updated.pedia_search == state->pedia_search &&
                updated.pedia_history == state->pedia_history &&
                updated.pedia_history_index == state->pedia_history_index &&
                updated.pedia_article_first_line == state->pedia_article_first_line)
                return {UiCommandStatus::unavailable,
                    "Civilopedia navigation state is unchanged"};
            SystemGameplayBackend local_backend;
            const auto result = screens_.dispatch(command, frame.snapshot, &local_backend);
            if (result.status != UiCommandStatus::performed) return result;
            selected_pedia_id_ = std::move(updated.selected_pedia_id);
            pedia_page_ = updated.pedia_page;
            pedia_category_tab_ = updated.pedia_category_tab;
            pedia_search_ = std::move(updated.pedia_search);
            pedia_history_ = std::move(updated.pedia_history);
            pedia_history_index_ = updated.pedia_history_index;
            pedia_article_first_line_ = updated.pedia_article_first_line;
            ++sequence_;
            return applied;
        }
        if (const auto *scroll = gameplay->as<ProductionPopupScroll>()) {
            const auto *choices = frame.snapshot.find<ProductionPopupChoices>();
            if (gameplay->source_id() != "ProductionPopup" ||
                screens_.focused_screen() !=
                    std::optional<std::string>{"ProductionPopup"} ||
                !choices || !production_scroll_ || !choices->visible_rows ||
                scroll->city_id != choices->city_id ||
                choices->backend_owner != sources_.production_backend ||
                production_scroll_->player_id != choices->player_id ||
                production_scroll_->city_id != choices->city_id ||
                production_scroll_->selected_source_path !=
                    choices->selected_source_path ||
                production_scroll_->first_visible != choices->first_visible)
                return {UiCommandStatus::invalid,
                        "Production popup scroll source or frame is stale"};
            const auto max_first = choices->entries.size() > choices->visible_rows
                ? choices->entries.size() - choices->visible_rows : 0;
            auto &first = production_scroll_->first_visible;
            if (scroll->direction == ProductionPopupScrollDirection::up) {
                if (!first)
                    return {UiCommandStatus::unavailable,
                            "Production popup is already at the first choice"};
                --first;
            } else if (scroll->direction == ProductionPopupScrollDirection::down) {
                if (first >= max_first)
                    return {UiCommandStatus::unavailable,
                            "Production popup is already at the last choice"};
                ++first;
            } else return {UiCommandStatus::invalid,
                           "Unknown production popup scroll direction"};
            ++sequence_;
            return {UiCommandStatus::navigation, {}};
        }
        if (const auto *world_action = gameplay->as<WorldViewAction>();
            world_action && world_action->kind == WorldViewActionKind::end_turn) {
            const auto *world = frame.snapshot.find<WorldViewState>();
            if (!world || !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                !world->end_turn_available)
                return {UiCommandStatus::unavailable,
                        world && !world->end_turn_unavailable_reason.empty()
                            ? world->end_turn_unavailable_reason
                            : "Live End Turn handler is not connected"};
        }
        if (const auto *world_action = gameplay->as<WorldViewAction>();
            world_action && world_action->kind == WorldViewActionKind::unit_action) {
            const auto *world = frame.snapshot.find<WorldViewState>();
            if (gameplay->source_id() != "UnitPanel" || !world ||
                !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                !world->selected_unit ||
                world->selected_unit->id != world_action->related_id)
                return {UiCommandStatus::unavailable,
                        "Selected unit is unavailable in this UI frame"};
            const auto &actions = world->selected_unit->actions;
            const auto found = std::find_if(actions.begin(), actions.end(),
                [&](const WorldViewActionItem &action) {
                    return action.id == world_action->id;
                });
            if (found == actions.end() || !found->available)
                return {UiCommandStatus::unavailable,
                        "Selected unit action is unavailable in this UI frame"};
        }
        if (const auto *world_action = gameplay->as<WorldViewAction>();
            world_action && world_action->kind == WorldViewActionKind::notification) {
            const auto *world = frame.snapshot.find<WorldViewState>();
            if (gameplay->source_id() != "ActionInfoPanel" || !world ||
                !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                std::none_of(world->notifications.begin(), world->notifications.end(),
                    [&](const WorldViewNotification &notification) {
                        return notification.id == world_action->id;
                    }))
                return {UiCommandStatus::unavailable,
                        "Notification is absent from the current live UI queue"};
        }
        if (const auto *world_action = gameplay->as<WorldViewAction>();
            world_action && world_action->kind == WorldViewActionKind::info_corner) {
            const auto *world = frame.snapshot.find<WorldViewState>();
            if (gameplay->source_id() != "InfoCorner" || !world ||
                !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                std::none_of(world->info_corner.begin(), world->info_corner.end(),
                    [&](const WorldViewActionItem &item) {
                        return item.id == world_action->id && item.available &&
                            item.target_screen.empty();
                    }))
                return {UiCommandStatus::unavailable,
                        "InfoCorner action is absent or disabled in the current frame"};
        }
        if (const auto *world_action = gameplay->as<WorldViewAction>();
            world_action && world_action->kind == WorldViewActionKind::select_unit) {
            const auto *world = frame.snapshot.find<WorldViewState>();
            if (gameplay->source_id() != "UnitFlagManager" || !world ||
                !frame.snapshot.active_player_id() ||
                world->player_id != *frame.snapshot.active_player_id() ||
                std::none_of(world->unit_flags.begin(), world->unit_flags.end(),
                    [&](const WorldViewIcon &flag) {
                        return flag.id == world_action->id && flag.visible;
                    }))
                return {UiCommandStatus::unavailable,
                        "Unit flag is absent from the current projected frame"};
        }
    }
    const auto previous_focus = screens_.focused_screen();
    // Production uses its real retained backend; research/system commands
    // continue through the existing gameplay backend. No detached aggregate.
    UiGameplayBackend* dispatch_backend=backend_;
    if(const auto* gameplay=std::get_if<UiGameplayCommand>(&command);
       gameplay&&gameplay->source_id()=="ProductionPopup"&&gameplay->as<CityChooseProduction>())
        dispatch_backend=sources_.production_backend;
    auto result = screens_.dispatch(command, frame.snapshot, dispatch_backend);
    if (result.status == UiCommandStatus::navigation && (sources_.audio_bridge || sources_.civis_audio_transition))
        if (const auto *open = std::get_if<UiOpenScreen>(&command);
            open && previous_focus != std::optional<std::string>{open->id} &&
            screens_.focused_screen() == std::optional<std::string>{open->id})
            if (const auto *events = frame.snapshot.find<PopupFrame>();
                events && frame.snapshot.active_player_id() &&
                events->player_id == *frame.snapshot.active_player_id()) {
                const auto event = std::find_if(events->events.begin(), events->events.end(),
                    [&](const PopupEvent &value) {
                        return value.context_id == open->id && value.event_id >= 0;
                    });
                if (event != events->events.end() && event->audio_cue)
                    emit_audio(UiAudioKind::popup, *event->audio_cue);
            }
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "ProductionPopup")
            production_scroll_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "SocialPolicyPopup")
            policy_scroll_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "ChooseIdeology")
            ideology_view_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "ChoosePantheon")
            pantheon_confirm_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "ChooseReligion")
            religion_selection_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && close->id == "CityView")
            set_selected_city(std::nullopt);
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && popup_scroll_ && close->id == popup_scroll_->context_id)
            popup_scroll_.reset();
    if (result.status == UiCommandStatus::navigation)
        if (const auto *close = std::get_if<UiCloseScreen>(&command);
            close && popup_text_ && close->id == popup_text_->context_id)
            popup_text_.reset();
    if (result.status == UiCommandStatus::performed)
        if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
            if (gameplay->source_id() == "ProductionPopup" &&
                gameplay->as<CityChooseProduction>() &&
                screens_.focused_screen() ==
                    std::optional<std::string>{"ProductionPopup"}) {
                (void)screens_.dispatch(UiCloseScreen{"ProductionPopup"},
                                        frame.snapshot, backend_);
                production_scroll_.reset();
            }
    if (result.status == UiCommandStatus::performed)
        if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
            if (gameplay->source_id() == "TechPopup" &&
                gameplay->as<ProgressionChooseTechnology>() &&
                screens_.focused_screen() == std::optional<std::string>{"TechPopup"}) {
                (void)screens_.dispatch(UiCloseScreen{"TechPopup"}, frame.snapshot, backend_);
            }
    if (result.status == UiCommandStatus::navigation ||
        result.status == UiCommandStatus::performed ||
        result.status == UiCommandStatus::pending_refresh ||
        result.status == UiCommandStatus::uncertain) ++sequence_;
    return result;
}

LiveUiAudioResult LiveUiHost::publish_civis_audio_cue(UiAudioKind kind,std::string_view borrowed_id)const {
    const std::string id(borrowed_id);
    if(!sources_.civis_audio_transition)return {};
    if(plot_yield_ui_thread_!=std::this_thread::get_id())
        throw std::logic_error("Civis audio actual UI publisher foreign thread");
    const auto storage=owned_plot_yield_ui_sources_;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    const auto self=selected_owner_lifetime_.lock();
    if(!self||self.get()!=this||!city_selection_control_)
        throw std::logic_error("Civis audio actual canonical selected UI publisher retired");
    city_selection_control_->require_current();
    if(id.empty()||(kind!=UiAudioKind::popup&&kind!=UiAudioKind::notification))
        throw std::logic_error("Civis audio actual published cue invalid");
    if(civis_audio_source_sequence_==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Civis audio actual publisher sequence exhausted");
    const UiAuthoredAudioCue cue{++civis_audio_source_sequence_,id};
    const auto result=sources_.civis_audio_transition(cue.sequence,kind,cue.id);
    if(sources_.audio_observer)sources_.audio_observer(kind,cue,result);
    return result;
}
void LiveUiHost::emit_audio(UiAudioKind kind, const UiAuthoredAudioCue &cue) {
    if(sources_.civis_audio_transition){
        if(!cue.sequence||cue.id.empty())return;
        auto& previous=last_authored_audio_cue_[kind];
        if(cue.sequence<=previous)return;
        previous=cue.sequence; //Real ingress reservation; failed callbacks do not replay a frame.
        (void)publish_civis_audio_cue(kind,cue.id);return;
    }
    if(!sources_.audio_bridge)return;
    const auto result=sources_.audio_bridge->transition(cue.sequence,kind,cue.id);
    if(sources_.audio_observer)sources_.audio_observer(kind,cue,result);
}

UiCommandResult LiveUiHost::dispatch(const UiCommand &command, const LiveUiFrame &frame) {
    return route(command, frame);
}

UiCommandResult LiveUiHost::search_civilopedia(
    std::string_view query, const LiveUiFrame &frame) {
    return route(civilopedia_search_command(query), frame);
}

LiveUiInputResult LiveUiHost::wheel_civilopedia_article(const LiveUiFrame &frame,
    std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
    WorldViewRect scroll_panel) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() !=
        std::optional<std::string>{"CivilopediaScreen"}) return {};
    if (!contains(scroll_panel, x, y))
        return {true, {UiCommandStatus::invalid,
                       "Focused Civilopedia owns wheel input"}};
    if (!wheel_delta)
        return {true, {UiCommandStatus::unavailable, "No scroll movement"}};
    return {true, route(UiGameplayCommand::make("CivilopediaScreen",
        SystemPediaAction{wheel_delta > 0 ?
            SystemPediaActionKind::article_scroll_up :
            SystemPediaActionKind::article_scroll_down, {}}), frame)};
}

UiCommandResult LiveUiHost::edit_popup_text(
    std::string_view text, const LiveUiFrame &frame) {
    const auto *view = frame.snapshot.find<PopupTextView>();
    if (!view) return {UiCommandStatus::invalid, "No focused popup EditBox"};
    return route(UiGameplayCommand::make(view->context_id,
        PopupTextEdit{view->context_id, view->event_id, view->text,
                      std::string(text)}), frame);
}

UiCommandResult LiveUiHost::dispatch_top_panel(TopPanelAction action,
                                               const LiveUiFrame &frame) {
    if (!current(frame)) return {UiCommandStatus::invalid, "UI frame is stale"};
    if (const auto focused = screens_.focused_screen()) {
        const auto *screen = screens_.registry().find(*focused);
        if (screen && screen->modal)
            return {UiCommandStatus::invalid, "A modal UI screen owns input focus"};
    }
    const auto result = dispatch_top_panel_action(action, screens_, frame.snapshot);
    if (result.status == UiCommandStatus::navigation ||
        result.status == UiCommandStatus::performed ||
        result.status == UiCommandStatus::pending_refresh ||
        result.status == UiCommandStatus::uncertain) ++sequence_;
    return result;
}

LiveUiInputResult LiveUiHost::activate(const UiCommand &command, bool enabled,
                                       std::string_view reason, const LiveUiFrame &frame) {
    if (!enabled)
        return {true, {UiCommandStatus::unavailable,
                       reason.empty() ? "UI action is unavailable" : std::string(reason)}};
    return {true, route(command, frame)};
}

LiveUiInputResult LiveUiHost::click(const LiveUiFrame &frame, std::int32_t x,
                                    std::int32_t y,
                                    std::span<const LiveUiHit> measured_hits) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    const auto focused = screens_.focused_screen();
    const auto *focused_context = focused ? screens_.registry().find(*focused) : nullptr;
    const bool modal = focused_context && focused_context->modal;
    for (auto hit = measured_hits.rbegin(); hit != measured_hits.rend(); ++hit) {
        if (!contains(hit->bounds, x, y)) continue;
        const auto *source = screens_.registry().find(hit->context_id);
        if (!source || !screens_.visible(hit->context_id))
            return {true, {UiCommandStatus::invalid, "UI hit source is hidden or unknown"}};
        if (const auto *gameplay = std::get_if<UiGameplayCommand>(&hit->command);
            gameplay && gameplay->source_id() != hit->context_id)
            return {true, {UiCommandStatus::invalid, "UI hit and gameplay source disagree"}};
        if (modal && hit->context_id != *focused)
            return {true, {UiCommandStatus::invalid, "A modal UI screen owns input focus"}};
        return activate(hit->command, hit->enabled, hit->unavailable_reason, frame);
    }
    if (modal)
        return {true, {UiCommandStatus::invalid, "A modal UI screen owns input focus"}};
    if (frame.world_view) {
        if (const auto *hit = frame.world_view->hit(x, y))
            return activate(hit->command, hit->enabled, hit->unavailable_reason, frame);
    }
    return {};
}

LiveUiInputResult LiveUiHost::wheel_production_popup(
    const LiveUiFrame &frame, std::int32_t x, std::int32_t y,
    std::int32_t wheel_delta, WorldViewRect measured_scroll_panel) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() != std::optional<std::string>{"ProductionPopup"})
        return {};
    if (!wheel_delta)
        return {true, {UiCommandStatus::unavailable, "No scroll movement"}};
    if (!contains(measured_scroll_panel, x, y))
        return {true, {UiCommandStatus::invalid,
                       "A focused production popup owns wheel input"}};
    const auto *choices = frame.snapshot.find<ProductionPopupChoices>();
    if (!choices || !choices->visible_rows)
        return {true, {UiCommandStatus::unavailable,
                       "Selected production scroll layout is unavailable"}};
    return {true, route(city_scroll_production_popup(choices->city_id,
        wheel_delta > 0 ? ProductionPopupScrollDirection::up
                        : ProductionPopupScrollDirection::down), frame)};
}

LiveUiInputResult LiveUiHost::wheel_popup(const LiveUiFrame &frame,
    std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
    WorldViewRect scroll_panel) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() ==
            std::optional<std::string>{"ChooseArchaeology"} ||
        screens_.focused_screen() ==
            std::optional<std::string>{"ChoosePantheon"})
        return {true, {UiCommandStatus::unavailable,
            "Native popup scroll movement is unavailable"}};
    const auto *view = frame.snapshot.find<PopupScrollView>();
    if (!view || screens_.focused_screen() !=
        std::optional<std::string>{view->context_id}) return {};
    if (!wheel_delta)
        return {true, {UiCommandStatus::unavailable, "No scroll movement"}};
    if (!contains(scroll_panel, x, y))
        return {true, {UiCommandStatus::invalid,
                       "A focused popup owns wheel input"}};
    return {true, route(UiGameplayCommand::make(view->context_id,
        PopupScroll{view->context_id, view->event_id, view->first_visible,
                    wheel_delta > 0 ? PopupScrollDirection::up
                                    : PopupScrollDirection::down}), frame)};
}

LiveUiInputResult LiveUiHost::wheel_policy(const LiveUiFrame &frame,
    std::int32_t wheel_delta, const PolicyScrollRegion &region) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() != std::optional<std::string>{"SocialPolicyPopup"})
        return {};
    const auto *source = sources_.progression[static_cast<std::size_t>(
        ProgressionScreen::policies)];
    if (!source || !frame.policy_scroll || !policy_scroll_ ||
        frame.policy_scroll->player_id != policy_scroll_->player_id ||
        frame.policy_scroll->turn != policy_scroll_->turn ||
        frame.policy_scroll->selected_source_path != source->selected_source_path ||
        region.selected_source_path != source->selected_source_path ||
        region.max_offset < 0 || region.max_offset > 10000 ||
        std::find(source->control_ids.begin(), source->control_ids.end(),
                  region.panel_id) == source->control_ids.end())
        return {true, {UiCommandStatus::unavailable,
                       "Selected policy scroll state or control is unavailable"}};
    const auto old = frame.policy_scroll->offsets.find(region.panel_id);
    const auto offset = old == frame.policy_scroll->offsets.end() ? 0 : old->second;
    if (offset != region.offset)
        return {true, {UiCommandStatus::invalid, "Policy scroll geometry is stale"}};
    if (!wheel_delta)
        return {true, {UiCommandStatus::unavailable, "No scroll movement"}};
    // BNW SocialPolicyPopup.lua uses a 68-pixel vertical policy grid step.
    const auto next = std::clamp(offset + (wheel_delta > 0 ? -68 : 68),
                                 0, region.max_offset);
    if (next == offset)
        return {true, {UiCommandStatus::unavailable, "Policy panel is at its scroll edge"}};
    policy_scroll_->offsets[region.panel_id] = next;
    ++sequence_;
    return {true, {UiCommandStatus::navigation, {}}};
}

LiveUiInputResult LiveUiHost::wheel_tech_popup(const LiveUiFrame &frame,
    std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
    const TechPopupScrollRegion &region) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() != std::optional<std::string>{"TechPopup"})
        return {};
    const auto *source = sources_.progression[static_cast<std::size_t>(
        ProgressionScreen::technology_popup)];
    const auto *event = frame.snapshot.find<ProgressionTechPopupState>();
    const auto *view = frame.tech_popup_scroll ? &*frame.tech_popup_scroll : nullptr;
    if (!source || !event || !view ||
        !frame.snapshot.active_player_id() || !frame.snapshot.turn() ||
        *frame.snapshot.turn() != region.turn ||
        event->player_id != *frame.snapshot.active_player_id() ||
        event->token != region.event_token ||
        !event->unavailable_reason.empty() ||
        view->player_id != region.player_id ||
        view->turn != region.turn ||
        view->frame != region.frame ||
        view->frame != frame.snapshot.frame() ||
        view->event_token != region.event_token ||
        view->viewport_width != region.viewport_width ||
        view->viewport_height != region.viewport_height ||
        view->selected_source_path != region.selected_source_path ||
        source->selected_source_path != region.selected_source_path ||
        view->scroll_panel_height != region.scroll_panel_height ||
        view->vertical_offset_pixels != region.vertical_offset_pixels ||
        view->choice_count != region.choice_count ||
        region.choice_count > 4096)
        return {true, {UiCommandStatus::invalid,
                       "Selected BNW technology chooser scroll frame is stale"}};
    if (!contains(region.bounds, x, y))
        return {true, {UiCommandStatus::invalid,
                       "Focused technology chooser owns wheel input"}};
    return {true, {UiCommandStatus::unavailable, wheel_delta
        ? "Native BNW TechPopup wheel step is unverified"
        : "No scroll movement"}};
}

LiveUiInputResult LiveUiHost::wheel_ideology(const LiveUiFrame &frame,
    std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
    const IdeologyScrollRegion &region) {
    if (!current(frame))
        return {true, {UiCommandStatus::invalid, "UI frame is stale"}};
    if (screens_.focused_screen() != std::optional<std::string>{"ChooseIdeology"})
        return {};
    const auto *source = sources_.progression[static_cast<std::size_t>(
        ProgressionScreen::ideology)];
    const auto *view = frame.ideology_view ? &*frame.ideology_view : nullptr;
    if (!source || !view || !ideology_view_ ||
        view->player_id != region.player_id || view->turn != region.turn ||
        view->event_token != region.event_token ||
        view->selected_source_path != region.selected_source_path ||
        view->selected_source_path != source->selected_source_path ||
        view->viewport_width != region.viewport_width ||
        view->viewport_height != region.viewport_height ||
        view->overlay != region.overlay ||
        view->overlay != ideology_view_->overlay ||
        region.offset != (region.overlay == LiveUiFrame::IdeologyOverlay::tenets
            ? ideology_view_->tenet_offset : ideology_view_->list_offset) ||
        region.max_offset < 0 || region.max_offset > 2'000'000)
        return {true, {UiCommandStatus::invalid,
                       "Ideology ScrollPanel geometry is stale"}};
    if (!contains(region.bounds, x, y))
        return {true, {UiCommandStatus::invalid,
                       "Focused ideology popup owns wheel input"}};
    if (!wheel_delta || region.max_offset == 0)
        return {true, {UiCommandStatus::unavailable,
                       "Ideology ScrollPanel has no confirmed movement"}};
    return {true, {UiCommandStatus::unavailable,
                   "Native BNW ScrollPanel wheel step is unverified"}};
}

} // namespace civ5::ui

namespace civ5::ui {
void LiveUiHost::register_owned_research_handlers() {
    if(plot_yield_ui_thread_!=std::this_thread::get_id())throw std::logic_error("Research UI foreign thread");
    if(!city_selection_control_||selected_owner_lifetime_.expired()||research_handlers_)
        throw std::logic_error("Research handlers need the genuine registered selected LiveUiHost");
    ResearchHandlers handlers;
    const std::array<const char*,3> names{"TechTree","TopPanel","DiploList"};
    for(std::size_t n=0;n<names.size();++n){
        handlers.contexts[n]=screens_.registry().find(names[n]);
        if(!handlers.contexts[n])throw std::logic_error("Research handler context is not registered");
    }
    research_handlers_=std::move(handlers);
}
void LiveUiHost::require_owned_research_handlers(const LiveUiFrame& frame,std::int64_t team)const {
    if(plot_yield_ui_thread_!=std::this_thread::get_id())throw std::logic_error("Research UI foreign thread");
    if(!research_handlers_||!city_selection_control_||selected_owner_lifetime_.expired())
        throw std::logic_error("Research UI handlers absent");
    const std::array<const char*,3> names{"TechTree","TopPanel","DiploList"};
    for(std::size_t n=0;n<names.size();++n)
        if(screens_.registry().find(names[n])!=research_handlers_->contexts[n])
            throw std::logic_error("Research UI handler registry changed");
    const auto* projection=frame.snapshot.find<BnwResearchChoiceUiSnapshot>();
    const auto* progression=frame.snapshot.find<ProgressionFrameState>();
    if(!current(frame)||!projection||!progression||!frame.top_panel||
       projection->team!=team||progression->team_id!=team||progression->player_id!=projection->player||
       !projection->player_score||!projection->team_score)
        throw std::logic_error("Research UI has no current technology/TopPanel/stored score projection");
}
void LiveUiHost::consume_owned_research_dirty(runtime::BnwResearchChoiceDirty kind,
    const LiveUiFrame& frame,const runtime::BnwResearchChoiceCommit& commit) {
    if(plot_yield_ui_thread_!=std::this_thread::get_id())throw std::logic_error("Research UI foreign thread");
    require_owned_research_handlers(frame,commit.team());
    const auto* projection=frame.snapshot.find<BnwResearchChoiceUiSnapshot>();
    if(projection->player!=commit.player()||projection->revision!=commit.after_revision()||
       projection->research_queue!=commit.queue())throw std::logic_error("Research handler consumed another stored queue");
    switch(kind){
    case runtime::BnwResearchChoiceDirty::research_buttons:
        if(!frame.snapshot.find<ProgressionFrameState>())throw std::logic_error("Research button projection absent");break;
    case runtime::BnwResearchChoiceDirty::game_data:
        if(!frame.top_panel)throw std::logic_error("Actual TopPanel redraw absent");break;
    case runtime::BnwResearchChoiceDirty::score:
        if(!projection->player_score||!projection->team_score)throw std::logic_error("Actual stored score snapshot absent");break;
    default:throw std::logic_error("Research dirty handler kind unsupported");
    }
    // The registered Civis receiver keeps the actual freshly captured payload.
    // No event_add/Original listener count or GUI loading completion is minted.
    research_handlers_->snapshot=std::make_shared<const BnwResearchChoiceUiSnapshot>(*projection);
    research_handlers_->sequence=frame.sequence;
    require_owned_research_handlers(frame,commit.team());
}
}

namespace civ5::ui {
std::optional<BnwNotificationPublicationProgress> LiveUiHost::notification_publication_progress()const{
 if(plot_yield_ui_thread_!=std::this_thread::get_id())throw std::runtime_error("Notification history foreign creator");
 const auto owner=notification_publication_owner_;
 if(!owner)return std::nullopt;
 if(owner->creator_!=std::this_thread::get_id()||owner->ui_.lock().get()!=this||owner->self_.lock().get()!=owner.get())
  throw std::runtime_error("Notification history canonical receiver changed");
 // Historical actual receives only, including failed late source bookends.
 return owner->progress_;
}
}
