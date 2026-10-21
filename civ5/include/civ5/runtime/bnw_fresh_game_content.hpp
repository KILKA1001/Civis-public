#pragma once

#include "civ5/content/saved_rules.hpp"
#include <memory>
#include <span>
#include <atomic>
#include <thread>
#include <functional>

namespace civ5::runtime {

enum class FreshGameContentBlockerCode {
    expansion2_not_selected,
    content_profile_incomplete,
    native_gamecore_unavailable,
    rules_activation_failed,
    rules_database_incomplete,
    lua_loading_pending,
    map_script_pending,
    world_setup_pending,
};

struct FreshGameContentBlocker {
    FreshGameContentBlockerCode code;
    std::string message;
};

// Owns the activated rules independently of any UI or live world. The last
// three blockers remain until the session owner runs its real Lua, map-script,
// and world-creation stages. This result never attests to a playable game.
struct BnwFreshGameContentResult {
    BnwFreshGameContentResult()=default;
    BnwFreshGameContentResult(const BnwFreshGameContentResult&)=delete;
    BnwFreshGameContentResult& operator=(const BnwFreshGameContentResult&)=delete;
    BnwFreshGameContentResult(BnwFreshGameContentResult&&)noexcept;
    BnwFreshGameContentResult& operator=(BnwFreshGameContentResult&&)noexcept;
    ~BnwFreshGameContentResult();
    SavedContentProfile profile;
    std::shared_ptr<content::RulesDatabase> rules;
    std::optional<content::SavedRulesActivationReport> activation;
    std::vector<FreshGameContentBlocker> blockers;

    bool content_ready() const;
    bool has_blocker(FreshGameContentBlockerCode code) const;
  private:
    friend class BnwCreationConfig;
    friend BnwFreshGameContentResult bootstrap_bnw_fresh_game_content(
        const std::filesystem::path&,const content::Catalog&,
        std::span<const std::size_t>,std::span<const std::size_t>);
    friend class BnwSelectedMapSourceOwner;
    friend class BnwSelectedMapJob;
    friend struct BnwMapScriptSelectedAccess;
    friend std::shared_ptr<BnwFreshGameContentResult> bootstrap_bnw_fresh_game_content_owner(
        const std::filesystem::path&,const content::Catalog&,
        std::span<const std::size_t>,std::span<const std::size_t>);
    // Issued only after actual successful activation. Independent strong
    // owners retain SAME facade and SAME private RulesImpl storage.
    std::optional<std::filesystem::path> activated_game_root_;
    const content::RulesDatabase* activated_rules_=nullptr;
    std::function<bool(const content::RulesDatabase&)> activated_rules_identity_;
    std::shared_ptr<const content::RulesDatabase> activated_rules_owner_;
    void initialize_actual_rules_identity(const content::RulesDatabase&);
    static std::shared_ptr<const BnwFreshGameContentResult> retain_factory_owner(
        const std::shared_ptr<const BnwFreshGameContentResult>&);
    struct ContentEpoch;
    std::shared_ptr<ContentEpoch> epoch_;
    std::thread::id owner_thread_=std::this_thread::get_id();
    static std::shared_ptr<ContentEpoch> retain_epoch(
        const std::shared_ptr<const BnwFreshGameContentResult>&);
};

// Private factory storage: the stable view owns SAME activated rule backing.
// Public facade death/move revokes the epoch, but cannot free provider operands.
struct BnwFreshGameContentResult::ContentEpoch final {
    const std::thread::id thread;
    mutable std::atomic<bool> retired{false};
    const std::filesystem::path installation;
    const content::Catalog catalog;
    BnwFreshGameContentResult view;
    std::weak_ptr<const BnwFreshGameContentResult> canonical;
    const std::vector<std::pair<std::filesystem::path,std::pair<std::uint64_t,std::string>>> manifest_pins;
    ContentEpoch(const std::shared_ptr<BnwFreshGameContentResult>&,
        std::filesystem::path,content::Catalog,
        std::vector<std::pair<std::filesystem::path,std::pair<std::uint64_t,std::string>>>);
    void require_current()const;
    void require_recipe(const std::filesystem::path&,const content::Catalog&)const;
};

// Resolve only caller-selected packages. Expansion2 must be among those DLCs;
// other installed packages are never inferred. Rules use the existing base,
// selected-DLC, and selected-mod activation contracts in one isolated DB.
BnwFreshGameContentResult bootstrap_bnw_fresh_game_content(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    std::span<const std::size_t> selected_dlc,
    std::span<const std::size_t> selected_mods = {});

// Genuine owner for retained jobs. The by-value result above remains diagnostic;
// external make_shared/no-op controlblocks cannot seed this private authority.
std::shared_ptr<BnwFreshGameContentResult> bootstrap_bnw_fresh_game_content_owner(
    const std::filesystem::path&,const content::Catalog&,
    std::span<const std::size_t>,std::span<const std::size_t> = {});

} // namespace civ5::runtime
