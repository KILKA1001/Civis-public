#include "civ5/ui/bnw_notification_minimap_record_queue.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
namespace civ5::ui {
struct BnwNotificationMinimapRecordQueue::State final {
 struct Record final {
  std::array<std::uint8_t,0x80> bytes;
  BnwEngineUiNotificationEffect source;
 };
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool appending=false;
 const std::weak_ptr<const BnwClientSceneFrame> scene;
 std::weak_ptr<BnwNotificationMinimapRecordQueue> canonical;
 const MinimapWorldTransform transform;
 const std::size_t maximum_records;
 std::vector<Record> records;
 State(std::weak_ptr<const BnwClientSceneFrame> s,MinimapWorldTransform t,std::size_t maximum)
  :scene(std::move(s)),transform(std::move(t)),maximum_records(maximum) {
  if(!maximum||maximum>std::numeric_limits<std::size_t>::max()/sizeof(Record))
   throw std::invalid_argument("Notification minimap OWN queue budget invalid");
  // No ring overrun/wrap or slot replacement policy is inferred from Original.
  records.reserve(maximum);
 }
};
namespace {
MinimapWorldTransform actual_transform(const terrain::WorldLayout& layout,
                                      const terrain::HeightOwnerParameters& height) {
 auto t=minimap_world_transform(layout,height);
 if(!std::all_of(t.cube_to_world.begin(),t.cube_to_world.end(),
                 [](float x){return std::isfinite(x);})||
    !std::isfinite(t.scale_x)||!std::isfinite(t.scale_y)||
    t.scale_x==0||t.scale_y==0||!std::isfinite(t.offset_x)||!std::isfinite(t.offset_y))
  throw std::invalid_argument("Notification minimap actual runtime transform invalid");
 return t;
}
bool same_transform(const MinimapWorldTransform& a,const MinimapWorldTransform& b) {
 return a.map_to_cube==b.map_to_cube&&a.cube_to_world==b.cube_to_world&&
  a.scale_x==b.scale_x&&a.scale_y==b.scale_y&&a.offset_x==b.offset_x&&a.offset_y==b.offset_y;
}
void word(std::array<std::uint8_t,0x80>& b,std::size_t offset,std::uint32_t value) {
 for(std::size_t i=0;i<4;++i)b[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
std::array<std::uint8_t,0x80> encode_record(
 const MinimapWorldTransform& actual,const BnwEngineUiNotificationEffect& effect) {
 // Exact producer integer arithmetic; no (-1,-1) early return or identity default.
 const auto r=static_cast<std::uint32_t>(effect.y);
 const auto q=static_cast<std::uint32_t>(effect.x)-
              static_cast<std::uint32_t>(effect.y>>1);
 const auto s=0u-q-r;
 const auto& m=actual.map_to_cube;
 const auto component=[&](std::size_t lane) {
  std::uint32_t value=static_cast<std::uint32_t>(m[12+lane]);
  value+=q*static_cast<std::uint32_t>(m[lane]);
  value+=s*static_cast<std::uint32_t>(m[8+lane]);
  value+=r*static_cast<std::uint32_t>(m[4+lane]);
  return value;
 };
 const auto x=component(0),y=component(1);
 // Original defines header and payload through +0x2f. Remaining reserved tail
 // is zero only as OWN deterministic initialization, not Original byte parity.
 std::array<std::uint8_t,0x80> bytes{};
 word(bytes,0,0x80);word(bytes,4,0x60);word(bytes,8,0);word(bytes,12,0);
 word(bytes,0x10,static_cast<std::uint32_t>(effect.lookup)); // Already lookup+1.
 word(bytes,0x14,(x&0xffffu)|((y&0xffffu)<<16));
 word(bytes,0x18,0x3f800000u);word(bytes,0x1c,0x3f800000u);
 word(bytes,0x20,0);word(bytes,0x24,0x3e4ccccdu);
 word(bytes,0x28,0x3f800000u);word(bytes,0x2c,0x40a00000u);
 return bytes;
}
}
BnwNotificationMinimapRecordQueue::BnwNotificationMinimapRecordQueue(
 std::weak_ptr<const BnwClientSceneFrame> scene,MinimapWorldTransform transform,std::size_t maximum)
 :state_(std::make_shared<State>(std::move(scene),std::move(transform),maximum)){}
std::shared_ptr<BnwNotificationMinimapRecordQueue>
BnwNotificationMinimapRecordQueue::create_for_actual_scene(
 const SceneFactoryKey& key,const terrain::WorldLayout& layout,
 const terrain::HeightOwnerParameters& height,std::size_t maximum) {
 if(key.creator_!=std::this_thread::get_id())
  throw std::runtime_error("Notification minimap Scene factory foreign creator");
 const auto scene=key.scene_;
 if(scene.expired())throw std::invalid_argument("Notification minimap actual Scene absent");
 auto transform=actual_transform(layout,height);
 auto result=std::shared_ptr<BnwNotificationMinimapRecordQueue>(
  new BnwNotificationMinimapRecordQueue(std::move(scene),std::move(transform),maximum));
 result->state_->canonical=result;
 pure(result->state_);
 return result;
}
BnwNotificationMinimapRecordQueue::~BnwNotificationMinimapRecordQueue() {
 state_->alive.store(false); // No borrowed Scene/source/runtime access.
}
void BnwNotificationMinimapRecordQueue::quarantine_without_callbacks(
 const std::shared_ptr<State>& state)noexcept {
 if(state)state->failed.store(true);
}
void BnwNotificationMinimapRecordQueue::pure(const std::shared_ptr<State>& state) {
 if(!state||state->creator!=std::this_thread::get_id()||
    !state->alive.load()||state->failed.load())
  throw std::runtime_error("Notification minimap queue retired, failed or foreign");
 if(state->scene.expired()||state->canonical.expired())
  throw std::runtime_error("Notification minimap queue outside owner retired");
}
std::shared_ptr<BnwNotificationMinimapRecordQueue::State>
BnwNotificationMinimapRecordQueue::save_for_source_without_callbacks()const {
 if(creator_!=std::this_thread::get_id())
  throw std::runtime_error("Notification minimap queue foreign creator");
 const auto saved=state_;pure(saved);
 {
  const auto actual=saved->canonical.lock();
  if(!actual||actual.get()!=this){quarantine_without_callbacks(saved);
   throw std::runtime_error("Notification minimap canonical queue substituted");}
 }
 pure(saved); // Internal facade pin released; it cannot mask last outside owner.
 if(saved->appending){quarantine_without_callbacks(saved);
  throw std::runtime_error("Notification minimap queue source reentered");}
 return saved;
}
void BnwNotificationMinimapRecordQueue::append_current_owned(
 const std::shared_ptr<State>& saved,
 const std::weak_ptr<const BnwClientSceneFrame>& expected_scene,
 const terrain::WorldLayout& actual_layout,const terrain::HeightOwnerParameters& actual_height,
 const BnwEngineUiNotificationEffect& effect) {
 pure(saved);
 if(saved->appending){quarantine_without_callbacks(saved);
  throw std::runtime_error("Notification minimap queue append reentered");}
 saved->appending=true;
 struct Appending {State& state;~Appending(){state.appending=false;}} appending{*saved};
 try {
  {
   const auto scene=saved->scene.lock(),expected=expected_scene.lock();
   if(!scene||!expected||scene.get()!=expected.get()||
      scene.owner_before(expected)||expected.owner_before(scene))
    throw std::runtime_error("Notification minimap queue actual Scene get/control differs");
  }
  pure(saved); // Drop temporary Scene pins before any allocation/copy boundary.
  const auto current=actual_transform(actual_layout,actual_height);
  if(!same_transform(current,saved->transform))
   throw std::runtime_error("Notification minimap queue actual terrain transform changed");
  if(effect.kind!=BnwEngineUiNotificationEffectKind::gameplay_minimap_notification||
     effect.lookup<=0||!effect.generation||!effect.occurrence)
   throw std::invalid_argument("Notification minimap queue actual source effect invalid");
  if(saved->records.size()>=saved->maximum_records)
   throw std::runtime_error("Notification minimap OWN record storage full");
  if(!saved->records.empty()&&effect.occurrence<=saved->records.back().source.occurrence)
   throw std::runtime_error("Notification minimap source occurrence replayed or unordered");
  auto bytes=encode_record(current,effect);
  State::Record record{bytes,effect}; // Actual source VALUE metadata copy may allocate.
  pure(saved); // FIRST independent life POST; no borrowed layout/effect used afterward.
  saved->records.push_back(std::move(record)); // Real owned FIFO store mutation.
  pure(saved); // FIRST POST, no caller bool/count/physical-success token.
 } catch(...) {quarantine_without_callbacks(saved);throw;}
}
}
