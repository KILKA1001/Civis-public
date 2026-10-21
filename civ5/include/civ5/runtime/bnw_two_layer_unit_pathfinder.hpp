#pragma once
#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;
// Owned by the SAME LiveSession common control allocation. All search inputs
// and callbacks are private to its authentic Core consumer. A caller cannot
// manufacture an admission/path receipt by supplying a cost or a predicate.
class BnwTwoLayerUnitPathFinder final {
 public:
  ~BnwTwoLayerUnitPathFinder();
  BnwTwoLayerUnitPathFinder(const BnwTwoLayerUnitPathFinder&)=delete;
  BnwTwoLayerUnitPathFinder& operator=(const BnwTwoLayerUnitPathFinder&)=delete;
 private:
  friend class LiveSession;
  friend class BnwNativeUnitAllocatorCore;
  enum class List:std::int8_t {none=-1,open=0,closed=1};
  enum class ChildNotice:std::int32_t {initial=0,open_update=1,closed_update=2,parent_update=3,new_node=4};
  struct CostCache {
   bool visible=false,mountain=false,water=false,can_enter_terrain=false,revealed=false;
   bool other_friendly_city=false,enemy_city=false,visible_enemy=false,visible_enemy_defender=false;
   std::int32_t friendly_units=0;
  };
  struct Node {
   std::int32_t x=-1,y=-1,total=0,known=0,heuristic=0,moves=0,turns=0;
   List list=List::none;
   Node *parent=nullptr,*next=nullptr,*previous=nullptr,*stack=nullptr;
   std::vector<Node*>children;
   bool on_stack=false;
   CostCache cache;
   void clear()noexcept;
  };
  struct Callbacks {
   std::function<void()>require_current,initialize,uninitialize;
   std::function<bool(std::int32_t,std::int32_t)>destination_valid;
   std::function<bool(const Node*,Node&)>valid;
   std::function<std::int32_t(const Node&,const Node&)>cost;
   std::function<void(const Node*,Node&,ChildNotice)>notify_child;
   std::function<std::int32_t(std::int32_t,std::int32_t,std::int32_t,std::int32_t)>heuristic;
   // Native PathNodeAdd's flags/mountain operand. Evaluated only with positive
   // moves and outside the destination, before touching the partial layer.
   std::function<bool(const Node&)>partial_required;
  };
  explicit BnwTwoLayerUnitPathFinder(BnwAStarCacheControl&,std::uint64_t,std::uint64_t,bool,bool);
  bool generate(std::uint64_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,
      std::int32_t,bool,bool,const Callbacks&);
  void require_thread()const;
  void fence()const;
  bool valid_coordinates(std::int32_t,std::int32_t)const noexcept;
  bool destination(std::int32_t,std::int32_t)const noexcept;
  Node& primary(std::int32_t,std::int32_t);
  Node& partial(std::int32_t,std::int32_t);
  void clear_lists()noexcept;
  void add_open(Node&);
  void notify_open(Node&);
  void update_open(Node&);
  Node* get_best();
  void create_children(Node&);
  void link_child(Node&,Node&);
  void update_parents(Node&);
  void push(Node&);
  Node* pop();
  const std::thread::id creator_=std::this_thread::get_id();
  BnwAStarCacheControl*control_;
  std::int32_t width_,height_,destination_x_=-1,destination_y_=-1;
  bool wrap_x_,wrap_y_,busy_=false,poisoned_=false;
  std::vector<Node>primary_,partial_;
  Node *open_=nullptr,*tail_=nullptr,*closed_=nullptr,*best_=nullptr,*stack_=nullptr;
  const Callbacks*callbacks_=nullptr;
  std::optional<BnwAStarCacheControl>expected_control_;
};
}
