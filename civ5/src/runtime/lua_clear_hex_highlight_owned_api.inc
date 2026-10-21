//Outside Host::Impl. Sole NativeIssuer calls after actual Context protected
//loads and their lexical callback storage retire; before first Scene draw.
namespace civ5::runtime {
void LuaScriptHost::drain_clear_hex_highlight_events_owned(const BnwWorldViewSourceOperationLoan& op)const{
 const auto p=impl_;
 if(!p||p->ui_thread!=std::this_thread::get_id()||!p->state||p->failed||p->calling)
  throw ScriptError("Highlight outer drain requires SAME current idle Host");
 try{
  Impl::WorldViewNativeHostScope native(*p,*this,op);
  p->worldview_require_callback_origin();op.require_same_vm(*this);p->worldview_require_callback_origin();
  p->drain_clear_hex_highlight_owned(op);p->worldview_require_callback_origin();op.require_same_vm(*this);
  native.returned=true;
 }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
