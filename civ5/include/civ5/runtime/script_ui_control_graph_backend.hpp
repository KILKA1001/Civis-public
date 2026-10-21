#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/content/manifest.hpp"
#include <array>
namespace civ5::runtime {
// Non-mintable callback seam. Only the actual VM may derive/construct a
// descriptor retaining its compiled Proto, registry closure and environment.
// Source binding is a later exact union; no implementation/stub is supplied here.
class ScriptUiOwnedControlCallback {
 public:
    virtual ~ScriptUiOwnedControlCallback()=default;
    ScriptUiOwnedControlCallback(const ScriptUiOwnedControlCallback&)=delete;
    ScriptUiOwnedControlCallback& operator=(const ScriptUiOwnedControlCallback&)=delete;
    virtual const LuaScriptHost& host()const=0;
    virtual const ScriptUiChildSource& source()const=0;
    virtual void require_current()const=0;
    virtual ScriptUiEventResult invoke(std::span<const ScriptValue>)const=0;
 private:
    friend class LuaScriptHost;
    ScriptUiOwnedControlCallback()=default;
};
struct ScriptUiGraphBackendNode {
    std::uint64_t id=0,parent=0;content::XmlNode authored;
    bool hidden=false,alive=true,dynamic=false;
};
struct ScriptUiGraphBackendNamed {std::string name;std::uint64_t id;};
// This interface alone issues no graph. The graph's backend constructor is
// private and reserved for the selected authentic adapter factory.
class ScriptUiControlGraphBackend {
 public:
    virtual ~ScriptUiControlGraphBackend()=default;
    virtual void require_current()const=0;
    virtual void retire()=0;
    virtual void poison()=0;
    virtual std::vector<ScriptUiGraphBackendNamed> static_controls()const=0;
    virtual ScriptUiGraphBackendNode inspect(std::uint64_t)const=0;
    virtual std::vector<ScriptUiGraphBackendNamed> build(std::string_view,std::uint64_t)=0;
    virtual void set_hidden(std::uint64_t,bool)=0;
    virtual void change_parent(std::uint64_t,std::uint64_t)=0;
    virtual void destroy_child(std::uint64_t,std::uint64_t)=0;
    virtual void set_alpha(std::uint64_t,double)=0;
    virtual void set_disabled(std::uint64_t,bool)=0;
    virtual void set_void1(std::uint64_t,std::int32_t)=0;
    virtual void set_void2(std::uint64_t,std::int32_t)=0;
    virtual void set_consume_mouse_over(std::uint64_t,bool){throw std::runtime_error("Actual ConsumeMouseOver backend absent");}
    virtual void set_text(std::uint64_t,std::string)=0;
    virtual void set_tooltip(std::uint64_t,std::string)=0;
    virtual void set_world_position(std::uint64_t,std::array<double,3>)=0;
    virtual void set_offset(std::uint64_t,std::array<double,2>)=0;
    virtual void set_offset_x(std::uint64_t,double)=0;
    virtual void set_color(std::uint64_t,std::array<double,4>)=0;
    virtual void set_fg_color(std::uint64_t,std::array<double,4>)=0;
    virtual void set_texture(std::uint64_t,std::string)=0;
    virtual void set_texture_offset(std::uint64_t,std::array<double,2>)=0;
    virtual void set_percent(std::uint64_t,double)=0;
    virtual void animation_set_to_beginning(std::uint64_t)=0;
    virtual void animation_play(std::uint64_t)=0;
    virtual void calculate_size(std::uint64_t)=0;
    virtual void reprocess_anchoring(std::uint64_t)=0;
    virtual std::array<double,2> get_offset_val(std::uint64_t)const=0;
    virtual double get_size_y(std::uint64_t)const=0;
    virtual void set_size_y(std::uint64_t,double)=0;
    virtual void calculate_internal_size(std::uint64_t)=0;
    virtual double get_ratio(std::uint64_t)const=0;
    virtual std::uint64_t lookup_control(std::string_view)const=0;
    virtual std::uint64_t get_button(std::uint64_t)const=0;
    virtual void register_callback(std::uint64_t,std::int64_t,std::shared_ptr<const ScriptUiOwnedControlCallback>)=0;
    virtual ScriptUiEventResult invoke_callback(std::uint64_t,std::int64_t,std::span<const ScriptValue>)=0;
};
}
