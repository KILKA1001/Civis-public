#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/script_ui_control_graph_backend.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace civ5::runtime {
class BnwWorldViewScriptUiAdapter;enum class BnwUnitFlagHealthCommand;
// Supporting authored graph only. The registering selected-source owner must
// check its actual resource/session/window lease in validate. Neither XML nor a
// graph handle grants activation, native geometry or rendered publication.
class ScriptUiControlGraph final {
 public:
    using Id = std::uint64_t;
    struct Node { Id id=0,parent=0; content::XmlNode authored; bool hidden=false,alive=true,dynamic=false; };
    struct Named {std::string name; Id id;};
    ~ScriptUiControlGraph(){layout_lifetime_->alive=false;}
    ScriptUiControlGraph(const ScriptUiControlGraph&)=delete;
    ScriptUiControlGraph& operator=(const ScriptUiControlGraph&)=delete;
    ScriptUiControlGraph(ScriptUiControlGraph&&)=delete;
    ScriptUiControlGraph& operator=(ScriptUiControlGraph&&)=delete;
    ScriptUiControlGraph(std::string context,content::XmlNode document,std::function<void()> validate)
      :context_(std::move(context)),validate_(std::move(validate)) {
        if(context_.empty()||context_.find('\0')!=context_.npos||!validate_||document.name!="Context")
            throw std::runtime_error("Authored graph needs a context, parsed Context and actual owner validator");
        require_current();
        for(auto &child:document.children) {
            if(child.name=="Instance") {
                const auto name=child.attribute("Name");
                if(name.empty()||name.find('\0')!=name.npos||child.children.size()!=1||!templates_.emplace(name,child.children[0]).second)
                    throw std::runtime_error("Authored instance name/root missing or duplicated");
                std::vector<Named> ids; std::vector<Node> scratch;
                append(scratch,child.children[0],0,true,ids);
            } else append(nodes_,child,0,false,static_);
        }
        require_current();
    }
    const std::string &context()const {return context_;}
    void require_current()const {
        if(std::this_thread::get_id()!=thread_||retired_||poisoned_||busy_)
            throw std::runtime_error("UI graph thread/lifetime/reentry refused");
        // Guard can call native code; reentry is refused before that borrow.
        busy_=true;
        try {if(backend_)backend_->require_current();else validate_();busy_=false;} catch(...) {busy_=false;throw;}
        if(retired_||poisoned_)throw std::runtime_error("UI graph retired during owner validation");
    }
    void retire() {require_thread();retired_=true;if(backend_)backend_->retire();}
    void poison() {require_thread();poisoned_=true;if(backend_)backend_->poison();}
    std::vector<Named> static_controls()const {require_current();if(backend_)return named(backend_->static_controls());return static_;}
    Node inspect(Id id)const {require_current();if(backend_){const auto n=backend_->inspect(id);return {n.id,n.parent,n.authored,n.hidden,n.alive,n.dynamic};}const auto result=find(nodes_,id);require_current();return result;}
    std::vector<Named> build(std::string_view name,Id parent=0) {
        require_current();
        if(backend_)return named(backend_->build(name,parent));
        if(parent)find(nodes_,parent);
        const auto found=templates_.find(std::string(name));
        if(found==templates_.end())throw std::runtime_error("Undeclared authored instance");
        std::vector<Named> result;
        mutate([&](auto &next){append(next,found->second,parent,true,result);});
        return result;
    }
    void set_hidden(Id id,bool value) {require_current();if(backend_){backend_->set_hidden(id,value);return;}find(nodes_,id);mutate([&](auto &next){find(next,id).hidden=value;});}
    void change_parent(Id id,Id parent) {
        require_current();if(backend_){backend_->change_parent(id,parent);return;}const auto &node=find(nodes_,id);
        if(!node.dynamic)throw std::runtime_error("Static authored control cannot be reparented by dynamic API");
        for(auto p=parent;p;p=find(nodes_,p).parent)if(p==id)throw std::runtime_error("UI parent cycle refused");
        mutate([&](auto &next){find(next,id).parent=parent;});
    }
    void destroy_child(Id parent,Id id) {
        require_current();if(backend_){backend_->destroy_child(parent,id);return;}const auto &node=find(nodes_,id);if(parent)find(nodes_,parent);
        if(!node.dynamic||node.parent!=parent)throw std::runtime_error("DestroyChild requires this parent's dynamic child");
        mutate([&](auto &next){
            std::vector<Id> removed{id};
            for(std::size_t i=0;i<removed.size();++i)
                for(const auto &child:next)if(child.alive&&child.parent==removed[i])removed.push_back(child.id);
            for(auto dead:removed)find(next,dead).alive=false;
        });
    }
    void set_alpha(Id id,double value){require_backend();backend_->set_alpha(id,value);}
    void set_disabled(Id id,bool value){require_backend();backend_->set_disabled(id,value);}
 private:
    template<class F> auto layout_owned(F&& operation)const {
        const auto life=layout_lifetime_;
        const auto first=[&]{
            // Foreign callers do not read mutable Graph/backend fields or
            // invoke the backend; only independent atomic failure is written.
            if(life->creator!=std::this_thread::get_id()){
                life->failed.store(true);throw std::runtime_error("Layout graph foreign creator before saved borrow");
            }
            if(!life->alive.load()||life->failed.load())
                throw std::runtime_error("Layout graph facade retired/failed before saved borrow");
        };
        first();const auto backend=backend_;
        const auto pure=[&](bool active){
            first();const auto& actual=*life->owner;
            if(actual.thread_!=life->creator||actual.retired_||actual.poisoned_||actual.busy_!=active||
               !backend||actual.backend_.get()!=backend.get()||actual.backend_.owner_before(backend)||backend.owner_before(actual.backend_))
                throw std::runtime_error("Layout graph canonical backend/thread/reentry changed");
        };
        try {
            // Admission shares the sticky failure path with all callback
            // bookends. A swallowed nested refusal still fails its outer call.
            pure(false);life->owner->busy_=true;
            struct Exit{std::shared_ptr<LayoutLifetime> life;~Exit(){if(life->alive.load())life->owner->busy_=false;}}exit{life};
            pure(true);backend->require_current();pure(true);
            auto result=operation(*backend);pure(true);
            backend->require_current();pure(true);return result;
        }catch(...){
            life->failed.store(true);
            if(life->alive.load())life->owner->poisoned_=true;
            if(backend)backend->poison();throw;
        }
    }
 public:
    void set_void1(Id id,std::int32_t value){layout_owned([&](auto& b){b.set_void1(id,value);return 0;});}
    void set_void2(Id id,std::int32_t value){layout_owned([&](auto& b){b.set_void2(id,value);return 0;});}
    void set_text(Id id,std::string value){require_backend();backend_->set_text(id,std::move(value));}
    void set_tooltip(Id id,std::string value){require_backend();backend_->set_tooltip(id,std::move(value));}
    void set_world_position(Id id,std::array<double,3> value){require_backend();backend_->set_world_position(id,value);}
    void set_offset(Id id,std::array<double,2> value){require_backend();backend_->set_offset(id,value);}
    void set_offset_x(Id id,double value){require_backend();backend_->set_offset_x(id,value);}
    void set_color(Id id,std::array<double,4> value){require_backend();backend_->set_color(id,value);}
    void set_fg_color(Id id,std::array<double,4> value){require_backend();backend_->set_fg_color(id,value);}
    void set_texture(Id id,std::string value){require_backend();backend_->set_texture(id,std::move(value));}
    void set_texture_offset(Id id,std::array<double,2> value){require_backend();backend_->set_texture_offset(id,value);}
    void set_percent(Id id,double value){require_backend();backend_->set_percent(id,value);}
    void animation_set_to_beginning(Id id){require_backend();backend_->animation_set_to_beginning(id);}
    void animation_play(Id id){require_backend();backend_->animation_play(id);}
    void set_consume_mouse_over(Id id,bool value){layout_owned([&](auto& b){b.set_consume_mouse_over(id,value);return 0;});}
    void calculate_size(Id id){layout_owned([&](auto& b){b.calculate_size(id);return 0;});}
    void reprocess_anchoring(Id id){layout_owned([&](auto& b){b.reprocess_anchoring(id);return 0;});}
    std::array<double,2> get_offset_val(Id id)const{return layout_owned([&](auto& b){return b.get_offset_val(id);});}
    double get_size_y(Id id)const{return layout_owned([&](auto& b){return b.get_size_y(id);});}
    void set_size_y(Id id,double value){layout_owned([&](auto& b){b.set_size_y(id,value);return 0;});}
    void calculate_internal_size(Id id){layout_owned([&](auto& b){b.calculate_internal_size(id);return 0;});}
    double get_ratio(Id id)const{return layout_owned([&](auto& b){return b.get_ratio(id);});}
    Id lookup_control(std::string_view path)const {require_backend();return backend_->lookup_control(path);}
    Id get_button(Id id)const {require_backend();return backend_->get_button(id);}
    void register_owned_callback(Id id,std::int64_t event,std::shared_ptr<const ScriptUiOwnedControlCallback> callback){
        require_backend();backend_->register_callback(id,event,std::move(callback));
    }
 private:
    friend class BnwWorldViewScriptUiAdapter;
    friend class LuaScriptHost;
    friend class BnwUnitFlagHealthInstanceLoan;
    // Installed ONLY by actual selected Adapter graph factory; never a name/type cast.
    std::weak_ptr<BnwWorldViewScriptUiAdapter> unitflag_health_adapter_;
    template<class F> void unitflag_health_owned(Id,BnwUnitFlagHealthCommand,F&&)const;
    void require_unitflag_health_current_owned(Id,BnwUnitFlagHealthCommand)const;
    void set_unitflag_health_hidden_owned(Id,bool);
    void set_unitflag_health_offset_x_owned(Id,double);
    void set_unitflag_health_percent_owned(Id,double);
    void set_unitflag_health_fg_color_owned(Id,std::array<double,4>);
    struct OwnedBackendTag {};
    ScriptUiControlGraph(OwnedBackendTag,std::string context,std::shared_ptr<ScriptUiControlGraphBackend> backend)
      :context_(std::move(context)),backend_(std::move(backend)){
        if(context_.empty()||!backend_)throw std::runtime_error("Selected graph backend absent");require_current();
    }
    ScriptUiEventResult invoke_owned_callback(Id id,std::int64_t event,std::span<const ScriptValue> args){
        require_backend();return backend_->invoke_callback(id,event,args);
    }
    struct LayoutLifetime {
        const ScriptUiControlGraph* const owner;
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true},failed{false};
        explicit LayoutLifetime(const ScriptUiControlGraph* actual):owner(actual){}
    };
    const std::shared_ptr<LayoutLifetime> layout_lifetime_=std::make_shared<LayoutLifetime>(this);
    void require_backend()const {require_current();if(!backend_)throw std::runtime_error("Authored graph has no selected control-effects backend");}
    static std::vector<Named> named(const std::vector<ScriptUiGraphBackendNamed>& values){
        std::vector<Named> result;result.reserve(values.size());for(const auto& n:values)result.push_back({n.name,n.id});return result;
    }
    static Node &find(std::vector<Node> &nodes,Id id) {
        const auto it=std::find_if(nodes.begin(),nodes.end(),[&](const auto &n){return n.alive&&n.id==id;});
        if(it==nodes.end())throw std::runtime_error("UI control handle is absent or destroyed");return *it;
    }
    static const Node &find(const std::vector<Node> &nodes,Id id) {
        const auto it=std::find_if(nodes.begin(),nodes.end(),[&](const auto &n){return n.alive&&n.id==id;});
        if(it==nodes.end())throw std::runtime_error("UI control handle is absent or destroyed");return *it;
    }
    void require_thread()const {if(std::this_thread::get_id()!=thread_)throw std::runtime_error("UI graph requires its constructor thread");}
    void append(std::vector<Node> &nodes,const content::XmlNode &xml,Id parent,bool dynamic,std::vector<Named> &ids) {
        const auto name=xml.attribute("ID");
        if(name.empty()||name.find('\0')!=name.npos||std::any_of(ids.begin(),ids.end(),[&](const auto &n){return n.name==name;}))
            throw std::runtime_error("Authored graph slice requires unique nonempty subtree IDs");
        if(xml.name!="WorldAnchor"&&xml.name!="Stack"&&xml.name!="Container"&&xml.name!="Image"&&xml.name!="Label")
            throw std::runtime_error("Unsupported authored control kind");
        const auto hidden=xml.attribute("Hidden");
        if(!hidden.empty()&&hidden!="0"&&hidden!="1")throw std::runtime_error("Unsupported authored Hidden value");
        if(nodes.size()>=4096||next_==0)throw std::runtime_error("Authored graph budget exhausted");
        const auto id=next_++;nodes.push_back({id,parent,xml,hidden=="1",true,dynamic});ids.push_back({name,id});
        for(const auto &child:xml.children)append(nodes,child,id,dynamic,ids);
    }
    template<class F> void mutate(F operation) {
        // Mutation attempts cannot be replayed after unknown callback outcome.
        busy_=true;
        try {
            auto next=nodes_;operation(next);validate_();
            if(retired_||poisoned_)throw std::runtime_error("UI graph retired before mutation commit");
            nodes_.swap(next);busy_=false;
        }catch(...) {busy_=false;poisoned_=true;throw;}
    }
    std::string context_;std::function<void()> validate_;
    std::shared_ptr<ScriptUiControlGraphBackend> backend_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::map<std::string,content::XmlNode> templates_;std::vector<Node> nodes_;std::vector<Named> static_;
    Id next_=1;bool retired_=false;mutable bool poisoned_=false;mutable bool busy_=false;
};
} // namespace civ5::runtime
