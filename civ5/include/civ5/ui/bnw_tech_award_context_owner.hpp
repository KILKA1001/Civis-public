#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <array>
#include <filesystem>
#include "civ5/ui/bnw_tech_award_plain_label_layout.hpp"
#include "civ5/ui/bnw_tech_award_context_layout.hpp"
#include "civ5/ui/bnw_tech_award_animation.hpp"
#include "civ5/runtime/lua_script_host.hpp"
namespace civ5::content {struct Catalog;struct XmlNode;}
namespace civ5::runtime {struct SavedContentProfile;}
namespace civ5::assets {class ResourceMountTable;}
namespace civ5::runtime {class LuaScriptHost;class BnwSelectedStockLuaBootstrap;class BnwMajorTechTransportRequest;}
namespace civ5::ui {
class LiveUiHost;class BnwTechAwardInputDispatch;
struct BnwTechAwardControlValue {
 std::string id,kind,text,tooltip,texture,texture_sha256;
 bool hidden=false;
 std::int32_t texture_x=0,texture_y=0;
 std::optional<std::int32_t> local_x,local_y,width,height;
 bool operator==(const BnwTechAwardControlValue&)const=default;
};
struct BnwTechAwardMeasuredLabel {
 std::string path,id,text,font_key,font_sha256,font_style;
 BnwTechAwardPlainLabelLayout layout;
};
struct BnwTechAwardContextObservation {
 std::string context="TechAwardPopup",xml_sha256;
 std::vector<BnwTechAwardControlValue> controls;
 std::vector<BnwTechAwardMeasuredLabel> measured_labels;
 std::string styles_sha256;
 std::shared_ptr<const BnwTechAwardContextLayout> client_layout;
 std::optional<std::int32_t> measured_grid_width,measured_grid_height;
 std::uint64_t setter_occurrences=0;
 std::int32_t bonus_width=0,bonus_height=0;
 bool bonus_size_calculated=false,bonus_anchoring_reprocessed=false,auto_size_complete=false;
 bool queued=false,visible=false;
 // Native style/font layout and original PopupPriority issuer are mandatory.
 std::string pending;
 std::string language;
 std::vector<std::pair<std::string,std::string>> localized_queries;
};
// Distinct actual selected XML control storage. No Civilopedia Controls or
// publicly supplied callback/boolean can issue, mutate or mark it queued.
class BnwTechAwardContextOwner final {
 public:
 ~BnwTechAwardContextOwner();
 void require_current()const;
 BnwTechAwardContextObservation observation()const;
 BnwTechAwardContextOwner(const BnwTechAwardContextOwner&)=delete;
 BnwTechAwardContextOwner&operator=(const BnwTechAwardContextOwner&)=delete;
 private:
 friend class runtime::LuaScriptHost;
 friend class runtime::BnwMajorTechTransportRequest;
 friend class LiveUiHost;
 friend class BnwTechAwardAnimationWitness;
 void transition_selected_mouse(ClientPoint,const BnwTechAwardContextLayout&,const BnwTechAwardAnimationWitness&)const;
 bool selected_mouse_visible(std::string_view,const BnwTechAwardContextLayout&)const;
 void initialize_selected_alpha(const BnwTechAwardContextLayout&)const;
 std::vector<std::pair<std::string,float>> advance_selected_alpha(
  const BnwTechAwardAnimationSample&,const BnwTechAwardContextLayout&,
  const BnwTechAwardAnimationWitness&)const;
 friend class BnwTechAwardInputDispatch;
 friend class runtime::BnwSelectedStockLuaBootstrap;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwTechAwardContextOwner(std::unique_ptr<Impl>);
 // Resource table must be the SAME Bootstrap-owned table and survive its VM
 // lifetime; its parent/source bookends are enforced by the genuine request.
 static std::shared_ptr<const BnwTechAwardContextOwner> create(
  std::shared_ptr<const runtime::BnwMajorTechTransportRequest>,const assets::ResourceMountTable&);
 std::shared_ptr<const BnwTechAwardContextOwner> retain_canonical(
  const std::shared_ptr<const BnwTechAwardContextOwner>&)const;
 // Bootstrap-only setup from its actual selected root/catalog/profile.
 void bind_selected_localization(const std::filesystem::path&,const runtime::SavedContentProfile&,const content::Catalog&)const;
 std::string convert_text_key(std::string_view,std::span<const runtime::ScriptValue>)const;
 std::string convert_text_key_owned(std::string_view,std::span<const runtime::ScriptValue>)const;
 // Source-proved native table member; no public integer issuer.
 std::int32_t popup_priority()const;
 std::int32_t native_popup_type()const;
 std::int32_t native_mouse_left_click()const;
 std::int32_t active_player()const;
 std::int32_t active_team()const;
 std::vector<std::string> control_ids()const;
 std::string control_kind(std::string_view)const;
 void set_text(std::string_view,std::string_view)const;
 void set_tooltip(std::string_view,std::string_view)const;
 void set_texture(std::string_view,std::string_view)const;
 void set_texture_offset(std::string_view,std::int64_t,std::int64_t)const;
 void set_hide(std::string_view,bool)const;
 // Native selected-control ingress slots. Only the actual SAME LuaHost's
 // context installer may register after retaining each original Lua closure.
 // No public callback/ref number/readiness issuer is accepted here.
 void register_click(std::string_view,std::int64_t mouse_event)const;
 std::shared_ptr<const BnwTechAwardContextOwner> retain_queued_canonical(const std::shared_ptr<const BnwTechAwardContextOwner>&)const;
 BnwTechAwardContextObservation queued_observation(ClientViewport)const;
 std::shared_ptr<const BnwTechAwardContextLayout> project_client_layout(ClientViewport)const;
 void prepare_client_layout_resources()const;
 void require_queued_current()const;
 std::shared_ptr<const runtime::BnwMajorTechTransportRequest> retained_request_for_input()const;
 void begin_queued_input(const std::shared_ptr<const BnwTechAwardInputDispatch>&)const;
 void finish_queued_input(const BnwTechAwardInputDispatch&)const;
 void fail_queued_input()const noexcept;
 void require_same_actual_request(const runtime::BnwMajorTechTransportRequest&)const;
 void register_input_handler()const;
 void register_showhide_handler()const;
 bool requires_showhide()const;
 void finish_actual_showhide()const;
 void calculate_bonus_stack()const;
 void reprocess_bonus_stack()const;
 // Does not substitute authored 560x530 for actual dynamic font/text layout.
 // Current implementation refuses at this reached metric frontier.
 void auto_size()const;
 std::array<std::int32_t,2> measure_grid_nominal()const;
 BnwTechAwardMeasuredLabel measure_authored_plain_label(std::string,const content::XmlNode&,std::string)const;
 void queue_popup()const;
 void dequeue_popup()const;
 void increment_timer_semaphore()const;
 void decrement_timer_semaphore()const;
 void popup_shown()const;
 void popup_processed(std::int64_t popup_type,std::int64_t result)const;
 bool hidden()const;
};
}
