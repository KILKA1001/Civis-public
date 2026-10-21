#pragma once
#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <atomic>
#include <thread>
namespace civ5::runtime {
struct BnwMapScriptSelectedAccess;
// Actual VFS/rules lifetime for one scoped map-constructor provider job.
// No fresh Game, UI startup or map publication capability is issued.
class BnwSelectedMapSourceOwner final:public ScriptUiContextSourceOwner {
 public:
  ~BnwSelectedMapSourceOwner() override;
  BnwSelectedMapSourceOwner(const BnwSelectedMapSourceOwner&)=delete;
  BnwSelectedMapSourceOwner& operator=(const BnwSelectedMapSourceOwner&)=delete;
  void require_current(std::string_view,const ScriptUiChildSource*)const override;
  std::shared_ptr<const ScriptUiChildSource> resolve_child(
   std::string_view,const ScriptUiChildSource*,std::string_view) override;
 private:
  friend struct BnwMapScriptSelectedAccess;
  struct Pin {assets::AssetHandle handle;std::string sha;};
  BnwSelectedMapSourceOwner(std::shared_ptr<const BnwFreshGameContentResult>,
   assets::AssetHandle,std::vector<std::pair<std::string,assets::AssetHandle>>,
   const assets::ResourceMountTable&,const BnwMapSetupContext&,const SimulationRandom&,std::uint64_t,
   std::shared_ptr<BnwFreshGameContentResult::ContentEpoch>);
  void begin_load(std::string_view);
  void require_include_bytes(std::string_view,std::span<const std::uint8_t>);
  void loaded(std::string_view);
  ScriptMapInitQueryResult query(LuaScriptHost&,std::int32_t);
  void retire()noexcept;
  void require_same_provider_operands(const assets::ResourceMountTable&,
    const BnwMapSetupContext&,const SimulationRandom&)const;
  std::thread::id thread_=std::this_thread::get_id();
  std::shared_ptr<const BnwFreshGameContentResult> parent_;
  std::shared_ptr<BnwFreshGameContentResult::ContentEpoch> epoch_;
  const content::RulesDatabase* database_=nullptr;
  std::string rules_sha_;
  std::function<bool()> rules_current_;
  Pin root_;
  std::vector<Pin> includes_;
  std::vector<std::string> include_names_;
  const assets::ResourceMountTable* resources_=nullptr;
  std::vector<assets::MountedResource> root_selection_;
  std::vector<assets::MountedResource> include_selection_;
  const BnwMapSetupContext* setup_=nullptr;
  const SimulationRandom* random_=nullptr;
  BnwMapSetupContext setup_before_;
  std::uint64_t source_limit_=0;
  std::atomic<bool> retired_{false};
  mutable bool checking_=false;
  bool attempted_=false,loaded_=false,queried_=false;
};
// Definition belongs to the proposed existing map provider job, reusing its
// selected resource resolver, includes and actual RNG. No callback proof flag.
struct BnwSelectedMapInitResult {
 BnwMapScriptResult constructor;
 std::optional<ScriptMapInitQueryResult> init;
};
BnwSelectedMapInitResult query_bnw_selected_map_init_data(
 std::shared_ptr<const BnwFreshGameContentResult>,const assets::ResourceMountTable&,
 const BnwMapSourceKey&,BnwMapSetupContext,SimulationRandom,BnwMapScriptBudget={});

// A real, retained standalone selected-source job; not an initialized Game.
// Owns ONE internally constructed VFS table, VM, providers and local RNG.
// Requested setup/local RNG still require a genuine Pregame/Game join before
// native map initialization or fresh LiveSession publication can be granted.
enum class BnwSelectedMapJobBoundary { constructor_loaded, init_data_returned,
    native_pregame_operands_required };
struct BnwSelectedMapJobObservation {
    BnwSelectedMapJobBoundary boundary;
    BnwSelectedMapInitResult source;
    SimulationRandomState map_random;
    std::size_t initialized_plots;
};
class BnwSelectedMapNativeServicesUnavailable final:public ScriptError {
 public:using ScriptError::ScriptError;
};
class BnwSelectedMapJob final {
 public:
    static std::shared_ptr<BnwSelectedMapJob> from_selected_installation(
        std::shared_ptr<const BnwFreshGameContentResult>,
        const std::filesystem::path&,const content::Catalog&,
        const BnwMapSourceKey&,BnwMapSetupContext,SimulationRandom,BnwMapScriptBudget={});
    ~BnwSelectedMapJob();
    BnwSelectedMapJob(const BnwSelectedMapJob&)=delete;
    BnwSelectedMapJob& operator=(const BnwSelectedMapJob&)=delete;
    void advance_init_data_query();
    // Earliest source prerequisite is actual Pregame World/Climate/SeaLevel
    // getters for the init log. Missing services throw BEFORE reset/grid.
    // DoMapSetup, pathfinder setup, full InitPlots and Barbarians remain later.
    void advance_native_map_init();
    void require_current()const;
    BnwSelectedMapJobObservation observe()const;
    // Read-only SAME table alias for existing source observer instrumentation.
    // Its lifetime pin does not issue job operations or map/game authority.
    std::shared_ptr<const assets::ResourceMountTable> resource_source_view()const;
 private:
    struct Impl;std::shared_ptr<Impl> impl_;
    explicit BnwSelectedMapJob(std::shared_ptr<Impl>);
};
}
