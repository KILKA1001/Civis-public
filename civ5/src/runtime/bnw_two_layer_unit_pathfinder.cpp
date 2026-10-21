#include "civ5/runtime/bnw_two_layer_unit_pathfinder.hpp"
#include <array>
#include <climits>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void need(bool ok,const char*why){if(!ok)throw BnwCommonPathfinderUnavailable(why);}
std::int32_t sum(std::int32_t a,std::int32_t b){const auto n=std::int64_t(a)+b;need(n>=INT32_MIN&&n<=INT32_MAX,"Native path cost addition overflow");return static_cast<std::int32_t>(n);}
std::int32_t product(std::int32_t a,std::int32_t b){const auto n=std::int64_t(a)*b;need(n>=INT32_MIN&&n<=INT32_MAX,"Native path cost multiplication overflow");return static_cast<std::int32_t>(n);}
std::int32_t ranged(std::int32_t value,std::int32_t size,bool wrap){if(!wrap)return value;const auto n=value%size;return n<0?n+size:n;}
std::int32_t half(std::int32_t y){return y>=0?(y>>1):((y-1)/2);}
}
void BnwTwoLayerUnitPathFinder::Node::clear()noexcept{
 total=known=heuristic=moves=turns=0;on_stack=false;list=List::none;
 parent=next=previous=stack=nullptr;children.clear();
 // Native clear preserves coordinates and does not clear CostCache.
}
BnwTwoLayerUnitPathFinder::BnwTwoLayerUnitPathFinder(BnwAStarCacheControl&control,
 std::uint64_t width,std::uint64_t height,bool wx,bool wy)
 :control_(&control),width_(0),height_(0),wrap_x_(wx),wrap_y_(wy){
 need(width>0&&height>0&&width<=INT16_MAX&&height<=INT16_MAX&&
  width<=std::numeric_limits<std::size_t>::max()/height,"Native path node short coordinates/capacity unavailable");
 width_=static_cast<std::int32_t>(width);height_=static_cast<std::int32_t>(height);
 primary_.resize(static_cast<std::size_t>(width*height));partial_.resize(primary_.size());
 for(std::int32_t x=0;x<width_;++x)for(std::int32_t y=0;y<height_;++y){
  auto&a=primary(x,y);auto&b=partial(x,y);a.x=b.x=x;a.y=b.y=y;
 }
}
BnwTwoLayerUnitPathFinder::~BnwTwoLayerUnitPathFinder()=default;
void BnwTwoLayerUnitPathFinder::require_thread()const{need(creator_==std::this_thread::get_id(),"Foreign common path tree access before controls");need(!poisoned_,"Common path tree search was interrupted/retired");}
void BnwTwoLayerUnitPathFinder::fence()const{require_thread();need(busy_&&callbacks_&&callbacks_->require_current,"Missing actual Core path dispatch");callbacks_->require_current();require_thread();
 need(expected_control_&&control_&&control_->mp_cache_safe_==expected_control_->mp_cache_safe_&&
  control_->force_reset_==expected_control_->force_reset_&&control_->data_invalidates_==expected_control_->data_invalidates_&&
  control_->data_==expected_control_->data_&&control_->start_x_==expected_control_->start_x_&&
  control_->start_y_==expected_control_->start_y_&&control_->info_==expected_control_->info_,
  "Actual common cache controls changed inside native path callback");}
bool BnwTwoLayerUnitPathFinder::valid_coordinates(std::int32_t x,std::int32_t y)const noexcept{return x>=0&&x<width_&&y>=0&&y<height_;}
bool BnwTwoLayerUnitPathFinder::destination(std::int32_t x,std::int32_t y)const noexcept{return x==destination_x_&&y==destination_y_;}
BnwTwoLayerUnitPathFinder::Node&BnwTwoLayerUnitPathFinder::primary(std::int32_t x,std::int32_t y){need(valid_coordinates(x,y),"Common path node outside actual map");return primary_[static_cast<std::size_t>(x)*height_+y];}
BnwTwoLayerUnitPathFinder::Node&BnwTwoLayerUnitPathFinder::partial(std::int32_t x,std::int32_t y){need(valid_coordinates(x,y),"Common partial node outside actual map");return partial_[static_cast<std::size_t>(x)*height_+y];}
void BnwTwoLayerUnitPathFinder::clear_lists()noexcept{
 while(open_){auto*n=open_->next;open_->clear();open_=n;}
 while(closed_){auto*n=closed_->next;closed_->clear();closed_=n;}
 best_=stack_=nullptr;
 // Native GeneratePath replaces open tail with the source start node below.
}
void BnwTwoLayerUnitPathFinder::notify_open(Node&n){
 fence();
 if(n.moves>0&&!destination(n.x,n.y)){
  const auto needed=callbacks_->partial_required(n);fence();
  if(needed){auto&p=partial(n.x,n.y);
   // Native PathNodeAdd writes its second layer immediately, with parent
   // inherited from the primary node. Its total is deliberately copied.
   p.moves=0;p.turns=n.turns;p.heuristic=n.heuristic;
   p.known=sum(n.known,product(1000,n.moves));p.total=n.total;
   p.x=n.x;p.y=n.y;p.parent=n.parent;p.list=List::open;p.cache=n.cache;
   add_open(p);
  }
 }
 fence();
}
void BnwTwoLayerUnitPathFinder::add_open(Node&n){
 n.list=List::open;
 if(!open_){open_=tail_=&n;n.next=n.previous=nullptr;notify_open(n);return;}
 if(n.total<=open_->total){n.next=open_;open_->previous=&n;open_=&n;notify_open(n);return;}
 if(n.total>=tail_->total){n.previous=tail_;tail_->next=&n;tail_=&n;notify_open(n);return;}
 // The choice of traversal direction determines native equal-cost order.
 if(std::llabs(std::int64_t(n.total)-tail_->total)<std::llabs(std::int64_t(n.total)-open_->total)){
  Node*node=tail_;Node*next=nullptr;
  while(node){if(n.total<node->total){next=node;node=node->previous;}
   else{if(next){next->previous=&n;n.next=next;n.previous=node;node->next=&n;if(node->next==nullptr)tail_=node;}
    else{n.previous=tail_;tail_->next=&n;tail_=&n;}notify_open(n);return;}}
  need(next,"Native open-list tail walk lost start");next->previous=&n;n.next=next;open_=&n;notify_open(n);
 }else{
  Node*node=open_;Node*previous=nullptr;
  while(node){if(n.total>node->total){previous=node;node=node->next;}
   else{if(previous){previous->next=&n;n.previous=previous;n.next=node;node->previous=&n;if(node->next==nullptr)tail_=node;}
    else{n.next=open_;open_->previous=&n;open_=&n;}notify_open(n);return;}}
  need(previous,"Native open-list start walk lost tail");previous->next=&n;n.previous=previous;tail_=&n;notify_open(n);
 }
}
void BnwTwoLayerUnitPathFinder::update_open(Node&n){
 need(n.list==List::open,"Native UpdateOpenNode requires OPEN");
 if(n.previous&&n.total<n.previous->total){
  n.previous->next=n.next;if(n.next)n.next->previous=n.previous;else tail_=n.previous;
  auto*p=n.previous;while(p&&n.total<p->total)p=p->previous;
  if(p){n.next=p->next;n.previous=p;if(p->next)p->next->previous=&n;p->next=&n;}
  else{n.next=open_;n.previous=nullptr;if(n.next)n.next->previous=&n;open_=&n;}
 }
}
BnwTwoLayerUnitPathFinder::Node*BnwTwoLayerUnitPathFinder::get_best(){
 if(!open_)return nullptr;auto*n=open_;open_=n->next;
 if(open_)open_->previous=nullptr;else tail_=nullptr;
 // DELETEOPEN has no PathNodeAdd effect, then native CLOSED prepend.
 n->list=List::closed;n->next=closed_;if(closed_)closed_->previous=n;closed_=n;
 return n;
}
void BnwTwoLayerUnitPathFinder::link_child(Node&parent,Node&node){
 fence();const auto edge=callbacks_->cost(parent,node);fence();
 const auto known=sum(parent.known,edge);
 if(node.list==List::open){parent.children.push_back(&node);
  if(known<node.known){need(parent.parent!=&node,"Native path parent back edge");node.parent=&parent;node.known=known;node.total=sum(known,node.heuristic);update_open(node);callbacks_->notify_child(&parent,node,ChildNotice::open_update);fence();}
 }else if(node.list==List::closed){parent.children.push_back(&node);
  if(known<node.known){need(parent.parent!=&node,"Native path parent back edge");node.parent=&parent;node.known=known;node.total=sum(known,node.heuristic);callbacks_->notify_child(&parent,node,ChildNotice::closed_update);fence();update_parents(node);}
 }else{need(node.list==List::none&&parent.parent!=&node,"Native new path child state differs");node.parent=&parent;node.known=known;
  node.heuristic=callbacks_->heuristic?callbacks_->heuristic(node.x,node.y,destination_x_,destination_y_):0;fence();node.total=sum(node.known,node.heuristic);
  callbacks_->notify_child(&parent,node,ChildNotice::new_node);fence();add_open(node);parent.children.push_back(&node);
 }
 need(parent.children.size()<=INT16_MAX,"Native path child short overflow");
}
void BnwTwoLayerUnitPathFinder::create_children(Node&node){
 constexpr std::array<std::int32_t,6>dx{0,1,1,0,-1,-1},dy{1,0,-1,-1,0,1};
 for(std::size_t i=0;i<6;++i){auto x=node.x-half(node.y)+dx[i];auto y=ranged(node.y+dy[i],height_,wrap_y_);x=ranged(x+half(y),width_,wrap_x_);
  if(valid_coordinates(x,y)){auto&child=primary(x,y);fence();const auto allowed=callbacks_->valid(&node,child);fence();if(allowed)link_child(node,child);}
 }
}
void BnwTwoLayerUnitPathFinder::push(Node&n){
 if(n.on_stack)return;
 // Native empty-stack branch chooses the primary coordinate node, even if
 // the improved child was a partial-layer node. Do not replace this quirk.
 if(!stack_)stack_=&primary(n.x,n.y);
 else{need(n.stack==nullptr,"Native path stack link already occupied");n.stack=stack_;stack_=&n;}
 n.on_stack=true;
}
BnwTwoLayerUnitPathFinder::Node*BnwTwoLayerUnitPathFinder::pop(){if(!stack_)return nullptr;auto*n=stack_;stack_=n->stack;n->stack=nullptr;n->on_stack=false;return n;}
void BnwTwoLayerUnitPathFinder::update_parents(Node&node){
 need(stack_==nullptr,"Native UpdateParents requires empty stack");auto*parent=&node;
 while(parent){const auto count=parent->children.size();for(std::size_t i=0;i<count;++i){auto*child=parent->children[i];fence();const auto edge=callbacks_->cost(*parent,*child);fence();const auto known=sum(parent->known,edge);
   if(known<child->known){child->known=known;child->total=sum(known,child->heuristic);need(parent->parent!=child,"Native path improved child back edge");child->parent=parent;if(child->list==List::open)update_open(*child);callbacks_->notify_child(parent,*child,ChildNotice::parent_update);fence();push(*child);}}
  parent=pop();
 }
}
bool BnwTwoLayerUnitPathFinder::generate(std::uint64_t unit,std::int32_t sx,std::int32_t sy,
 std::int32_t dx,std::int32_t dy,std::int32_t info,bool reuse,bool multiplayer,const Callbacks&callbacks){
 require_thread();if(busy_){poisoned_=true;throw BnwCommonPathfinderUnavailable("Common path search reentry");}
 need(callbacks.require_current&&callbacks.initialize&&callbacks.uninitialize&&callbacks.destination_valid&&callbacks.valid&&callbacks.cost&&callbacks.notify_child&&callbacks.partial_required,"Actual native unit path callbacks are incomplete");
 busy_=true;callbacks_=&callbacks;expected_control_=*control_;
 struct Retire{BnwTwoLayerUnitPathFinder&tree;bool normal=false;~Retire(){tree.callbacks_=nullptr;tree.expected_control_.reset();tree.busy_=false;if(!normal){tree.poisoned_=true;tree.control_->force_reset_cache();}}}scope{*this};
 fence();control_->set_data(unit);reuse=control_->permit_requested_reuse(reuse,multiplayer,sx,sy,info);
 destination_x_=dx;destination_y_=dy;control_->start_x_=sx;control_->start_y_=sy;control_->info_=info;expected_control_=*control_;
 callbacks.initialize();fence();
 const auto finish=[&](bool found){callbacks.uninitialize();fence();scope.normal=true;return found;};
 if(!valid_coordinates(sx,sy))return finish(false);
 if(!reuse){clear_lists();control_->force_reset_=false;expected_control_=*control_;auto&start=primary(sx,sy);start.known=0;start.heuristic=callbacks.heuristic?callbacks.heuristic(sx,sy,dx,dy):0;fence();start.total=sum(start.known,start.heuristic);open_=tail_=&start;
  notify_open(start);(void)callbacks.valid(nullptr,start);fence();callbacks.notify_child(nullptr,start,ChildNotice::initial);fence();}
 const auto accepted=callbacks.destination_valid(dx,dy);fence();if(!accepted)return finish(false);
 if(valid_coordinates(dx,dy)){auto&target=primary(dx,dy);if(target.list==List::closed){best_=&target;return finish(true);}}
 for(;;){best_=get_best();if(!best_)return finish(false);create_children(*best_);if(!best_)return finish(false);if(destination(best_->x,best_->y))return finish(true);}
}
}
