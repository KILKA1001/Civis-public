#pragma once

#include "civ5/ui/ui_runtime.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/bnw_research_choice.hpp"
#include <set>

namespace civ5::ui {

// Only values actually present in the activated BNW rules and live player are
// published. The host supplies a new value after each game-state change.
struct ProgressionTechnology {
    std::int64_t id = -1;
    std::string type, description_key;
    std::optional<std::string> description;
    std::int64_t era = -1, grid_x = 0, grid_y = 0, cost = 0;
    bool researched = false, queued = false;
    std::int64_t progress_times100 = 0;
};
struct ProgressionPolicy {
    std::int64_t id = -1;
    std::string type, branch_type, description_key;
    std::optional<std::string> description;
    std::int64_t grid_x = 0, grid_y = 0;
    bool adopted = false;
};
struct ProgressionBranch {
    std::int64_t id = -1;
    std::string type, description_key;
    std::optional<std::string> description;
    bool ideology = false;
};
struct ProgressionFrameState {
    std::int64_t player_id = -1, team_id = -1;
    std::vector<ProgressionTechnology> technologies;
    std::vector<ProgressionPolicy> policies;
    std::vector<ProgressionBranch> branches;
    std::optional<std::int64_t> current_research;
    bool policy_ownership_resolved = false;
    std::string unavailable_reason;
};

enum class ProgressionTechPopupMode { research, free_technology, steal_technology };
// The live GameCore/Lua popup dispatcher supplies this signal only while a
// CHOOSETECH or CHOOSE_TECH_TO_STEAL event is active for the named player.
struct ProgressionTechPopupEvent {
    std::int64_t player_id = -1;
    ProgressionTechPopupMode mode = ProgressionTechPopupMode::research;
    std::optional<std::int64_t> steal_target_player;
    // Opaque nonzero identity assigned by the real popup owner. Two events
    // with identical player/mode/target must still have different tokens.
    std::uint64_t token = 0;
};
// Supplied only for a real CHOOSETECH / CHOOSE_TECH_TO_STEAL event. Candidate
// IDs come from the active research rules and player gates, never from the
// technology table alone.
struct ProgressionTechPopupState {
    std::int64_t player_id = -1;
    ProgressionTechPopupMode mode = ProgressionTechPopupMode::research;
    std::optional<std::int64_t> steal_target_player;
    std::uint64_t token = 0;
    std::vector<std::int64_t> eligible_technologies;
    std::string unavailable_reason;
};
// Published only by the owner of a real BUTTONPOPUP_CHOOSE_IDEOLOGY event.
// A policy table containing three ideology branches is not evidence that the
// active player has an ideology decision to make.
struct ProgressionIdeologyPopupEvent {
    std::int64_t player_id = -1;
    std::uint64_t token = 0;
};
// Published only while the GameCore/Lua BUTTONPOPUP_TECH_TREE owner holds the
// popup. Its token is unique per occurrence, even for the same player/turn.
struct ProgressionTechnologyTreeEvent {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t token = 0;
};
ProgressionTechPopupState capture_progression_tech_popup(
    runtime::ResearchSelectionContext &research, std::int64_t player_id,
    ProgressionTechPopupMode mode, std::optional<std::int64_t> steal_target_player = std::nullopt);

// Called by the live session with its current owned state and active rules.
// If the policy load hook has not populated ResearchContext, saved_world may
// supply validated read-only ownership to paint the frame. It never makes a
// live policy action available.
ProgressionFrameState capture_progression_frame(
    const runtime::ResearchContext &world,
    const runtime::PlayerResearchContext &players,
    std::span<const runtime::TeamTechnologies> teams,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    std::int64_t player_id,
    const content::InfoLocalizer &localizer = {},
    const saves::SavedSessionSnapshot *saved_world = nullptr);

enum class ProgressionScreen { technologies, policies, ideology, technology_popup };
std::string_view progression_context_id(ProgressionScreen screen);
struct ProgressionScreenSource {
    std::string requested_key, selected_source_path;
    std::vector<std::string> control_ids;
    content::XmlNode document;
};
ProgressionScreenSource read_progression_screen_source(
    const assets::ResourceMountTable &resources, ProgressionScreen screen);
UiCommand progression_top_panel_command(bool science);
std::optional<UiCommand> progression_top_panel_command(TopPanelAction action);
UiCommand progression_open_command(ProgressionScreen screen);
std::vector<UiCommand> progression_popup_open_tree_commands();

struct ProgressionRect { std::int64_t x = 0, y = 0, width = 0, height = 0; };
enum class ProgressionItemKind { technology, policy, ideology };
struct ProgressionRenderItem {
    ProgressionItemKind kind = ProgressionItemKind::technology;
    std::int64_t id = -1;
    std::string parent_control_id, text;
    // Local to parent_control_id. A missing rectangle means the authored Lua
    // measures the item at runtime using data absent from this frame.
    std::optional<ProgressionRect> rect;
    bool selected = false;
    bool owned = false;
};
struct ProgressionRenderPlan {
    ProgressionScreen screen = ProgressionScreen::technologies;
    std::string selected_source_path;
    std::vector<ProgressionRenderItem> items;
    std::vector<std::string> unresolved;
};
ProgressionRenderPlan plan_progression_screen(const ProgressionScreenSource &source,
    ProgressionScreen screen, const UiFrameSnapshot &frame);

struct ProgressionChooseTechnology { std::int64_t technology = -1; bool clear_queue = true; };
// TechPopup.lua acknowledges CHOOSETECH and publishes TECH_TREE before its
// authored OpenTTButton changes screens. The owner must perform both events.
struct ProgressionOpenTechnologyTree { std::uint64_t event_token = 0; };
struct ProgressionCloseTechnologyTree { std::uint64_t event_token = 0; };
struct ProgressionChoosePolicy { std::int64_t policy = -1; };
struct ProgressionChooseIdeology { std::int64_t branch = -1; };
UiCommand progression_choose_technology(std::int64_t id, bool clear_queue = true);
UiCommand progression_choose_popup_technology(std::int64_t id);
UiCommand progression_popup_open_tree_command(std::uint64_t event_token);
UiCommand progression_tree_close_command(std::uint64_t event_token);
UiCommand progression_choose_policy(std::int64_t id);
UiCommand progression_choose_ideology(std::int64_t id);

// Read-only BNW CvPlayerPolicies::CanAdoptPolicy preflight through its native
// non-Lua gates. Values come from the current owned session world and selected
// activated rules. A pass is never authority to adopt: Lua and all mutation
// effects remain separate mandatory services.
struct BnwPolicyChoicePreview {
    bool passes_known_rules = false;
    std::string policy_type, branch_type;
    std::optional<std::int64_t> culture, next_cost, free_policies, free_tenets;
    std::string blocker;
};
BnwPolicyChoicePreview preview_bnw_policy_choice(
    const runtime::LiveSession &session, const content::RulesDatabase &database,
    const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines, std::int64_t player_id,
    std::int64_t policy_id);

// Returns null when the live frame has not supplied progression data for the
// active player. A screen can render this as unavailable without fake values.
const ProgressionFrameState *progression_for_active_player(const UiFrameSnapshot &frame);

// The research selection implementation is available in Civis. Policy and
// ideology adoption lack a BNW mutation backend and remain unavailable.
class ProgressionGameplayBackend final : public UiGameplayBackend {
  public:
    explicit ProgressionGameplayBackend(runtime::ResearchSelectionContext *research = nullptr)
        : research_(research) {}
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
  private:
    runtime::ResearchSelectionContext *research_;
};

// A live BNW owner can route TechTree selections through the transactional
// session API. Policy and ideology commands validate live frame/ownership
// boundaries but remain unavailable until their full GameCore/Lua effects
// and rollback exist. The optional popup event reader must reflect the real
// current GameCore/Lua event; a saved queue or visible XML is not an event.
class BnwLiveProgressionBackend final : public UiGameplayBackend {
  public:
    using PopupEventReader = std::function<std::optional<ProgressionTechPopupEvent>(
        const runtime::LiveSession &)>;
    // Called only after a successful queue commit. The owner must process
    // exactly the supplied token; it must not infer an event from research.
    using PopupProcessed = std::function<void(const ProgressionTechPopupEvent &)>;
    // policy_database is the activated selected-rule owner and must outlive
    // this backend. A missing owner keeps policy actions unavailable.
    BnwLiveProgressionBackend(runtime::LiveSession &session,
        runtime::BnwResearchChoiceRules rules,
        runtime::BnwResearchChoiceServices services,
        PopupEventReader popup_event = {}, PopupProcessed popup_processed = {},
        const content::RulesDatabase *policy_database = nullptr);
    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
  private:
    runtime::LiveSession &session_;
    runtime::BnwResearchChoiceRules rules_;
    runtime::BnwResearchChoiceServices services_;
    PopupEventReader popup_event_;
    PopupProcessed popup_processed_;
    const content::RulesDatabase *policy_database_ = nullptr;
};

} // namespace civ5::ui
