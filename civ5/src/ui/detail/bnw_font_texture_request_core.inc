// Inside ui after the complete SAME GuiPort/registry definitions.
struct detail::BnwFontTextureRequest::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> failed{false};
 const std::weak_ptr<NotificationContextGuiPort> gui;
 const std::weak_ptr<NotificationContextGuiPort::Impl> storage;
 const std::weak_ptr<NotificationContextFontRegistryFields> registry;
 const std::weak_ptr<NotificationContextFontRegistryFields::Life> registry_life;
 const std::weak_ptr<NotificationContextFontRegistryFields::FontObject::Life> font_life;
 const std::weak_ptr<runtime::detail::BnwFontResourceLoan::Life> resource_life;
 const runtime::detail::BnwFontResourceLoan* const resource;
 const NotificationContextFontRegistryFields::Slot* const cell;
 const NotificationContextFontRegistryFields::FontObject* const allocation;
 const std::uint64_t registry_epoch,font_epoch,resource_epoch,serial,occurrence,before_texture_store_sequence;
 mutable std::optional<std::uint64_t> adopted_store;
 void require_storage_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,const runtime::detail::BnwFontResourceLoan&,bool after_store)const;
 const std::uint32_t key;
 const std::size_t pending_index,row_ordinal;
 const content::XmlNode* const row;
 const std::shared_ptr<const content::XmlNode> document;
 const std::shared_ptr<const runtime::BnwCivisOwnedStockSourceBundle> bundle;
 const std::string source,name;
 const bool allow_color;
 using Family=NotificationContextFontRegistryFields::FontObject::PendingRowFamily;
 const std::weak_ptr<Family> family;
 const std::weak_ptr<Family::Life> family_life;
 std::weak_ptr<const void> request_life;
 enum class Role {base,optional_index};const Role role;
 Impl(const std::shared_ptr<NotificationContextGuiPort>& facade,
      const std::shared_ptr<NotificationContextGuiPort::Impl>& state,
      const std::shared_ptr<NotificationContextFontRegistryFields>& fields,
      const runtime::detail::BnwFontResourceLoan& loan,const content::XmlNode& node,
      std::uint32_t actual_key,std::uint64_t actual_serial,std::uint64_t sequence,
      std::string actual_source,std::string actual_name,bool actual_allow,
      const Family::DerivedIndex* actual_derived=nullptr)
  :gui(facade),storage(state),registry(fields),registry_life(fields->life),
   font_life(fields->pending.back().font->life),resource_life(loan.life_),resource(&loan),
   cell(&fields->map_96cc.find(actual_key)->second),allocation(fields->pending.back().font.get()),
   registry_epoch(fields->life->epoch.load()),font_epoch(fields->pending.back().font->life->epoch.load()),
   resource_epoch(loan.life_->epoch.load()),serial(actual_serial),occurrence(sequence),before_texture_store_sequence(fields->pending.back().font->texture_store_sequence),key(actual_key),
   pending_index(fields->pending.size()-1),row_ordinal(fields->pending.back().font->textures_40c2c->size()),
   row(&node),document(loan.parser_document_),bundle(loan.bundle_),
   source(actual_derived?actual_derived->filename:std::move(actual_source)),name(std::move(actual_name)),allow_color(actual_allow),
   family(fields->pending.back().font->pending_row_family),
   family_life(fields->pending.back().font->pending_row_family->life),
   role(actual_derived?Role::optional_index:Role::base){
    const auto f=family.lock();require(bool(f),"Font actual row family missing at request construction");f->first();
    require(f->font_allocation==allocation&&f->resource==resource&&f->serial==serial&&f->ordinal==row_ordinal&&
        f->before_store==before_texture_store_sequence&&f->node==row&&f->document.get()==document.get()&&
        !f->document.owner_before(document)&&!document.owner_before(f->document),
        "Font request not actual reached pending-row owner");
    if(actual_derived)require(f->derived&&actual_derived==&*f->derived&&f->base&&f->base->returned&&
        f->expected_base&&actual_derived->marker!=0&&!f->index&&!f->row_stored&&source==f->derived->filename,
        "Font index constructor operand is not actual reached same-row metadata");
    else require(source==f->original_src&&!f->base&&!f->derived&&!f->index&&!f->row_stored,
        "Font base constructor operand is not actual reached XML source");
 }
};
detail::BnwFontTextureRequest::BnwFontTextureRequest(std::unique_ptr<Impl> p)
 :impl_(std::move(p)),life_(new Life(impl_->occurrence)){
 const auto f=impl_->family.lock();require(bool(f),"Font request lost actual family during Life allocation");
 f->first();impl_->request_life=life_;
 f->register_slot(impl_->role==Impl::Role::optional_index,life_,impl_->occurrence);
}
detail::BnwFontTextureRequest::~BnwFontTextureRequest(){life_->alive_.store(false);
 if(const auto f=impl_->family.lock();f&&!f->row_stored)f->life->failed.store(true);
}
void detail::BnwFontTextureRequest::Life::require_current_owned(std::uint64_t expected)const{
 if(creator_!=std::this_thread::get_id()||!alive_.load()||failed_.load()||!occurrence_||expected!=occurrence_){
  failed_.store(true);throw std::runtime_error("Font texture request lifetime/occurrence retired/foreign/substituted");}
}
void detail::BnwFontTextureRequest::Impl::require_storage_current_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan,bool after_store)const{
 const auto& p=*this;
 try{
  require(p.creator==std::this_thread::get_id()&&!p.failed.load(),"Font texture request foreign/failed before raw access");
  // Independent fences FIRST: never dereference saved raw loan/Font/slot if
  // its actual owner has retired. Shared/control identity also forbids remint.
  const auto ff=p.family_life.lock();require(bool(ff),"Font pending-row family fence retired");ff->first();
  const auto f=p.family.lock();require(f&&f->life.get()==ff.get()&&!f->life.owner_before(ff)&&!ff.owner_before(f->life),"Font family life/control changed");
  f->require_slot(p.role==Role::optional_index,p.request_life,p.occurrence);
  const auto rl=p.registry_life.lock();const auto fl=p.font_life.lock();const auto ll=p.resource_life.lock();
  require(rl&&fl&&ll,"Font texture real owner fence retired");rl->first(p.registry_epoch);fl->first(p.font_epoch);ll->first(p.resource_epoch);
  require(&loan==p.resource,"Font texture request uses another live resource loan");
  const auto g=p.gui.lock();const auto s=p.storage.lock();const auto r=p.registry.lock();
  require(g&&s&&r&&g->impl_.get()==s.get()&&!g->impl_.owner_before(s)&&!s.owner_before(g->impl_),"Font texture canonical Gui facade/storage retired");
  require(s->canonical_gui.lock().get()==g.get()&&!s->canonical_gui.owner_before(g)&&!g.owner_before(s->canonical_gui)&&s->font_registry.get()==r.get()&&
   !s->font_registry.owner_before(r)&&!r.owner_before(s->font_registry),"Font texture actual Gui/registry control substituted");
  require(r->life.get()==rl.get()&&!r->life.owner_before(rl)&&!rl.owner_before(r->life),"Font texture registry life changed");
  s->first(op,true);r->first();op.require_operation(p.serial);
  require(p.occurrence&&p.occurrence<=r->request_sequence&&p.pending_index<r->pending.size(),"Font texture unissued occurrence");
  require(f->resource_epoch==p.resource_epoch&&!f->resource_life.owner_before(ll)&&!ll.owner_before(f->resource_life)&&f->resource_life.lock().get()==ll.get(),"Font family uses another actual loan life/epoch");
  const auto& pending=r->pending[p.pending_index];require(pending.key==p.key&&pending.font.get()==p.allocation,"Font pending allocation changed");
  require(pending.font->life.get()==fl.get()&&!pending.font->life.owner_before(fl)&&!fl.owner_before(pending.font->life),"Font allocation lifetime changed");
  const auto at=r->map_96cc.find(p.key);require(at!=r->map_96cc.end()&&&at->second==p.cell&&!at->second.completed,"Font texture NULL key cell changed");
  const auto verify_font_cut=[&]{
   f->first();f->require_slot(p.role==Role::optional_index,p.request_life,p.occurrence);
   require(p.pending_index<r->pending.size()&&r->pending.at(p.pending_index).font.get()==p.allocation,"Font actual pending owner changed");
   const auto& actual_pending=r->pending.at(p.pending_index);
   require(actual_pending.font->pending_row_family.get()==f.get()&&
       !actual_pending.font->pending_row_family.owner_before(f)&&!f.owner_before(actual_pending.font->pending_row_family),
       "Font actual row-family allocation/control substituted");
   require(f->font_allocation==p.allocation&&f->resource==p.resource&&f->serial==p.serial&&f->ordinal==p.row_ordinal&&
       f->before_store==p.before_texture_store_sequence&&f->node==p.row&&f->document.get()==p.document.get()&&
       !f->document.owner_before(p.document)&&!p.document.owner_before(f->document),
       "Font family actual row/source/Font preimages changed");
   if(p.role==Role::optional_index)require(f->base&&f->base->returned&&f->expected_base&&f->derived&&
       f->derived->marker!=0&&p.source==f->derived->filename,"Font index source lost actual base/derived operand");
   else require(p.source==f->original_src,"Font base source no longer actual XML operand");
   require(actual_pending.font->textures_40c2c.has_value(),"Font texture actual vector unconstructed");
   require(after_store==f->row_stored,"Font family/vector append phase mismatch");
   if(after_store){
    f->all_queries_returned();
    require(actual_pending.font->textures_40c2c->size()==p.row_ordinal+1&&actual_pending.font->texture_store_sequence==p.before_texture_store_sequence+1,
        "Font texture earned vector/store delta changed");
    const auto& stored=actual_pending.font->textures_40c2c->at(p.row_ordinal);
    const auto same_borrower=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto marker=f->derived?f->derived->marker:0;
    require(stored.name[63]=='\0'&&std::string(stored.name.data())==p.name&&same_borrower(stored.base,f->expected_base)&&
        same_borrower(stored.active,f->expected_active)&&stored.allow_color==p.allow_color&&
        std::bit_cast<std::uint32_t>(stored.bc)==std::bit_cast<std::uint32_t>(static_cast<float>(marker))&&
        stored.request_occurrence==f->base->occurrence&&stored.index_occurrence==(f->index?f->index->occurrence:0)&&
        stored.source_serial==p.serial&&stored.ordinal==p.row_ordinal&&stored.store_sequence==p.before_texture_store_sequence+1,
        "Font actual family-row metadata/custody/occurrences mismatch");
   }else require(actual_pending.font->textures_40c2c->size()==p.row_ordinal&&actual_pending.font->texture_store_sequence==p.before_texture_store_sequence,
        "Font texture pre-query vector/store cut changed");
  };
  verify_font_cut();
  require(loan.life_.get()==ll.get()&&loan.bundle_.get()==p.bundle.get()&&
   !loan.bundle_.owner_before(p.bundle)&&!p.bundle.owner_before(loan.bundle_)&&loan.parser_document_.get()==p.document.get(),"Font texture actual resource scope replaced");
  require(p.row&&p.document,"Font texture reached XML row missing");
  (void)loan.xml_owned(op); // Validate SAME staged tree; never copy/rebind it.
  bool found=false;for(const auto& section:p.document->children)if(section.name=="textures"){
   std::size_t ordinal=0;for(const auto& row:section.children)if(row.name=="texture"){
    if(ordinal==p.row_ordinal){found=&row==p.row;break;}++ordinal;}break;}
  require(found,"Font texture row not SAME staged XML occurrence");
  loan.require_current_owned(op);s->first(op,true);rl->first(p.registry_epoch);fl->first(p.font_epoch);ll->first(p.resource_epoch);
  verify_font_cut();
 }catch(...){p.failed.store(true);if(const auto f=p.family.lock())f->life->failed.store(true);throw;}
}
#include "bnw_font_texture_row_seal_core.inc"
const std::string& detail::BnwFontTextureRequest::source_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan)const{require_current_owned(op,loan);return impl_->source;}
bool detail::BnwFontTextureRequest::optional_index_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan)const{require_current_owned(op,loan);return impl_->role==Impl::Role::optional_index;}
std::uint64_t detail::BnwFontTextureRequest::texture_occurrence_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan)const{require_current_owned(op,loan);return impl_->occurrence;}
std::weak_ptr<detail::BnwFontTextureRequest::Life> detail::BnwFontTextureRequest::texture_lifetime_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const runtime::detail::BnwFontResourceLoan& loan)const{
 require_current_owned(op,loan);return life_;
}
