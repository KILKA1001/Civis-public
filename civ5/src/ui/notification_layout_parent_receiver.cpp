#include "civ5/ui/notification_layout_parent_receiver.hpp"
#include "civ5/ui/detail/notification_layout_baseline.hpp"
#include "civ5/ui/detail/notification_layout_scroll_receiver.hpp"
#include "civ5/ui/detail/notification_layout_image_state.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace civ5::ui::detail {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
float add(float a,float b){volatile float v=a+b;need(std::isfinite(v),"Layout receiver FP32 addition overflow");return v;}
float sub(float a,float b){volatile float v=a-b;need(std::isfinite(v),"Layout receiver FP32 subtraction overflow");return v;}
float mul(float a,float b){volatile float v=a*b;need(std::isfinite(v),"Layout receiver FP32 multiplication overflow");return v;}
float absolute_delta(float a,float b){const auto v=static_cast<float>(std::fabs(static_cast<long double>(a)-b));need(std::isfinite(v),"Layout receiver x87 delta overflow");return v;}
template<class A>void finite(const A& a){for(float v:a)need(std::isfinite(v),"Layout receiver nonfinite native field");}
float centered(float parent,float own,float margin){
    const float half=mul(sub(parent,own),0.5F);
    const double rounded=half>=0?std::floor(static_cast<double>(half)+0.5):std::ceil(static_cast<double>(half)-0.5);
    const auto narrowed=static_cast<float>(rounded);need(std::isfinite(narrowed),"Layout receiver centered coordinate overflow");
    return add(margin,narrowed);
}
template<class T>struct PathExit {std::vector<T>& path;~PathExit(){path.pop_back();}};
}
const NotificationLayoutParentReceiver::NodeView&
NotificationLayoutParentReceiver::node(const TreeView& v,NodeKey id){
    v.require_pure();
    const auto n=std::find_if(v.nodes_.begin(),v.nodes_.end(),[&](const auto& a){return a.key==id;});
    need(n!=v.nodes_.end()&&n->state,"Actual canonical layout receiver node absent");return *n;
}
std::vector<NotificationLayoutParentReceiver::NodeKey> NotificationLayoutParentReceiver::children(const TreeView& v,NodeKey id){
    v.require_children_owned(id);
    std::vector<const NodeView*> ordered;
    const auto& parent=*node(v,id).state;
    if(parent.kind_==NotificationLayoutBaseline::Kind::scroll_panel)
        need(parent.scroll_&&parent.scroll_->handles_c8_cc_d0,
            "Actual Scroll child-vector/special-handle distinction unissued");
    for(const auto& n:v.nodes_)if(n.parent&&*n.parent==id){
        if(parent.kind_==NotificationLayoutBaseline::Kind::scroll_panel){
            const auto& handles=*parent.scroll_->handles_c8_cc_d0;
            if(n.key.domain==id.domain&&n.key.id!=0&&std::find(handles.begin(),handles.end(),n.key.id)!=handles.end())continue;
        }
        ordered.push_back(&n);
    }
    std::sort(ordered.begin(),ordered.end(),[](const auto* a,const auto* b){return a->attached_order<b->attached_order;});
    std::vector<NodeKey> result;result.reserve(ordered.size());std::uint64_t prior=0;
    for(const auto* n:ordered){need(n->attached_order>prior,"Actual native child attachment order repeated");prior=n->attached_order;result.push_back(n->key);}
    return result;
}
std::array<float,2> NotificationLayoutParentReceiver::base_anchor_coordinates(
    const NotificationLayoutBaseline& actual,const NotificationLayoutBaseline* parent){
    const auto& b=actual.base_;finite(b.margin_70_74);finite(b.size_30_34);
    auto result=b.margin_70_74; // 79D300 stores70/74 into28/2c BEFORE parent branch.
    if(!parent)return result; // Only a genuine closed parent-NULL observation can reach this call.
    auto size=parent->base_.size_30_34;
    if(parent->kind_==NotificationLayoutBaseline::Kind::scroll_panel&&!(b.flags_40&0x240U)){
        need(parent->scroll_.has_value(),"Actual ScrollPanel type12 layout state absent");size=parent->scroll_->calculated_84_88;
    }
    finite(size);
    if(b.flags_40&0x20U){
        if(b.flags_40&0x40U)result[0]=add(result[0],size[0]);
        else result[0]=sub(sub(size[0],b.size_30_34[0]),mul(result[0],1.0F));
    }else if(b.flags_40&0x10U)result[0]=centered(size[0],b.size_30_34[0],result[0]);
    else if(b.flags_40&0x40U)result[0]=sub(mul(result[0],-1.0F),b.size_30_34[0]);
    if(b.flags_40&0x100U){
        if(b.flags_40&0x200U)result[1]=add(result[1],size[1]);
        else result[1]=sub(sub(size[1],b.size_30_34[1]),mul(result[1],1.0F));
    }else if(b.flags_40&0x80U)result[1]=centered(size[1],b.size_30_34[1],result[1]);
    else if(b.flags_40&0x200U)result[1]=sub(mul(result[1],-1.0F),b.size_30_34[1]);
    return result;
}
std::array<float,4> NotificationLayoutParentReceiver::base_extent_bounds(
    const NotificationLayoutBaseline& actual,std::span<const NotificationLayoutBaseline*const> actual_children){
    const auto& b=actual.base_;finite(b.offset_28_2c);finite(b.size_30_34);
    std::array<float,4> result{b.offset_28_2c[0],b.offset_28_2c[1],add(b.offset_28_2c[0],b.size_30_34[0]),add(b.offset_28_2c[1],b.size_30_34[1])};
    // 6EBC30 unions every actual child in forward order. No hidden/0x240 filter.
    for(const auto* child:actual_children){need(child,"Actual extent child absent");finite(child->base_.bounds_10_1c);
        result[0]=std::min(result[0],child->base_.bounds_10_1c[0]);result[1]=std::min(result[1],child->base_.bounds_10_1c[1]);
        result[2]=std::max(result[2],child->base_.bounds_10_1c[2]);result[3]=std::max(result[3],child->base_.bounds_10_1c[3]);}
    return result;
}
void NotificationLayoutParentReceiver::base_anchor(TreeView& v,NodeKey id){
    v.require_full();v.require_pure();const auto& n=node(v,id);
    const auto* parent=n.parent?node(v,*n.parent).state:nullptr;
    const auto result=base_anchor_coordinates(*n.state,parent);
    v.require_full();v.require_pure();auto& state=*node(v,id).state;
    v.preflight_store_owned(id);state.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);state.base_.offset_28_2c=result;
    v.record_store_owned(id,Store::Kind::base_anchor);v.require_full();v.require_pure();
    node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutParentReceiver::stack_anchor(TreeView& v,NodeKey id){
    v.require_pure();need(node(v,id).state->stack_.has_value(),"Actual Stack anchor state absent");
    if(node(v,id).state->stack_->field_84!=1||node(v,id).state->stack_->field_8c!=0){
        //5895E0 snapshots BOTH direction operands BEFORE actual79D300.
        constexpr std::array<std::array<float,2>,4> directions{{{0,1},{0,-1},{-1,0},{1,0}}};
        const auto growth=node(v,id).state->stack_->field_84,wrap=node(v,id).state->stack_->field_88;
        const auto limit=node(v,id).state->stack_->field_8c;
        need(growth<directions.size()&&wrap<directions.size(),"Actual Stack native direction table index unsupported");
        const auto first=directions[growth],second=directions[wrap];
        base_anchor(v,id);v.require_pure();
        // Native first90/94 stores occur before either projected correction.
        // Keep those real partial stores if any later mandatory receiver fails.
        auto& initial=*node(v,id).state;
        need(initial.stack_&&initial.stack_->field_84==growth&&initial.stack_->field_88==wrap&&initial.stack_->field_8c==limit,
            "Stack directions changed during actual base anchoring");
        finite(initial.base_.size_30_34);finite(initial.base_.offset_28_2c);
        const auto size=initial.base_.size_30_34;
        v.preflight_store_owned(id);initial.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
        initial.stack_->field_90=initial.base_.offset_28_2c[0];initial.stack_->field_94=initial.base_.offset_28_2c[1];
        v.record_store_owned(id,Store::Kind::stack_anchor);v.require_pure();
        const float along=add(mul(first[1],size[1]),mul(size[0],first[0]));
        const float across=add(mul(size[1],second[1]),mul(size[0],second[0]));
        if(along<0){auto& current=*node(v,id).state;
            const float x=add(mul(first[0],along),current.stack_->field_90);
            const float y=add(current.stack_->field_94,mul(first[1],along));
            v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
            current.stack_->field_90=x;current.stack_->field_94=y;v.record_store_owned(id,Store::Kind::stack_anchor);v.require_pure();
        }
        // Signed WORD comparison; zero and negative limits never wrap.
        if(limit>0&&across<0){auto& current=*node(v,id).state;
            const float x=add(current.stack_->field_90,mul(second[0],across));
            const float y=add(mul(second[1],across),current.stack_->field_94);
            v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
            current.stack_->field_90=x;current.stack_->field_94=y;v.record_store_owned(id,Store::Kind::stack_anchor);v.require_pure();
        }
        v.require_full();v.require_pure();node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);return;
    }
    need(node(v,id).state->stack_->field_84==1&&node(v,id).state->stack_->field_8c==0,
        "Actual selected Stack anchor is not Top/noWrap");
    base_anchor(v,id);v.require_pure();auto& state=*node(v,id).state;
    auto origin=state.base_.offset_28_2c;
    const float xproj=mul(state.base_.size_30_34[0],0.0F);
    const float yproj=mul(state.base_.size_30_34[1],-1.0F);
    const float projected=add(yproj,xproj);
    if(projected<0){origin[0]=add(origin[0],mul(0.0F,projected));origin[1]=add(origin[1],mul(-1.0F,projected));}
    v.require_full();v.require_pure();auto& current=*node(v,id).state;
    v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
    current.stack_->field_90=origin[0];current.stack_->field_94=origin[1];
    v.record_store_owned(id,Store::Kind::stack_anchor);v.require_full();v.require_pure();
    node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutParentReceiver::stack_calculate(TreeView& v,NodeKey id){
    v.require_full();v.require_pure();const auto& n=node(v,id);auto& state=*n.state;
    need(state.stack_.has_value(),"Actual selected Stack size state absent");
    if(state.stack_->field_84!=1||state.stack_->field_8c!=0){
        constexpr std::array<std::array<float,2>,4> directions{{{0,1},{0,-1},{-1,0},{1,0}}};
        const auto growth=state.stack_->field_84,wrap=state.stack_->field_88;
        const std::int16_t limit=state.stack_->field_8c;
        need(growth<directions.size()&&wrap<directions.size(),"Stack CalculateSize actual direction index unsupported");
        const auto first=directions[growth],second=directions[wrap];
        const auto zero=v.arithmetic_pair_owned();finite(zero);
        //4D5110 cursor20/24 is literal XORPS zero. 4AE8D0 scratch reset is
        //earned iff that cursor equals the mutable actual GUI namespace pair.
        //This normalized selected route closes that proved reset branch only.
        need(zero[0]==0.0F&&zero[1]==0.0F,"Actual wrap/nonTop Stack requires genuine zero arithmetic namespace; nonzero branch unsupported");
        const auto old_bounds=state.base_.bounds_10_1c;finite(old_bounds);finite(state.base_.offset_28_2c);finite(state.base_.margin_70_74);
        const float px=state.stack_->field_98,py=state.stack_->field_9c;
        need(std::isfinite(px)&&std::isfinite(py),"Actual Stack wrap padding nonfinite");
        auto bounds=std::array<float,4>{std::fabs(state.base_.offset_28_2c[0]),std::fabs(state.base_.offset_28_2c[1]),0,0};
        bounds[2]=add(bounds[0],zero[0]);bounds[3]=add(bounds[1],zero[1]);
        auto extents=std::array<float,4>{zero[0],zero[1],zero[0],zero[1]};
        auto cursor=std::array<float,2>{0.0F,0.0F}; // actual4D516A XORPS20/24
        // Actual native initial30/34 and10..1C stores precede the child vector.
        v.preflight_store_owned(id);state.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
        state.base_.size_30_34={0.0F,0.0F};state.base_.bounds_10_1c=bounds;
        v.record_store_owned(id,Store::Kind::stack_size);v.require_pure();
        struct Scratch {std::array<float,2>row,maximum;bool wrapped;};
        std::optional<Scratch> scratch; // no guessed static/lazy namespace data
        const auto unite=[](auto& box,const auto& child,const auto& offset){const std::array<float,4> translated{
            add(child[0],offset[0]),add(child[1],offset[1]),add(child[2],offset[0]),add(child[3],offset[1])};
            box[0]=std::min(box[0],translated[0]);box[1]=std::min(box[1],translated[1]);box[2]=std::max(box[2],translated[2]);box[3]=std::max(box[3],translated[3]);};
        // Full actual ordered vector is mandatory, including a genuine empty
        // vector. No default children, hidden omission, callback or recursive
        // parent dispatch is inserted inside this native scratch interval.
        for(const auto child_id:children(v,id)){const auto& child=node(v,child_id).state->base_;
            finite(child.offset_28_2c);finite(child.size_30_34);finite(child.extent_20_24);finite(child.bounds_10_1c);
            auto position=zero; //4AE933/937 actual output receives namespace pair
            if(cursor==zero)scratch.emplace(Scratch{zero,zero,false});
            need(scratch.has_value(),"Stack child scratch actual cursor reset not earned");
            if(!(child.flags_40&0x242U)&&child.size_30_34[0]!=0.0F&&child.size_30_34[1]!=0.0F){
                const float xp=add(px,add(child.size_30_34[0],child.offset_28_2c[0]));
                const float yp=add(py,add(child.size_30_34[1],child.offset_28_2c[1]));
                const float projected=add(mul(yp,first[1]),mul(xp,first[0]));
                const float magnitude=std::fabs(projected);
                const std::array<float,2>step{mul(magnitude,first[0]),mul(magnitude,first[1])};
                position=cursor;
                if(limit>0){
                    const float candidate_x=add(cursor[0],step[0]),candidate_y=add(cursor[1],step[1]);
                    const float candidate_yp=mul(candidate_y,first[1]),candidate_xp=mul(candidate_x,first[0]);
                    //4AEAB6..D9: two narrowed SSE products then x87 FADD/FABS
                    //compare against sign-extended WORD, not rounded ADDSS.
                    const long double distance=std::fabs(static_cast<long double>(candidate_yp)+candidate_xp);
                    need(std::isfinite(distance),"Stack native wrap-threshold extended overflow");
                    if(distance>static_cast<long double>(limit)){
                        scratch->row=scratch->maximum;cursor=scratch->row;position=cursor;scratch->wrapped=true;
                    }
                    const float across=add(mul(yp,second[1]),mul(xp,second[0]));
                    if(across<0.0F){const float amount=-across;
                        const float along=add(mul(cursor[1],first[1]),mul(cursor[0],first[0]));
                        cursor[0]=add(add(mul(first[0],along),scratch->row[0]),mul(amount,second[0]));
                        cursor[1]=add(add(mul(first[1],along),scratch->row[1]),mul(second[1],amount));
                        position=cursor;
                    }
                }
                cursor[0]=add(step[0],cursor[0]);cursor[1]=add(step[1],cursor[1]);
                if(projected<0.0F)position=cursor;
                if(limit>0){
                    const float end_x=add(px,add(position[0],child.extent_20_24[0]));
                    const float end_y=add(py,add(position[1],child.extent_20_24[1]));
                    const float end_projection=add(mul(end_y,second[1]),mul(end_x,second[0]));
                    const float start_projection=add(mul(position[1],second[1]),mul(position[0],second[0]));
                    const float prior_projection=add(mul(scratch->maximum[1],second[1]),mul(scratch->maximum[0],second[0]));
                    const float candidate=start_projection>end_projection?start_projection:end_projection;
                    const float maximum=candidate>prior_projection?candidate:prior_projection;
                    scratch->maximum={mul(second[0],maximum),mul(second[1],maximum)};
                }
            }
            //4AE8D0 returns a derived offset, not child28/2C mutation.
            v.child_positions_.push_back({id,child_id,position});
            if((child.flags_40&2U)||child.size_30_34[0]==0.0F||child.size_30_34[1]==0.0F)continue;
            if(!(child.flags_40&0x240U)){const std::array<float,4> extent{zero[0],zero[1],child.extent_20_24[0],child.extent_20_24[1]};unite(extents,extent,position);}
            unite(bounds,child.bounds_10_1c,position);
        }
        const std::array<float,2> size{absolute_delta(extents[2],extents[0]),absolute_delta(extents[3],extents[1])};
        const std::array<float,2> extent{static_cast<float>(std::fabs(static_cast<long double>(state.base_.margin_70_74[0]))+size[0]),
            static_cast<float>(std::fabs(static_cast<long double>(state.base_.margin_70_74[1]))+size[1])};finite(extent);
        const std::array<float,4> new_bounds{zero[0],zero[1],absolute_delta(bounds[2],bounds[0]),absolute_delta(bounds[3],bounds[1])};
        v.require_full();v.require_pure();auto& current=*node(v,id).state;
        need(v.arithmetic_pair_owned()==zero,"Stack actual mutable arithmetic namespace changed during receiver callbacks");
        need(current.stack_&&current.stack_->field_84==growth&&current.stack_->field_88==wrap&&current.stack_->field_8c==limit&&
            current.stack_->field_98==px&&current.stack_->field_9c==py,"Stack original direction/WORD/padding changed during receiver callbacks");
        v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
        current.base_.size_30_34=size;current.base_.extent_20_24=extent;current.base_.bounds_10_1c=new_bounds;
        v.record_store_owned(id,Store::Kind::stack_size);
        if(old_bounds!=new_bounds&&node(v,id).parent)slot94(v,*node(v,id).parent);
        v.require_full();v.require_pure();node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);return;
    }
    need(state.stack_&&state.stack_->field_84==1&&state.stack_->field_8c==0,
        "Actual selected Stack size is not Top/noWrap");
    const auto zero=v.arithmetic_pair_owned();
    const auto old_bounds=state.base_.bounds_10_1c;finite(old_bounds);finite(state.base_.offset_28_2c);finite(state.base_.margin_70_74);
    const float px=state.stack_->field_98,py=state.stack_->field_9c;need(std::isfinite(px)&&std::isfinite(py),"Actual Stack padding nonfinite");
    auto bounds=std::array<float,4>{std::fabs(state.base_.offset_28_2c[0]),std::fabs(state.base_.offset_28_2c[1]),0,0};
    bounds[2]=add(bounds[0],zero[0]);bounds[3]=add(bounds[1],zero[1]);
    auto extents=std::array<float,4>{zero[0],zero[1],zero[0],zero[1]};auto cursor=zero;
    const auto unite=[](auto& box,const auto& child,const auto& offset){const std::array<float,4> translated{
        add(child[0],offset[0]),add(child[1],offset[1]),add(child[2],offset[0]),add(child[3],offset[1])};
        box[0]=std::min(box[0],translated[0]);box[1]=std::min(box[1],translated[1]);box[2]=std::max(box[2],translated[2]);box[3]=std::max(box[3],translated[3]);};
    for(const auto child_id:children(v,id)){const auto& child=node(v,child_id).state->base_;
        finite(child.offset_28_2c);finite(child.size_30_34);finite(child.extent_20_24);finite(child.bounds_10_1c);auto position=zero;
        if(!(child.flags_40&0x242U)&&child.size_30_34[0]!=0&&child.size_30_34[1]!=0){
            position=cursor;const float xp=add(px,add(child.size_30_34[0],child.offset_28_2c[0]));
            const float yp=add(py,add(child.size_30_34[1],child.offset_28_2c[1]));
            const float projected=add(mul(yp,-1.0F),mul(xp,0.0F));const float absolute=std::fabs(projected);
            cursor[0]=add(cursor[0],mul(absolute,0.0F));cursor[1]=add(cursor[1],mul(absolute,-1.0F));if(projected<0)position=cursor;
        }
        // Derived Stack positions are kept separate from native child28/2c.
        v.child_positions_.push_back({id,child_id,position});
        if((child.flags_40&2U)||child.size_30_34[0]==0||child.size_30_34[1]==0)continue;
        if(!(child.flags_40&0x240U)){const std::array<float,4> extent{zero[0],zero[1],child.extent_20_24[0],child.extent_20_24[1]};unite(extents,extent,position);}
        unite(bounds,child.bounds_10_1c,position);
    }
    const std::array<float,2> size{absolute_delta(extents[2],extents[0]),absolute_delta(extents[3],extents[1])};
    const std::array<float,2> extent{static_cast<float>(std::fabs(static_cast<long double>(state.base_.margin_70_74[0]))+size[0]),
        static_cast<float>(std::fabs(static_cast<long double>(state.base_.margin_70_74[1]))+size[1])};finite(extent);
    const std::array<float,4> new_bounds{zero[0],zero[1],absolute_delta(bounds[2],bounds[0]),absolute_delta(bounds[3],bounds[1])};
    v.require_full();v.require_pure();auto& current=*node(v,id).state;
    v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
    current.base_.size_30_34=size;current.base_.extent_20_24=extent;current.base_.bounds_10_1c=new_bounds;
    v.record_store_owned(id,Store::Kind::stack_size);
    // Native4D5110 changed bounds invokes the actual parent's slot94.
    if(old_bounds!=new_bounds&&node(v,id).parent)slot94(v,*node(v,id).parent);
    v.require_full();v.require_pure();node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutParentReceiver::base_extent94(TreeView& v,NodeKey id){
    v.require_full();v.require_pure();std::vector<const NotificationLayoutBaseline*> actual_children;
    for(const auto child:children(v,id))actual_children.push_back(node(v,child).state);
    const auto old=node(v,id).state->base_.bounds_10_1c;finite(old);const auto next=base_extent_bounds(*node(v,id).state,actual_children);
    v.require_full();v.require_pure();auto& current=*node(v,id).state;
    v.preflight_store_owned(id);current.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);current.base_.bounds_10_1c=next;
    v.record_store_owned(id,Store::Kind::bounds_union);
    if(old!=next&&node(v,id).parent)slot94(v,*node(v,id).parent);
    v.require_full();v.require_pure();node(v,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutParentReceiver::slot94(TreeView& v,NodeKey id){
    v.require_pure();need(std::find(v.parent_path_.begin(),v.parent_path_.end(),id)==v.parent_path_.end(),"Actual parent slot94 cycle/reentry");
    v.parent_path_.push_back(id);PathExit exit{v.parent_path_};
    if(node(v,id).state->kind_==NotificationLayoutBaseline::Kind::base)
        need(node(v,id).base_dispatch==BaseDispatch::base_4c_and_94,"Actual subtype base slot94 observation unissued");
    const auto kind=node(v,id).state->kind_;
    if(kind==NotificationLayoutBaseline::Kind::button){
        const auto& actual=*node(v,id).state;
        need(actual.button_&&(actual.actual_type_=="UpButton"||actual.actual_type_=="DownButton"||actual.actual_type_=="Button"),
            "Actual A3C9C0 Button slot94 state absent or substituted");
        // Actual selected A3C9C0+94 dispatches6EBC30; no generic Button fallback.
    }
    if(kind==NotificationLayoutBaseline::Kind::container)
        need(node(v,id).state->actual_type_=="Container","Actual Container6EBC30 constructor variant substituted");
    if(kind==NotificationLayoutBaseline::Kind::grid)
        need(node(v,id).state->actual_type_=="Grid"&&node(v,id).state->grid_,"Actual Grid6EBC30 constructor variant absent");
    if(kind==NotificationLayoutBaseline::Kind::image)
        need(node(v,id).state->actual_type_=="Image"&&node(v,id).state->image_,"Actual Image6EBC30 constructor variant absent");
    if(kind==NotificationLayoutBaseline::Kind::box)v.require_box_native_common_return_owned(id);
    need(kind==NotificationLayoutBaseline::Kind::grid||kind==NotificationLayoutBaseline::Kind::box||kind==NotificationLayoutBaseline::Kind::image||kind==NotificationLayoutBaseline::Kind::container||kind==NotificationLayoutBaseline::Kind::base||kind==NotificationLayoutBaseline::Kind::button||
        kind==NotificationLayoutBaseline::Kind::context||kind==NotificationLayoutBaseline::Kind::stack||
        kind==NotificationLayoutBaseline::Kind::scroll_panel||kind==NotificationLayoutBaseline::Kind::slider,
        "Actual subtype extent receiver mapping unissued");
    if(node(v,id).state->kind_==NotificationLayoutBaseline::Kind::stack)stack_calculate(v,id);
    // Proven Stack76E380 tail, Context/base/Scroll/Slider94 all call6EBC30.
    base_extent94(v,id);v.require_pure();
    if(kind==NotificationLayoutBaseline::Kind::box)v.require_box_native_common_return_owned(id);
}
void NotificationLayoutParentReceiver::slot4c(TreeView& v,NodeKey id){
    v.require_full();v.require_pure();
    switch(node(v,id).state->kind_){
      case NotificationLayoutBaseline::Kind::stack:stack_anchor(v,id);break;
      case NotificationLayoutBaseline::Kind::scroll_panel:need(id.domain==0,"External Scroll receiver domain requires its genuine domain-scoped dispatcher");NotificationLayoutScrollReceiver::slot4c(v,id.id);break;
      case NotificationLayoutBaseline::Kind::base:
        need(node(v,id).base_dispatch==BaseDispatch::base_4c_and_94,"Actual subtype base slot4C observation unissued");
        base_anchor(v,id);break;
      case NotificationLayoutBaseline::Kind::button:{
        const auto& actual=*node(v,id).state;
        need(actual.button_&&(actual.actual_type_=="UpButton"||actual.actual_type_=="DownButton"||actual.actual_type_=="Button"),
            "Actual A3C9C0 Button slot4C state absent or substituted");
        // Actual selected A3C9C0+4C dispatches79D300. Keep distinct Button state.
        base_anchor(v,id);break;
      }
      case NotificationLayoutBaseline::Kind::grid:
        need(node(v,id).state->actual_type_=="Grid"&&node(v,id).state->grid_,"Actual Grid79D300 constructor variant absent");
        base_anchor(v,id);break;
      case NotificationLayoutBaseline::Kind::image:
        need(node(v,id).state->actual_type_=="Image"&&node(v,id).state->image_,"Actual Image79D300 constructor variant absent");
        base_anchor(v,id);break;
      case NotificationLayoutBaseline::Kind::box:{
        v.require_box_native_common_return_owned(id);base_anchor(v,id);
        v.require_box_native_common_return_owned(id);break;
      }
      case NotificationLayoutBaseline::Kind::container:
        need(node(v,id).state->actual_type_=="Container","Actual Container79D300 constructor variant substituted");
        base_anchor(v,id);break;
      case NotificationLayoutBaseline::Kind::context:
      case NotificationLayoutBaseline::Kind::slider:base_anchor(v,id);break;
      default:need(false,"Actual subtype anchoring receiver mapping unissued");
    }
    v.require_full();v.require_pure();v.record_four_c_return_owned(id);
}
void NotificationLayoutParentReceiver::reprocess_anchoring(TreeView& v,NodeKey id){
    v.require_pure();need(std::find(v.anchor_path_.begin(),v.anchor_path_.end(),id)==v.anchor_path_.end(),"Actual child anchor cycle/reentry");
    v.anchor_path_.push_back(id);PathExit exit{v.anchor_path_};
    slot4c(v,id);
    v.require_pure();slot94(v,id);v.require_pure();
    const auto ordered=children(v,id);
    for(const auto child:ordered){v.require_pure();reprocess_anchoring(v,child);v.require_pure();}
    v.require_full();v.require_pure();
}
}
