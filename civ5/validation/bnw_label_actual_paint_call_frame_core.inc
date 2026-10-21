// Inside civ5::ui AFTER complete actual WorldViewPaintScope.
namespace {
std::array<std::uint32_t,2> label_actual_numeric_profile_without_callbacks(){
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
 std::uint16_t cw=0;std::uint32_t mxcsr=0;
 __asm__ volatile("fnstcw %0\n\tstmxcsr %1":"=m"(cw),"=m"(mxcsr));
 return {cw,mxcsr};
#else
 throw std::runtime_error("Actual Label paint requires supported raw x87/SSE capture");
#endif
}
void label_paint_frame_need(bool yes,const char* message){
 if(!yes)throw std::runtime_error(message);
}
}
struct detail::BnwLabelActualPaintCallFrame::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> renderer_life;
 const UiScreenGlRenderer*const renderer;
 const UiScreenGlRenderer::WorldViewPaintScope*const scope;
 // Physical custody of the SAME original adjusted object; no active
 // item/Source/Root authority is prolonged by this strong owner.
 const std::shared_ptr<const ComposedUiFrame> frame_owner;
 const ComposedUiFrame*const frame;
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 const BnwWorldViewControlRenderPublication*const publication;
 const std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t serial,control_id;
 const std::weak_ptr<BnwWorldViewControlTree> tree;
 const std::array<std::uint32_t,2> numeric;
 Impl(std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> life,const UiScreenGlRenderer* actual,
      const UiScreenGlRenderer::WorldViewPaintScope* entered,
      std::shared_ptr<const ComposedUiFrame> owned,const ComposedUiFrame* composed,
      std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner,
      const BnwWorldViewControlRenderPublication* current,
      const runtime::BnwWorldViewSourceOperationLoan* source,std::uint64_t operation_serial,
      std::uint64_t control,std::optional<detail::BnwNormalRootContextEntry> enclosing,
      std::weak_ptr<BnwWorldViewControlTree> actual_tree,
      std::array<std::uint32_t,2> profile)
  :renderer_life(std::move(life)),renderer(actual),scope(entered),frame_owner(std::move(owned)),
   frame(composed),publication_owner(std::move(original_owner)),publication(current),enclosing_entry(std::move(enclosing)),
   operation(source),serial(operation_serial),control_id(control),tree(std::move(actual_tree)),numeric(profile){}
 static void require_enclosing_entry_without_callbacks(
  const detail::BnwNormalRootContextEntry& entry,const BnwWorldViewControlTree& tree){
  entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(tree);
  entry.require_current_without_callbacks();
 }
 void first()const{
  label_paint_frame_need(alive.load()&&creator==std::this_thread::get_id(),
   "Actual Label renderer paint callframe retired or foreign creator");
  label_paint_frame_need(publication_owner&&publication_owner.get()==publication,
   "Actual Label Frame lost SAME original Publication owner");
  label_paint_frame_need(frame_owner&&frame_owner.get()==frame,
   "Actual Label Frame lost SAME original owned adjusted backing");
  // Independent actual renderer destruction and exact active stack precede
  // every borrowed scope/frame/publication/operation dereference.
  const auto life=renderer_life.lock();
  label_paint_frame_need(life&&life->creator==creator&&life->alive&&!life->failed&&life->active&&
   life->renderer==renderer&&life->active_scope==scope&&scope,
   "Actual Label renderer paint scope retired or replaced");
  if(enclosing_entry){
   enclosing_entry->require_current_without_callbacks();
   const auto actual_tree=tree.lock();label_paint_frame_need(actual_tree!=nullptr,
    "Actual Label child lost original enclosing Context Tree");
   require_enclosing_entry_without_callbacks(*enclosing_entry,*actual_tree);
  }
  label_paint_frame_need(scope->painting&&frame&&publication&&scope->frame==frame&&
   scope->publication==publication&&&scope->operation==operation&&scope->serial==serial,
   "Actual Label requires SAME entered frame/publication/Source");
  scope->pure();
  label_paint_frame_need(alive.load()&&life->alive&&!life->failed&&life->active&&
   life->active_scope==scope&&scope->painting&&scope->frame==frame&&scope->publication==publication,
   "Actual Label scope changed during pure origin checks");
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Actual Label numeric control profile changed");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& source)const{
  first();label_paint_frame_need(&source==operation,"Actual Label has another Source operation");
  if(enclosing_entry){enclosing_entry->require_source_owned(source);first();}
  source.require_operation(serial);first();scope->full();first();
  if(enclosing_entry){enclosing_entry->require_source_owned(source);first();}
  source.require_operation(serial);first();
 }
};
detail::BnwLabelActualPaintCallFrame::BnwLabelActualPaintCallFrame(
 const IssuerKey&,std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwLabelActualPaintCallFrame::~BnwLabelActualPaintCallFrame(){impl_->alive.store(false);}
detail::BnwLabelActualPaintCallFrame::Guard::Guard(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwLabelActualPaintCallFrame::Guard detail::BnwLabelActualPaintCallFrame::capture_guard_without_callbacks()const{
 const auto data=impl_;data->first();return Guard(data);
}
void detail::BnwLabelActualPaintCallFrame::Guard::require_current_without_callbacks()const{
 const auto data=impl_;data->first();
}
std::shared_ptr<const BnwWorldViewControlRenderPublication>
detail::BnwLabelActualPaintCallFrame::Guard::original_publication_without_callbacks()const{
 const auto data=impl_;data->first();
 const auto owned=data->publication_owner;
 data->first();return owned;
}
void detail::BnwLabelActualPaintCallFrame::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& source)const{
 const auto data=impl_;data->full(source);data->first();
}
void detail::BnwLabelActualPaintCallFrame::Guard::require_actual_control_without_callbacks(
 const BnwWorldViewControlTree& actual,std::uint64_t control)const{
 const auto data=impl_;data->first();
 {const auto tree=data->tree.lock();label_paint_frame_need(tree&&tree.get()==&actual&&
   control==data->control_id,"Actual Label paint has another canonical control");}
 data->first();
}
void detail::BnwLabelActualPaintCallFrame::Guard::require_actual_renderer_without_callbacks(
 const UiScreenGlRenderer& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->renderer==&actual,
  "Actual Label paint has another renderer");data->first();
}
std::array<std::uint32_t,2> detail::BnwLabelActualPaintCallFrame::Guard::actual_numeric_profile_without_callbacks()const{
 const auto data=impl_;data->first();const auto result=data->numeric;data->first();return result;
}
// Separate immutable root-scope health, earned while original ACTIVE Frame lives.
struct detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::Impl final {
 const std::thread::id creator;
 const std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> renderer_life;
 const UiScreenGlRenderer*const renderer;
 const UiScreenGlRenderer::WorldViewPaintScope*const scope;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t serial;
 // Opaque association only: no history method dereferences this pointer.
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 const BnwWorldViewControlRenderPublication*const publication;
 const std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const std::shared_ptr<const ComposedUiFrame> frame_owner;
 const std::array<std::uint32_t,2> numeric;
 Impl(std::thread::id actual_creator,
  std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> life,const UiScreenGlRenderer* actual,
  const UiScreenGlRenderer::WorldViewPaintScope* entered,
  const runtime::BnwWorldViewSourceOperationLoan* source,std::uint64_t source_serial,
  std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner,
  const BnwWorldViewControlRenderPublication* original,std::shared_ptr<const ComposedUiFrame> owned,
  std::optional<detail::BnwNormalRootContextEntry> enclosing,
  std::array<std::uint32_t,2> profile)
  :creator(actual_creator),renderer_life(std::move(life)),renderer(actual),scope(entered),
   operation(source),serial(source_serial),publication_owner(std::move(original_owner)),publication(original),
   enclosing_entry(std::move(enclosing)),frame_owner(std::move(owned)),numeric(profile){}
 void first()const{
  label_paint_frame_need(creator==std::this_thread::get_id(),
   "Root scope health has a foreign creator");
  // Genuine weak life and exact active_scope precede EVERY borrowed scope or
  // Source read. No ACTIVE Frame, painting, frame or Label item is consulted.
  const auto life=renderer_life.lock();
  label_paint_frame_need(life&&life->creator==creator&&life->alive&&!life->failed&&life->active&&
   life->renderer==renderer&&scope&&life->active_scope==scope,
   "Root scope health actual renderer/scope retired or replaced");
  if(enclosing_entry)enclosing_entry->require_current_without_callbacks();
  label_paint_frame_need(&scope->operation==operation&&scope->serial==serial,
   "Root scope health actual Source tuple changed");
  scope->operation.require_origin_without_callbacks();scope->pure();
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->creator==creator&&
   life->renderer==renderer&&life->active_scope==scope&&&scope->operation==operation&&scope->serial==serial,
   "Root scope health changed during pure projection/tree checks");
  label_paint_frame_need(frame_owner&&publication&&publication_owner&&publication_owner.get()==publication,
   "Root scope health original immutable backing/association absent");
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Root scope health numeric control profile changed");
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->creator==creator&&
   life->renderer==renderer&&life->active_scope==scope,
   "Root scope health actual life/scope changed after numeric observation");
  if(enclosing_entry)enclosing_entry->require_current_without_callbacks();
  label_paint_frame_need(&scope->operation==operation&&scope->serial==serial,
   "Root scope health actual Source tuple changed at pure POST");
  scope->operation.require_origin_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& input)const{
  first();label_paint_frame_need(&input==operation,"Root scope health has another Source");
  if(enclosing_entry){enclosing_entry->require_source_owned(input);first();}
  input.require_operation(serial);first();scope->full();first();
  if(enclosing_entry){enclosing_entry->require_source_owned(input);first();}
  input.require_operation(serial);first();
 }
};
detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::RootScopeHealthGuard(
 std::shared_ptr<const Impl> data,const CaptureKey&):impl_(std::move(data)){}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_physical_current_without_callbacks()const{
 const auto data=impl_;data->first();
}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& input)const{
 const auto data=impl_;data->full(input);data->first();
}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_same_root_scope_without_callbacks(
 const RootScopeHealthGuard& other)const{
 const auto data=impl_;const auto saved=other.impl_;data->first();saved->first();
 label_paint_frame_need(data->renderer==saved->renderer&&data->scope==saved->scope&&
  data->operation==saved->operation&&data->serial==saved->serial&&
  !data->renderer_life.owner_before(saved->renderer_life)&&
  !saved->renderer_life.owner_before(data->renderer_life),
  "Root scope health belongs to another genuine root scope");
 data->first();saved->first();
}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_actual_renderer_without_callbacks(
 const UiScreenGlRenderer& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->renderer==&actual,
  "Root scope health belongs to another actual renderer");data->first();
}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_actual_publication_without_callbacks(
 const BnwWorldViewControlRenderPublication& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->publication==&actual,
  "Root scope health has another original publication association");data->first();
}
void detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::require_actual_frame_owner_without_callbacks(
 const std::shared_ptr<const ComposedUiFrame>& actual)const{
 const auto data=impl_;const auto owner=actual;data->first();
 label_paint_frame_need(owner&&owner.get()==data->frame_owner.get()&&
  !owner.owner_before(data->frame_owner)&&!data->frame_owner.owner_before(owner),
  "Root scope health has another original adjusted backing control");data->first();
}
std::array<std::uint32_t,2>
detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard::numeric_controls_without_callbacks()const{
 const auto data=impl_;data->first();const auto result=data->numeric;data->first();return result;
}
detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard
detail::BnwLabelActualPaintCallFrame::Guard::capture_root_scope_health_owned(
 const runtime::BnwWorldViewSourceOperationLoan& input)const{
 // Snap original storage first, then original ACTIVE FIRST/FULL before any new
 // owner or history-control allocation. Capture is never allowed after return.
 const auto data=impl_;const auto*const supplied=&input;
 data->first();data->full(*supplied);data->first();
 const auto frame_owner=data->publication->actual_adjusted_frame_owner_for_paint_without_callbacks(
  *data->renderer,*data->frame);data->first();
 label_paint_frame_need(frame_owner&&frame_owner.get()==data->frame&&
  frame_owner.get()==data->frame_owner.get()&&!frame_owner.owner_before(data->frame_owner)&&
  !data->frame_owner.owner_before(frame_owner),"Root scope capture changed SAME adjusted Frame control");
 // Every tuple operand is a VALUE snapshot before allocation/Source callbacks.
 const auto creator=data->creator;const auto life=data->renderer_life;
 const auto*const renderer=data->renderer;const auto*const scope=data->scope;
 const auto*const operation=data->operation;const auto serial=data->serial;
 const auto publication_owner=data->publication_owner;const auto enclosing_entry=data->enclosing_entry;
 const auto*const publication=data->publication;const auto numeric=data->numeric;
 const auto original_first=[&]{data->first();
  label_paint_frame_need(data->creator==creator&&data->renderer==renderer&&data->scope==scope&&
   data->operation==operation&&data->serial==serial&&data->publication==publication&&data->numeric==numeric&&
   !life.owner_before(data->renderer_life)&&!data->renderer_life.owner_before(life)&&
   data->frame==frame_owner.get()&&data->frame_owner.get()==frame_owner.get()&&
   !data->frame_owner.owner_before(frame_owner)&&!frame_owner.owner_before(data->frame_owner),
   "Root scope capture immutable original tuple changed");};
 original_first();data->full(*supplied);original_first();
 auto history=std::make_shared<const RootScopeHealthGuard::Impl>(creator,life,renderer,scope,
  operation,serial,publication_owner,publication,frame_owner,enclosing_entry,numeric);
 original_first();data->full(*supplied);original_first();history->first();original_first();
 const RootScopeHealthGuard::CaptureKey key;
 RootScopeHealthGuard result(history,key);
 original_first();result.require_physical_current_without_callbacks();original_first();
 return result;
}

std::shared_ptr<const detail::BnwLabelActualPaintCallFrame>
UiScreenGlRenderer::capture_label_actual_paint_call_frame_owned(){
 // Read raw renderer members once before any callback/allocation.
 const auto life=worldview_lifetime_;const auto*const actual=this;
 label_paint_frame_need(life&&life->creator==std::this_thread::get_id()&&life->alive&&!life->failed&&
  life->active&&life->renderer==actual&&life->active_scope,
  "Actual Label renderer entry has no live WorldView scope");
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 const auto*const frame=scope->frame;const auto*const publication=scope->publication;
 const auto*const operation=&scope->operation;const auto serial=scope->serial;
 const auto numeric=label_actual_numeric_profile_without_callbacks();
 std::shared_ptr<const ComposedUiFrame> frame_owner;
 std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 std::shared_ptr<BnwWorldViewControlTree> tree;
 std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const auto first=[&]{
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->renderer==actual&&
   life->active_scope==scope,"Actual Label issuing renderer retired");
  if(enclosing_entry){
   label_paint_frame_need(tree!=nullptr,"Actual Label child lost actual Tree capture");
   detail::BnwLabelActualPaintCallFrame::Impl::require_enclosing_entry_without_callbacks(*enclosing_entry,*tree);
  }
  label_paint_frame_need(frame&&publication&&scope->painting&&scope->frame==frame&&
   scope->publication==publication&&&scope->operation==operation&&scope->serial==serial,
   "Actual Label issuing frame/publication changed");
  scope->pure();
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Actual Label issuing numeric control profile changed");
  if(publication_owner)label_paint_frame_need(publication_owner.get()==publication,
   "Actual Label issuing original Publication control changed");
  if(frame_owner){
   label_paint_frame_need(frame_owner.get()==frame,
    "Actual Label issuing Frame differs from SAME adjusted owner");
   publication->require_adjusted_frame_owner_without_callbacks(frame_owner);
  }
 };
 const auto full=[&]{first();scope->full();first();};
 first();
 // Private publication fields are read by their genuine renderer friend only.
 label_paint_frame_need(publication->renderer_==actual&&publication->creator_==life->creator&&
  publication->tree_,"Actual Label publication has another renderer/control");
 // Capture the SAME immutable adjusted owner under current publication
 // FIRST before control-ID/Source FULL and before any facade allocation.
 // Root already created this original object before actual Frame entry.
 publication_owner=publication->original_owner_without_callbacks();first();
 tree=publication_owner->tree_;first();
 const auto captured_enclosing=publication_owner->actual_enclosing_root_context_entry_without_callbacks();first();
 if(captured_enclosing)enclosing_entry.emplace(*captured_enclosing);first();
 frame_owner=publication_owner->actual_adjusted_frame_owner_for_paint_without_callbacks(*actual,*frame);
 first();
 const detail::BnwLabelActualPaintCallFrame::IssuerKey key;
 std::shared_ptr<const detail::BnwLabelActualPaintCallFrame> result;
 {
  const auto control_id=publication->actual_checked_control_id_for_paint_without_callbacks();first();
  full();
  // All constructor arguments are value snapshots before make_shared allocation.
  auto data=std::make_shared<detail::BnwLabelActualPaintCallFrame::Impl>(
   std::weak_ptr<WorldViewPaintLifetime>(life),actual,scope,frame_owner,frame,publication_owner,publication,operation,serial,
   control_id,enclosing_entry,std::weak_ptr<BnwWorldViewControlTree>(tree),numeric);
  first();full();data->first();
  result=std::shared_ptr<const detail::BnwLabelActualPaintCallFrame>(
   new detail::BnwLabelActualPaintCallFrame(key,std::move(data)));
  first();full();result->capture_guard_without_callbacks().require_source_owned(*operation);first();
 }
 first();full();result->capture_guard_without_callbacks().require_current_without_callbacks();first();
 return result;
}

// Separate generic actual Root entry. Existing Label Frame above is untouched.
struct detail::BnwNormalRootPaintCallFrame::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> renderer_life;
 const UiScreenGlRenderer*const renderer;
 const UiScreenGlRenderer::WorldViewPaintScope*const scope;
 // Physical custody of the SAME original adjusted object; no active
 // item/Source/Root authority is prolonged by this strong owner.
 const std::shared_ptr<const ComposedUiFrame> frame_owner;
 const ComposedUiFrame*const frame;
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 const BnwWorldViewControlRenderPublication*const publication;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t serial;
 const std::optional<std::uint64_t> control_id;
 const std::optional<detail::BnwNormalRootContextEntry> context_entry;
 const std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const std::weak_ptr<BnwWorldViewControlTree> tree;
 const std::array<std::uint32_t,2> numeric;
 Impl(std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> life,const UiScreenGlRenderer* actual,
      const UiScreenGlRenderer::WorldViewPaintScope* entered,
      std::shared_ptr<const ComposedUiFrame> owned,const ComposedUiFrame* composed,
      std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner,
      const BnwWorldViewControlRenderPublication* current,
      const runtime::BnwWorldViewSourceOperationLoan* source,std::uint64_t operation_serial,
      std::optional<std::uint64_t> control,
      std::optional<detail::BnwNormalRootContextEntry> context,
      std::optional<detail::BnwNormalRootContextEntry> enclosing,
      std::weak_ptr<BnwWorldViewControlTree> actual_tree,
      std::array<std::uint32_t,2> profile)
  :renderer_life(std::move(life)),renderer(actual),scope(entered),frame_owner(std::move(owned)),
   frame(composed),publication_owner(std::move(original_owner)),publication(current),
   operation(source),serial(operation_serial),control_id(control),context_entry(std::move(context)),
   enclosing_entry(std::move(enclosing)),tree(std::move(actual_tree)),numeric(profile){}
 static void require_context_entry_pair_without_callbacks(
  const detail::BnwNormalRootContextEntry& original,
  const detail::BnwNormalRootContextEntry& supplied,const BnwWorldViewControlTree& actual_tree){
  // Only the Generic Frame friend reads the closed Entry. Ui never acquires
  // an Entry issuer/read friend or constructs its private lexical Scope.
  original.require_current_without_callbacks();
  original.require_same_entry_without_callbacks(supplied);
  original.require_same_tree_without_callbacks(actual_tree);
  supplied.require_same_tree_without_callbacks(actual_tree);
  original.require_current_without_callbacks();supplied.require_current_without_callbacks();
 }
 void first()const{
  label_paint_frame_need(alive.load()&&creator==std::this_thread::get_id(),
   "Actual generic Root renderer paint callframe retired or foreign creator");
  label_paint_frame_need(publication_owner&&publication_owner.get()==publication,
   "Generic Root Frame lost SAME original Publication physical owner");
  label_paint_frame_need(frame_owner&&frame_owner.get()==frame,
   "Actual generic Root Frame lost SAME original owned adjusted backing");
  // Independent actual renderer destruction and exact active stack precede
  // every borrowed scope/frame/publication/operation dereference.
  const auto life=renderer_life.lock();
  label_paint_frame_need(life&&life->creator==creator&&life->alive&&!life->failed&&life->active&&
   life->renderer==renderer&&life->active_scope==scope&&scope,
   "Actual generic Root renderer paint scope retired or replaced");
  label_paint_frame_need(bool(control_id)!=bool(context_entry),
   "Generic Frame must retain exactly one genuine Control or ContextEntry association");
  label_paint_frame_need(!enclosing_entry||!context_entry,
   "Only an indexed child retains a separate enclosing Context Entry");
  if(enclosing_entry){
   enclosing_entry->require_current_without_callbacks();
   const auto actual_tree=tree.lock();label_paint_frame_need(actual_tree!=nullptr,
    "Generic child lost original enclosing Context Tree");
   require_context_entry_pair_without_callbacks(*enclosing_entry,*enclosing_entry,*actual_tree);
  }
  if(context_entry){
   context_entry->require_current_without_callbacks();
   const auto actual_tree=tree.lock();label_paint_frame_need(actual_tree!=nullptr,
    "Generic Context Frame lost SAME actual Tree");
   context_entry->require_same_tree_without_callbacks(*actual_tree);
  }
  label_paint_frame_need(scope->painting&&frame&&publication&&scope->frame==frame&&
   scope->publication==publication&&&scope->operation==operation&&scope->serial==serial,
   "Actual generic Root requires SAME entered frame/publication/Source");
  scope->pure();
  label_paint_frame_need(alive.load()&&life->alive&&!life->failed&&life->active&&
   life->active_scope==scope&&scope->painting&&scope->frame==frame&&scope->publication==publication,
   "Actual generic Root scope changed during pure origin checks");
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Actual generic Root numeric control profile changed");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& source)const{
  first();label_paint_frame_need(&source==operation,"Actual generic Root has another Source operation");
  if(enclosing_entry){enclosing_entry->require_source_owned(source);first();}
  source.require_operation(serial);first();scope->full();first();
  if(enclosing_entry){enclosing_entry->require_source_owned(source);first();}
  if(context_entry){context_entry->require_source_owned(source);first();}
  source.require_operation(serial);first();
 }
};
detail::BnwNormalRootPaintCallFrame::BnwNormalRootPaintCallFrame(
 const IssuerKey&,std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwNormalRootPaintCallFrame::~BnwNormalRootPaintCallFrame(){impl_->alive.store(false);}
detail::BnwNormalRootPaintCallFrame::Guard::Guard(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwNormalRootPaintCallFrame::Guard detail::BnwNormalRootPaintCallFrame::capture_guard_without_callbacks()const{
 const auto data=impl_;data->first();return Guard(data);
}
void detail::BnwNormalRootPaintCallFrame::Guard::require_current_without_callbacks()const{
 const auto data=impl_;data->first();
}
void detail::BnwNormalRootPaintCallFrame::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& source)const{
 const auto data=impl_;data->full(source);data->first();
}
void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_control_without_callbacks(
 const BnwWorldViewControlTree& actual,std::uint64_t control)const{
 const auto data=impl_;data->first();
 label_paint_frame_need(data->control_id&&!data->context_entry,
  "Generic Context Frame cannot grant an ordinary Control association");
 {const auto tree=data->tree.lock();label_paint_frame_need(tree&&tree.get()==&actual&&
   control==*data->control_id,"Actual generic Root paint has another canonical control");}
 data->first();
}
void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_renderer_without_callbacks(
 const UiScreenGlRenderer& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->renderer==&actual,
  "Actual generic Root paint has another renderer");data->first();
}
std::array<std::uint32_t,2> detail::BnwNormalRootPaintCallFrame::Guard::actual_numeric_profile_without_callbacks()const{
 const auto data=impl_;data->first();const auto result=data->numeric;data->first();return result;
}
// Separate immutable root-scope health, earned while original ACTIVE Frame lives.
struct detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::Impl final {
 const std::thread::id creator;
 const std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> renderer_life;
 const UiScreenGlRenderer*const renderer;
 const UiScreenGlRenderer::WorldViewPaintScope*const scope;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t serial;
 // Opaque association only: no history method dereferences this pointer.
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 const BnwWorldViewControlRenderPublication*const publication;
 const std::shared_ptr<const ComposedUiFrame> frame_owner;
 const std::optional<detail::BnwNormalRootContextEntry> context_entry;
 const std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const std::array<std::uint32_t,2> numeric;
 Impl(std::thread::id actual_creator,
  std::weak_ptr<UiScreenGlRenderer::WorldViewPaintLifetime> life,const UiScreenGlRenderer* actual,
  const UiScreenGlRenderer::WorldViewPaintScope* entered,
  const runtime::BnwWorldViewSourceOperationLoan* source,std::uint64_t source_serial,
  std::shared_ptr<const BnwWorldViewControlRenderPublication> original_owner,
  const BnwWorldViewControlRenderPublication* original,std::shared_ptr<const ComposedUiFrame> owned,
  std::optional<detail::BnwNormalRootContextEntry> context,
  std::optional<detail::BnwNormalRootContextEntry> enclosing,std::array<std::uint32_t,2> profile)
  :creator(actual_creator),renderer_life(std::move(life)),renderer(actual),scope(entered),
   operation(source),serial(source_serial),publication_owner(std::move(original_owner)),
   publication(original),frame_owner(std::move(owned)),context_entry(std::move(context)),
   enclosing_entry(std::move(enclosing)),numeric(profile){}
 void first()const{
  label_paint_frame_need(creator==std::this_thread::get_id(),
   "Root scope health has a foreign creator");
  // Genuine weak life and exact active_scope precede EVERY borrowed scope or
  // Source read. No ACTIVE Frame, painting, frame or Label item is consulted.
  const auto life=renderer_life.lock();
  label_paint_frame_need(life&&life->creator==creator&&life->alive&&!life->failed&&life->active&&
   life->renderer==renderer&&scope&&life->active_scope==scope,
   "Root scope health actual renderer/scope retired or replaced");
  // Genuine Context scope checker, never old Frame/item/painting and never
  // merely Core.busy=true. Root Scope retirement must remain once-only.
  if(enclosing_entry)enclosing_entry->require_current_without_callbacks();
  if(context_entry)context_entry->require_current_without_callbacks();
  label_paint_frame_need(&scope->operation==operation&&scope->serial==serial,
   "Root scope health actual Source tuple changed");
  scope->operation.require_origin_without_callbacks();scope->pure();
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->creator==creator&&
   life->renderer==renderer&&life->active_scope==scope&&&scope->operation==operation&&scope->serial==serial,
   "Root scope health changed during pure projection/tree checks");
  label_paint_frame_need(frame_owner&&publication&&publication_owner&&publication_owner.get()==publication,
   "Root scope health original immutable backing/association absent");
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Root scope health numeric control profile changed");
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->creator==creator&&
   life->renderer==renderer&&life->active_scope==scope,
   "Root scope health actual life/scope changed after numeric observation");
  if(enclosing_entry)enclosing_entry->require_current_without_callbacks();
  label_paint_frame_need(&scope->operation==operation&&scope->serial==serial,
   "Root scope health actual Source tuple changed at pure POST");
  scope->operation.require_origin_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& input)const{
  first();label_paint_frame_need(&input==operation,"Root scope health has another Source");
  if(enclosing_entry){enclosing_entry->require_source_owned(input);first();}
  input.require_operation(serial);first();scope->full();first();
  if(enclosing_entry){enclosing_entry->require_source_owned(input);first();}
  if(context_entry){context_entry->require_source_owned(input);first();}
  input.require_operation(serial);first();
 }
};
detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::RootScopeHealthGuard(
 std::shared_ptr<const Impl> data,const CaptureKey&):impl_(std::move(data)){}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_physical_current_without_callbacks()const{
 const auto data=impl_;data->first();
}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& input)const{
 const auto data=impl_;data->full(input);data->first();
}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_same_root_scope_without_callbacks(
 const RootScopeHealthGuard& other)const{
 const auto data=impl_;const auto saved=other.impl_;data->first();saved->first();
 label_paint_frame_need(data->renderer==saved->renderer&&data->scope==saved->scope&&
  data->operation==saved->operation&&data->serial==saved->serial&&
  !data->renderer_life.owner_before(saved->renderer_life)&&
  !saved->renderer_life.owner_before(data->renderer_life),
  "Root scope health belongs to another genuine root scope");
 data->first();saved->first();
}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_actual_renderer_without_callbacks(
 const UiScreenGlRenderer& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->renderer==&actual,
  "Root scope health belongs to another actual renderer");data->first();
}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_actual_publication_without_callbacks(
 const BnwWorldViewControlRenderPublication& actual)const{
 const auto data=impl_;data->first();label_paint_frame_need(data->publication==&actual,
  "Root scope health has another original publication association");data->first();
}
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_actual_frame_owner_without_callbacks(
 const std::shared_ptr<const ComposedUiFrame>& actual)const{
 const auto data=impl_;const auto owner=actual;data->first();
 label_paint_frame_need(owner&&owner.get()==data->frame_owner.get()&&
  !owner.owner_before(data->frame_owner)&&!data->frame_owner.owner_before(owner),
  "Root scope health has another original adjusted backing control");data->first();
}
std::array<std::uint32_t,2>
detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::numeric_controls_without_callbacks()const{
 const auto data=impl_;data->first();const auto result=data->numeric;data->first();return result;
}
detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard
detail::BnwNormalRootPaintCallFrame::Guard::capture_root_scope_health_owned(
 const runtime::BnwWorldViewSourceOperationLoan& input)const{
 // Snap original storage first, then original ACTIVE FIRST/FULL before any new
 // owner or history-control allocation. Capture is never allowed after return.
 const auto data=impl_;const auto*const supplied=&input;
 data->first();data->full(*supplied);data->first();
 const auto frame_owner=data->publication->actual_adjusted_frame_owner_for_paint_without_callbacks(
  *data->renderer,*data->frame);data->first();
 label_paint_frame_need(frame_owner&&frame_owner.get()==data->frame&&
  frame_owner.get()==data->frame_owner.get()&&!frame_owner.owner_before(data->frame_owner)&&
  !data->frame_owner.owner_before(frame_owner),"Root scope capture changed SAME adjusted Frame control");
 // Every tuple operand is a VALUE snapshot before allocation/Source callbacks.
 const auto creator=data->creator;const auto life=data->renderer_life;
 const auto*const renderer=data->renderer;const auto*const scope=data->scope;
 const auto*const operation=data->operation;const auto serial=data->serial;
 const auto publication_owner=data->publication_owner;const auto context_entry=data->context_entry;
 const auto enclosing_entry=data->enclosing_entry;
 const auto*const publication=data->publication;const auto numeric=data->numeric;
 const auto original_first=[&]{data->first();
  label_paint_frame_need(data->creator==creator&&data->renderer==renderer&&data->scope==scope&&
   data->operation==operation&&data->serial==serial&&data->publication==publication&&data->numeric==numeric&&
   !life.owner_before(data->renderer_life)&&!data->renderer_life.owner_before(life)&&
   data->frame==frame_owner.get()&&data->frame_owner.get()==frame_owner.get()&&
   !data->frame_owner.owner_before(frame_owner)&&!frame_owner.owner_before(data->frame_owner),
   "Root scope capture immutable original tuple changed");};
 original_first();data->full(*supplied);original_first();
 auto history=std::make_shared<const RootScopeHealthGuard::Impl>(creator,life,renderer,scope,
  operation,serial,publication_owner,publication,frame_owner,context_entry,enclosing_entry,numeric);
 original_first();data->full(*supplied);original_first();history->first();original_first();
 const RootScopeHealthGuard::CaptureKey key;
 RootScopeHealthGuard result(history,key);
 original_first();result.require_physical_current_without_callbacks();original_first();
 return result;
}

void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_publication_without_callbacks(
 const BnwWorldViewControlRenderPublication& actual)const{
 const auto data=impl_;data->first();
 label_paint_frame_need(data->publication==&actual&&data->publication_owner.get()==&actual,
  "Generic Root Frame belongs to another original Publication");data->first();
}
void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_context_entry_without_callbacks(
 const BnwWorldViewControlRenderPublication& supplied_publication,
 const detail::BnwNormalRootContextEntry& supplied_entry)const{
 const auto data=impl_;const auto entry=supplied_entry;
 const auto*const publication=&supplied_publication;data->first();
 label_paint_frame_need(data->context_entry&&!data->control_id&&
  data->publication==publication&&data->publication_owner.get()==publication,
  "Generic Context Frame belongs to another original Publication or ordinary Control");
 // SAME strong original Publication is already retained by Impl before this
 // private pure getter. It returns the canonical Entry VALUE, never a new one.
 const auto from_publication=publication->actual_root_context_entry_for_paint_without_callbacks();data->first();
 label_paint_frame_need(from_publication.has_value(),
  "Generic Context Frame publication is not its genuine Context receiver kind");
 const auto tree=data->tree.lock();label_paint_frame_need(tree!=nullptr,
  "Generic Context Frame lost SAME original Tree");
 Impl::require_context_entry_pair_without_callbacks(*data->context_entry,entry,*tree);data->first();
 Impl::require_context_entry_pair_without_callbacks(*from_publication,entry,*tree);data->first();
}

void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_frame_owner_without_callbacks(
 const std::shared_ptr<const ComposedUiFrame>& input)const{
 const auto data=impl_;const auto owned=input;data->first();
 label_paint_frame_need(owned&&owned.get()==data->frame&&owned.get()==data->frame_owner.get()&&
  !owned.owner_before(data->frame_owner)&&!data->frame_owner.owner_before(owned),
  "Generic Root Frame has another original adjusted frame owner");data->first();
}
std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>
UiScreenGlRenderer::WorldViewPaintScope::capture_actual_normal_root_frame_owned(
 UiScreenGlRenderer& supplied)const{
 // Read raw renderer members once before any callback/allocation.
 const auto life=this->life;const auto*const actual=&supplied;
 label_paint_frame_need(life&&life->creator==std::this_thread::get_id()&&life->alive&&!life->failed&&
  life->active&&life->renderer==actual&&life->active_scope,
  "Actual generic Root renderer entry has no live WorldView scope");
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 label_paint_frame_need(scope==this,"Generic Root Frame issuer is another active scope");
 const auto*const frame=scope->frame;const auto*const publication=scope->publication;
 const auto*const operation=&scope->operation;const auto serial=scope->serial;
 const auto numeric=label_actual_numeric_profile_without_callbacks();
 std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 std::shared_ptr<const ComposedUiFrame> frame_owner;
 std::shared_ptr<BnwWorldViewControlTree> tree;
 std::optional<detail::BnwNormalRootContextEntry> enclosing_entry;
 const auto first=[&]{
  label_paint_frame_need(life->alive&&!life->failed&&life->active&&life->renderer==actual&&
   life->active_scope==scope,"Actual generic Root issuing renderer retired");
  if(enclosing_entry){
   label_paint_frame_need(tree!=nullptr,"Generic child lost actual Tree capture");
   detail::BnwNormalRootPaintCallFrame::Impl::require_context_entry_pair_without_callbacks(
    *enclosing_entry,*enclosing_entry,*tree);
  }
  label_paint_frame_need(frame&&publication&&scope->painting&&scope->frame==frame&&
   scope->publication==publication&&&scope->operation==operation&&scope->serial==serial,
   "Actual generic Root issuing frame/publication changed");
  scope->pure();
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Actual generic Root issuing numeric control profile changed");
  if(publication_owner)label_paint_frame_need(publication_owner.get()==publication,
   "Generic Root entry changed original Publication owner");
  if(frame_owner){
   label_paint_frame_need(frame_owner.get()==frame,
    "Actual generic Root issuing Frame differs from SAME adjusted owner");
   publication->require_adjusted_frame_owner_without_callbacks(frame_owner);
  }
 };
 const auto full=[&]{first();scope->full();first();};
 first();
 // Private publication fields are read by their genuine renderer friend only.
 label_paint_frame_need(publication->renderer_==actual&&publication->creator_==life->creator&&
  publication->tree_,"Actual generic Root publication has another renderer/control");
 // Capture the SAME immutable adjusted owner under current publication
 // FIRST before control-ID/Source FULL and before any facade allocation.
 // Root already created this original object before actual Frame entry.
 publication_owner=publication->original_owner_without_callbacks();first();
 tree=publication_owner->tree_;first();
 const auto captured_enclosing=publication_owner->actual_enclosing_root_context_entry_without_callbacks();first();
 if(captured_enclosing)enclosing_entry.emplace(*captured_enclosing);first();
 frame_owner=publication->actual_adjusted_frame_owner_for_paint_without_callbacks(*actual,*frame);
 first();
 const detail::BnwNormalRootPaintCallFrame::IssuerKey key;
 std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> result;
 {
  const auto control_id=publication->actual_checked_control_id_for_paint_without_callbacks();first();
  full();
  // All constructor arguments are value snapshots before make_shared allocation.
  auto data=std::make_shared<detail::BnwNormalRootPaintCallFrame::Impl>(
   std::weak_ptr<WorldViewPaintLifetime>(life),actual,scope,frame_owner,frame,publication_owner,publication,operation,serial,
   control_id,std::nullopt,enclosing_entry,std::weak_ptr<BnwWorldViewControlTree>(tree),numeric);
  first();full();data->first();
  result=std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>(
   new detail::BnwNormalRootPaintCallFrame(key,std::move(data)));
  first();full();result->capture_guard_without_callbacks().require_source_owned(*operation);first();
 }
 first();full();result->capture_guard_without_callbacks().require_current_without_callbacks();first();
 return result;
}

std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>
UiScreenGlRenderer::WorldViewPaintScope::capture_actual_normal_root_frame_owned(
 UiScreenGlRenderer& supplied,const detail::BnwNormalRootContextEntry& supplied_entry)const{
 using Frame=detail::BnwNormalRootPaintCallFrame;
 // Capture the closed value before callbacks, without access to Entry's key,
 // lexical Scope constructor or private reader outside Generic Frame.
 const auto entry=supplied_entry;const auto saved_life=life;
 const auto*const renderer=&supplied;const auto*const saved_scope=this;
 label_paint_frame_need(saved_life&&saved_life->creator==std::this_thread::get_id()&&
  saved_life->alive&&!saved_life->failed&&saved_life->active&&
  saved_life->renderer==renderer&&saved_life->active_scope==saved_scope,
  "Context generic Frame requires original live Ui scope");
 const auto*const actual_frame=saved_scope->frame;
 const auto*const publication=saved_scope->publication;
 const auto*const operation=&saved_scope->operation;const auto serial=saved_scope->serial;
 const auto numeric=label_actual_numeric_profile_without_callbacks();
 std::shared_ptr<const BnwWorldViewControlRenderPublication> publication_owner;
 std::shared_ptr<const ComposedUiFrame> frame_owner;
 std::shared_ptr<BnwWorldViewControlTree> tree;
 std::optional<detail::BnwNormalRootContextEntry> canonical_entry;
 const auto first=[&]{
  label_paint_frame_need(saved_life->creator==std::this_thread::get_id()&&
   saved_life->alive&&!saved_life->failed&&saved_life->active&&
   saved_life->renderer==renderer&&saved_life->active_scope==saved_scope,
   "Context generic Frame original Ui scope retired or changed");
  label_paint_frame_need(actual_frame&&publication&&saved_scope->painting&&
   saved_scope->frame==actual_frame&&saved_scope->publication==publication&&
   &saved_scope->operation==operation&&saved_scope->serial==serial,
   "Context generic Frame entered original frame/Publication/Source tuple changed");
  saved_scope->pure();
  const auto current=label_actual_numeric_profile_without_callbacks();
  label_paint_frame_need(current[0]==numeric[0]&&(current[1]&~std::uint32_t{0x3f})==
   (numeric[1]&~std::uint32_t{0x3f}),"Context generic Frame numeric controls changed");
  if(publication_owner)label_paint_frame_need(publication_owner.get()==publication,
   "Context generic Frame changed SAME original Publication owner");
  if(canonical_entry){
   label_paint_frame_need(tree!=nullptr,"Context generic Frame lost actual Tree custody");
   Frame::Impl::require_context_entry_pair_without_callbacks(*canonical_entry,entry,*tree);
  }
  if(frame_owner){
   label_paint_frame_need(frame_owner.get()==actual_frame&&actual_frame->paints.empty(),
    "Context generic Frame cannot substitute an indexed item or another frame");
   publication->require_adjusted_frame_owner_without_callbacks(frame_owner);
  }
 };
 first();
 // Strong originals precede the FIRST Source FULL and every allocator call.
 // Root15 owns creation and the adjusted object; this factory never copies it.
 publication_owner=publication->original_owner_without_callbacks();first();
 label_paint_frame_need(publication_owner&&publication_owner->renderer_==renderer&&
  publication_owner->creator_==saved_life->creator&&publication_owner->tree_,
  "Context generic Frame original Publication renderer/Tree differs");
 tree=publication_owner->tree_;first();
 const auto returned_entry=publication_owner->actual_root_context_entry_for_paint_without_callbacks();first();
 if(returned_entry)canonical_entry.emplace(*returned_entry);first();
 label_paint_frame_need(canonical_entry.has_value(),
  "Context generic Frame requires canonical genuine Context Publication");
 Frame::Impl::require_context_entry_pair_without_callbacks(*canonical_entry,entry,*tree);first();
 frame_owner=publication_owner->actual_adjusted_frame_owner_for_paint_without_callbacks(*renderer,*actual_frame);
 first();
 const auto full=[&]{first();saved_scope->full();first();};
 const Frame::IssuerKey key;
 full();
 auto physical=std::make_shared<Frame::Impl>(
  std::weak_ptr<WorldViewPaintLifetime>(saved_life),renderer,saved_scope,frame_owner,actual_frame,
  publication_owner,publication,operation,serial,std::nullopt,canonical_entry,std::nullopt,
  std::weak_ptr<BnwWorldViewControlTree>(tree),numeric);
 first();full();physical->first();
 auto result=std::shared_ptr<const Frame>(new Frame(key,std::move(physical)));
 first();full();
 const auto guard=result->capture_guard_without_callbacks();
 guard.require_actual_context_entry_without_callbacks(*publication,entry);first();
 guard.require_source_owned(*operation);first();
 guard.require_actual_frame_owner_without_callbacks(frame_owner);first();
 return result;
}

// A physical checker only. The header exposes no helper definition, callable
// getter, constructor or authority issuer. Friendship is ONLY the two health
// controls; ACTIVE Frame/Guard and all CaptureKey/IssuerKey classes are separate.
class detail::BnwNormalLabelChildScopeHealthBridge final {
 BnwNormalLabelChildScopeHealthBridge()=delete;
 BnwNormalLabelChildScopeHealthBridge(const BnwNormalLabelChildScopeHealthBridge&)=delete;
 BnwNormalLabelChildScopeHealthBridge&operator=(const BnwNormalLabelChildScopeHealthBridge&)=delete;
public:
 // Source-local retained-control access only; no helper member is declared
 // in a header. auto avoids naming the private LabelHealth::Impl type.
 static auto retain_original_label_control_without_callbacks(
  const detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard& label){
  return label.impl_;
 }
};
void detail::BnwNormalRootPaintCallFrame::RootScopeHealthGuard::require_actual_label_child_scope_without_callbacks(
 const BnwLabelActualPaintCallFrame::RootScopeHealthGuard& label,
 const BnwNormalRootContextEntry& offered_entry)const{
  const auto data=impl_;
  const auto child=BnwNormalLabelChildScopeHealthBridge::retain_original_label_control_without_callbacks(label);
  const auto entry=offered_entry;
  data->first();child->first();
  // Capture strong SAME original controls before inspecting retained fields.
  const auto parent=data->publication_owner;const auto original=child->publication_owner;
  const auto parent_frame=data->frame_owner;const auto child_frame=child->frame_owner;
  const auto same=[](const auto&a,const auto&b){
   return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
  };
  const auto pure=[&]{
   // Independent Ui life/exact Scope and original lexical Entry health FIRST.
   // Neither health FIRST depends on retired ACTIVE Frame/item/painting.
   data->first();child->first();
   label_paint_frame_need(data->context_entry&&!data->enclosing_entry&&child->enclosing_entry,
    "Label child health requires genuine Context Root and original enclosing Entry");
   data->context_entry->require_current_without_callbacks();
   child->enclosing_entry->require_current_without_callbacks();entry.require_current_without_callbacks();
   data->context_entry->require_same_entry_without_callbacks(*child->enclosing_entry);
   data->context_entry->require_same_entry_without_callbacks(entry);
   label_paint_frame_need(data->creator==child->creator&&data->renderer==child->renderer&&
    data->scope==child->scope&&data->operation==child->operation&&data->serial==child->serial&&
    !data->renderer_life.owner_before(child->renderer_life)&&
    !child->renderer_life.owner_before(data->renderer_life)&&
    data->numeric[0]==child->numeric[0]&&
    (data->numeric[1]&~std::uint32_t{0x3f})==(child->numeric[1]&~std::uint32_t{0x3f}),
    "Label child health belongs to another actual Ui Scope/life/renderer/Source tuple");
   label_paint_frame_need(parent&&original&&parent.get()==data->publication&&
    original.get()==child->publication&&parent.get()!=original.get()&&
    same(parent,data->publication_owner)&&same(original,child->publication_owner)&&
    parent->root_context_entry_&&!parent->control_&&!parent->enclosing_context_entry_&&
    parent->paint_indices_.empty()&&original->control_&&!original->root_context_entry_&&
    original->enclosing_context_entry_&&!original->paint_indices_.empty()&&
    parent->renderer_==data->renderer&&original->renderer_==data->renderer&&
    parent->creator_==data->creator&&original->creator_==data->creator,
    "Label child health original Publication receiver roles or physical owners changed");
   const auto tree=parent->tree_;const auto child_tree=original->tree_;
   label_paint_frame_need(tree&&child_tree&&same(tree,child_tree),
    "Label child health lost SAME actual original Context Tree");
   data->context_entry->require_same_tree_without_callbacks(*tree);
   child->enclosing_entry->require_same_tree_without_callbacks(*tree);
   entry.require_same_tree_without_callbacks(*tree);
   parent->root_context_entry_->require_same_entry_without_callbacks(*data->context_entry);
   original->enclosing_context_entry_->require_same_entry_without_callbacks(*child->enclosing_entry);
   // Physical original allocation/adjusted-owner fields ONLY. Do not call the
   // current publication or ACTIVE Frame getters after child Frame retirement.
   const auto canonical_parent=parent->original_owner_.lock();
   const auto canonical_child=original->original_owner_.lock();
   label_paint_frame_need(same(canonical_parent,parent)&&same(canonical_child,original)&&
    parent->original_owner_operation_==data->operation&&original->original_owner_operation_==child->operation&&
    parent->original_owner_source_serial_&&*parent->original_owner_source_serial_==data->serial&&
    original->original_owner_source_serial_&&*original->original_owner_source_serial_==child->serial&&
    parent_frame&&child_frame&&same(parent_frame,data->frame_owner)&&same(child_frame,child->frame_owner)&&
    same(parent->adjusted_frame_owner_,parent_frame)&&same(original->adjusted_frame_owner_,child_frame)&&
    parent->adjusted_frame_operation_==data->operation&&original->adjusted_frame_operation_==child->operation&&
    parent->adjusted_frame_source_serial_&&*parent->adjusted_frame_source_serial_==data->serial&&
    original->adjusted_frame_source_serial_&&*original->adjusted_frame_source_serial_==child->serial,
    "Label child health canonical original allocation or OWN adjusted-frame association changed");
   // Each Publication keeps its OWN actual adjusted frame. Caption geometry
   // need not alias the whole Root frame, and physical custody renews no Scope.
   data->context_entry->require_current_without_callbacks();
   child->enclosing_entry->require_current_without_callbacks();entry.require_current_without_callbacks();
   data->first();child->first();
  };
  pure();pure(); // Callback-free PURE PRE/POST; no effects/Source FULL/issuance.
}
