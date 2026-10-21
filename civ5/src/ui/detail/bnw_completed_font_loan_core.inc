// Inside ui after SAME Gui, Font registry and request complete definitions.
struct detail::BnwCompletedFontLoan::Impl final {
 using Registry=detail::NotificationContextFontRegistryFields;
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<detail::NotificationContextGuiPort> gui;
 const std::weak_ptr<detail::NotificationContextGuiPort::Impl> storage;
 const std::weak_ptr<Registry> registry;
 const std::weak_ptr<Registry::Life> registry_life;
 const std::weak_ptr<Registry::FontObject::Life> font_life;
 const std::weak_ptr<detail::BnwLabelProcessModuleOwner> module;
 const Registry::FontObject* const allocation;
 const Registry::Slot* const cell;
 const std::uint32_t key;
 const std::uint64_t registry_epoch,font_epoch,generation,loader_serial;
 mutable std::atomic<bool> failed{false};
 // Delegation performs FIRST after make_shared allocation and BEFORE
 // any initializer reads the actual published Font or completion marker.
 struct ConstructorCut final {
  std::shared_ptr<detail::NotificationContextGuiPort> gui;
  std::shared_ptr<detail::NotificationContextGuiPort::Impl> storage;
  std::shared_ptr<Registry> registry;
  std::shared_ptr<Registry::Life> registry_life;
  std::shared_ptr<Registry::FontObject::Life> font_life;
  std::shared_ptr<detail::BnwLabelProcessModuleOwner> module;
  const Registry::FontObject* allocation;const Registry::Slot* cell;
  std::uint32_t key;std::uint64_t registry_epoch,font_epoch,generation,loader_serial;
 };
 static ConstructorCut capture(const std::shared_ptr<detail::NotificationContextGuiPort>& g,
      const std::shared_ptr<detail::NotificationContextGuiPort::Impl>& s,
      const std::shared_ptr<Registry>& r,std::uint32_t k,
      const runtime::BnwWorldViewSourceOperationLoan& operation){
  require(g&&s&&r&&s->creator==std::this_thread::get_id()&&s->facade_alive.load()&&!s->failed.load(),
   "Completed Font constructor origin retired before initializer");
  require(g->impl_.get()==s.get()&&!g->impl_.owner_before(s)&&!s.owner_before(g->impl_)&&
   s->font_registry.get()==r.get()&&!s->font_registry.owner_before(r)&&!r.owner_before(s->font_registry),
   "Completed Font constructor canonical ownership changed");
  const auto rl=r->life;rl->first(1);s->first(operation,true);rl->first(1);
  const auto fresh_serial=operation.operation_serial();rl->first(1);
  operation.require_operation(fresh_serial);rl->first(1);s->first(operation,true);rl->first(1);r->first();
  require(g->impl_.get()==s.get()&&!g->impl_.owner_before(s)&&!s.owner_before(g->impl_)&&
   s->font_registry.get()==r.get()&&!s->font_registry.owner_before(r)&&!r.owner_before(s->font_registry),
   "Completed Font constructor owner control changed during Source cut");
  const auto at=r->map_96cc.find(k);require(at!=r->map_96cc.end()&&at->second.completed,
   "Completed Font constructor has no actual published Font");
  const auto allocation=at->second.completed.get();const auto fl=allocation->life;
  const auto fe=fl->epoch.load();fl->first(fe);
  const auto module=s->label_module;require(bool(module),"Completed Font constructor module retired");
  module->require_current_without_callbacks();fl->first(fe);rl->first(1);
  s->first(operation,true);fl->first(fe);rl->first(1);
  operation.require_operation(fresh_serial);fl->first(fe);rl->first(1);
  s->first(operation,true);fl->first(fe);rl->first(1);r->first();
  require(g->impl_.get()==s.get()&&!g->impl_.owner_before(s)&&!s.owner_before(g->impl_)&&
   s->font_registry.get()==r.get()&&!s->font_registry.owner_before(r)&&!r.owner_before(s->font_registry),
   "Completed Font constructor owner control changed during final Source cut");
  const auto post=r->map_96cc.find(k);
  require(post!=r->map_96cc.end()&&post->second.completed.get()==allocation&&
   allocation->life.get()==fl.get()&&!allocation->life.owner_before(fl)&&!fl.owner_before(allocation->life)&&
   allocation->completion&&s->label_module.get()==module.get()&&
   !s->label_module.owner_before(module)&&!module.owner_before(s->label_module),
   "Completed Font constructor publication/life/module changed across callback");
  return {g,s,r,rl,fl,module,allocation,&post->second,k,rl->epoch.load(),fe,s->generation,
   allocation->completion->source_serial};
 }
 Impl(const std::shared_ptr<detail::NotificationContextGuiPort>& g,
      const std::shared_ptr<detail::NotificationContextGuiPort::Impl>& s,
      const std::shared_ptr<Registry>& r,std::uint32_t k,
      const runtime::BnwWorldViewSourceOperationLoan& operation):Impl(capture(g,s,r,k,operation)){}
 explicit Impl(const ConstructorCut& cut)
  :gui(cut.gui),storage(cut.storage),registry(cut.registry),registry_life(cut.registry_life),font_life(cut.font_life),
   module(cut.module),allocation(cut.allocation),cell(cut.cell),key(cut.key),registry_epoch(cut.registry_epoch),
   font_epoch(cut.font_epoch),generation(cut.generation),loader_serial(cut.loader_serial){}
 struct Current final {
  std::shared_ptr<detail::NotificationContextGuiPort> gui;
  std::shared_ptr<detail::NotificationContextGuiPort::Impl> storage;
  std::shared_ptr<Registry> registry;
  std::shared_ptr<detail::BnwLabelProcessModuleOwner> module;
 };
 Current first()const{
  require(creator==std::this_thread::get_id()&&!failed.load(),"Completed Font loan foreign/failed");
  const auto rl=registry_life.lock();const auto fl=font_life.lock();
  require(rl&&fl,"Completed Font real owner life retired");rl->first(registry_epoch);fl->first(font_epoch);
  Current out{gui.lock(),storage.lock(),registry.lock(),module.lock()};
  require(out.gui&&out.storage&&out.registry&&out.module,"Completed Font canonical owner/module retired");
  const auto& s=out.storage;const auto& r=out.registry;
  require(s->creator==creator&&s->facade_alive.load()&&!s->failed.load()&&s->generation==generation&&
   out.gui->impl_.get()==s.get()&&!out.gui->impl_.owner_before(s)&&!s.owner_before(out.gui->impl_)&&
   s->canonical_gui.lock().get()==out.gui.get()&&!s->canonical_gui.owner_before(out.gui)&&!out.gui.owner_before(s->canonical_gui),
   "Completed Font canonical Gui/generation/control changed");
  require(!s->source.expired()&&!s->host.expired()&&!s->world.expired()&&!s->scene.expired()&&!s->window.expired(),
   "Completed Font Gui Source/world/window origin retired");
  require(s->font_registry.get()==r.get()&&!s->font_registry.owner_before(r)&&!r.owner_before(s->font_registry)&&
   s->label_module.get()==out.module.get()&&!s->label_module.owner_before(out.module)&&!out.module.owner_before(s->label_module),
   "Completed Font registry/module owner control changed");
  r->first();const auto at=r->map_96cc.find(key);
  require(at!=r->map_96cc.end()&&&at->second==cell&&at->second.completed.get()==allocation,"Completed Font registry publication changed");
  const auto& font=*at->second.completed;
  require(font.life.get()==fl.get()&&!font.life.owner_before(fl)&&!fl.owner_before(font.life)&&font.completion&&
   font.completion->generation==generation&&font.completion->source_serial==loader_serial&&font.resource_quote&&
   !font.pending_row_family&&font.styles_40c20&&font.textures_40c2c&&font.glyphs_40c38&&font.glyph_lookup_40c44&&
   font.completion->texture_count==font.textures_40c2c->size()&&font.completion->style_count==font.styles_40c20->size()&&
   font.completion->glyph_count==font.glyphs_40c38->size()&&font.texture_returns.size()==font.textures_40c2c->size(),
   "Completed Font actual loader-return shape/cleanup changed");
  return out;
 }
};
detail::BnwCompletedFontLoan::BnwCompletedFontLoan(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::BnwCompletedFontLoan::~BnwCompletedFontLoan()=default; // Registry owns Font; this is not native421EA0.
void detail::BnwCompletedFontLoan::require_current_without_callbacks()const{
 const auto saved=impl_;
 try{const auto current=saved->first();current.module->require_current_without_callbacks();(void)saved->first();}
 catch(...){saved->failed.store(true);throw;}
}
void detail::BnwCompletedFontLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;
 try{require_current_without_callbacks();const auto current=saved->first();
  current.storage->first(operation);require_current_without_callbacks();operation.require_current();require_current_without_callbacks();
  const auto actual=operation.label_module_owned();require_current_without_callbacks();
  require(actual&&actual.get()==current.module.get()&&!actual.owner_before(current.module)&&!current.module.owner_before(actual),
   "Completed Font Source names another renderer module");require_current_without_callbacks();}
 catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<const graphics::detail::BnwFontTextureBorrower>
detail::BnwCompletedFontLoan::texture_borrower_without_callbacks(std::uint32_t index)const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto& rows=impl_->allocation->textures_40c2c;
 require(index<rows->size(),"Completed Font texture ordinal outside actual vector");
 const auto result=rows->at(index).active;require_current_without_callbacks();return result;
 // DATA only: caller must enter genuine WorldViewPaintScope; NULL is not paint readiness.
}
std::optional<std::array<std::uint32_t,9>>
detail::BnwCompletedFontLoan::glyph_bits_without_callbacks(std::uint32_t ch)const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto& font=*impl_->allocation;const auto at=font.glyph_lookup_40c44->find(ch);
 if(at==font.glyph_lookup_40c44->end()){require_current_without_callbacks();return std::nullopt;}
 require(at->second<font.glyphs_40c38->size(),"Completed Font glyph map outside actual vector");
 const auto result=font.glyphs_40c38->at(at->second).bits;require_current_without_callbacks();return result;
 // No synthesized glyph, fallback character, layout/text-run or geometry authority.
}
std::array<std::int32_t,3> detail::BnwCompletedFontLoan::metrics_without_callbacks()const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto& scalars=impl_->allocation->scalar_40c08_40c18;
 const std::array<std::int32_t,3> result{std::bit_cast<std::int32_t>(scalars[0]),std::bit_cast<std::int32_t>(scalars[1]),std::bit_cast<std::int32_t>(scalars[2])};
 require_current_without_callbacks();return result;
}

std::uint32_t detail::BnwCompletedFontLoan::style_ordinal_without_callbacks(std::string_view requested)const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto prefix=requested.substr(0,requested.find('\0'));
 const auto& styles=*impl_->allocation->styles_40c20;
 require(styles.size()<=UINT32_MAX,"Completed Font styles outside native ordinal domain");
 for(std::size_t i=0;i<styles.size();++i){
  const auto& name=styles[i].name;require(name[63]=='\0',"Completed Font native64 style name not terminated");
  if(std::string_view(name.data())==prefix){
   const auto result=static_cast<std::uint32_t>(i);require_current_without_callbacks();return result;
  }
 }
 require_current_without_callbacks();return 0; // Original6F8205 XOR EAX,EAX, not invented fallback.
}
std::array<std::uint32_t,3> detail::BnwCompletedFontLoan::style_layer_texture_indices_without_callbacks(std::uint32_t ordinal)const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto& styles=*impl_->allocation->styles_40c20;
 require(ordinal<styles.size(),"Completed Font style ordinal outside actual native76 vector");
 const auto result=styles[ordinal].layers;require_current_without_callbacks();return result;
 // Actual UINT32_MAX unused slots stay exact; no default layer or texture.
}
detail::BnwCompletedFontLoan::TextureDrawData
detail::BnwCompletedFontLoan::texture_draw_data_without_callbacks(std::uint32_t ordinal)const{
 require_current_without_callbacks();const auto current=impl_->first();
 const auto& textures=*impl_->allocation->textures_40c2c;
 require(ordinal<textures.size(),"Completed Font texture ordinal outside actual native80 vector");
 const auto& row=textures[ordinal];
 TextureDrawData result{row.active,row.base,row.allow_color,std::bit_cast<std::uint32_t>(row.bc)};
 require_current_without_callbacks();return result;
 // Both borrowers refer to earned same-row semantic80 entries, including
 // genuine NULL. Shared-pointer copies are DATA, never query/paint renewal.
}
