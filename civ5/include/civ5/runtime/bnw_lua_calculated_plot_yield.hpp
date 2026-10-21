#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
namespace civ5::runtime {
class LiveSession;struct LiveSessionState;class BnwNormalPlotYieldRules;class BnwLocalClientScalarOwner;
struct ScriptMapProvider;
class BnwLuaCalculatedPlotYieldUnavailable final:public std::runtime_error{
 public:using std::runtime_error::runtime_error;
};
// Numeric Lua transport owner, never a refresh writer or VM publication proof.
// The bDisplay argument is the native Lua selector, NOT Game debug authority.
class BnwLuaCalculatedPlotYieldOwner final {
 public:
  ~BnwLuaCalculatedPlotYieldOwner();
  BnwLuaCalculatedPlotYieldOwner(const BnwLuaCalculatedPlotYieldOwner&)=delete;
  BnwLuaCalculatedPlotYieldOwner& operator=(const BnwLuaCalculatedPlotYieldOwner&)=delete;
  void require_current()const;
  void require_same_world(const std::shared_ptr<const LiveSessionState>& actual)const;
  std::int32_t value(std::int64_t plot,std::int64_t yield,bool display)const;
  // Read from the SAME actual Game/VM epoch; never a selected preview actor.
  std::int32_t active_player()const;
  std::int32_t active_team()const;
  // Readonly original CvPlayer::m_bTurnActive from SAME native Game/VM.
  bool player_is_turn_active(std::int64_t player)const;
  // CvDllPreGame::isNetworkMultiplayerGame from SAME imported Game/VM.
  bool pregame_is_multiplayer_game()const;
  bool local_client_has_sent_turn_complete()const;
  bool local_client_has_sent_turn_all_complete()const;
  bool local_client_is_processing_messages()const;
  // SDK Game.IsOption through SAME real imported Pregame/client/world.
  bool game_is_option(std::int32_t selector)const;
  bool game_is_option(const char* requested)const;
  bool is_revealed(std::int64_t plot,std::int64_t team,bool debug)const;
 private:
  friend class LuaScriptHost;
  friend class BnwSelectedStockLuaSourceOwner;
  friend std::shared_ptr<BnwLuaCalculatedPlotYieldOwner> make_bnw_lua_calculated_plot_yield_owner(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwNormalPlotYieldRules>);
  friend ScriptMapProvider bnw_lua_calculated_plot_services(
   std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner>);
  struct Impl;std::unique_ptr<Impl> impl_;
  explicit BnwLuaCalculatedPlotYieldOwner(std::unique_ptr<Impl>)noexcept;
  void bind_initial();
  void bind_stock_epoch(std::function<void()>);
  void bind_local_client(std::shared_ptr<const BnwLocalClientScalarOwner>);
  struct VmLease;
  static std::shared_ptr<VmLease> make_vm_lease(const void* actual_state);
  static void retire_vm_lease(const std::shared_ptr<VmLease>&)noexcept;
  void bind_vm_lease(const std::shared_ptr<VmLease>&)const;
  ScriptMapProvider current_map_borrow()const;
};
std::shared_ptr<BnwLuaCalculatedPlotYieldOwner> make_bnw_lua_calculated_plot_yield_owner(
 std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwNormalPlotYieldRules>);
ScriptMapProvider bnw_lua_calculated_plot_services(
 std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner>);
}
