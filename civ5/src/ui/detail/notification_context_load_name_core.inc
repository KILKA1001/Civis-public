// Inside civ5::ui AFTER complete Source8 native constructor/physical Context.
// Only the native positive731370 offset0 assignment slice is implemented.
// The source-issued InputLoan has NO issuer in this package. In particular,
// no metadata/XML/descriptor string can enter the setter through another API.
struct detail::NotificationContextNativeLoadNameInputLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const NotificationContextNativeConstructorReturn> constructor;
    const std::shared_ptr<const NotificationContextNativeWideStringValue> actual_argument;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    const std::uint64_t source_scope;
    std::weak_ptr<Impl> canonical_input;
    bool storing=false;
    std::uint64_t named_store_revision=0;
    // Only a future closed original loader-input issuer may invoke this VALUE
    // constructor after actual input provenance and lexical entry are earned.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> s,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> state,
        std::shared_ptr<const NotificationContextNativeConstructorReturn> done,
        std::shared_ptr<const NotificationContextNativeWideStringValue> input,
        const runtime::BnwWorldViewSourceOperationLoan*op,std::uint64_t current_serial,std::uint64_t actual_source_scope)
        :core(std::move(c)),source(std::move(s)),context(std::move(state)),constructor(std::move(done)),
         actual_argument(std::move(input)),operation(op),serial(current_serial),source_scope(actual_source_scope){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Context731370 input lexical frame retired/foreign/failed FIRST");
        require(operation==&op,"Context731370 actual operation identity changed before old operand");
        const auto actual=core.lock();const auto owner=source.lock();const auto state=context.lock();
        require(actual&&owner&&state&&state->impl_,"Context731370 physical input owners retired");
        require(NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner),
            "Context731370 canonical Source control replaced FIRST");
        const auto current_scope=owner->capture_active_operation_scope_serial_without_callbacks(&op);
        require(source_scope&&current_scope==source_scope,"Context731370 old input Source scope cannot renew at reused pointer");
        owner->require_operation_origin_without_callbacks(op,source_scope);
        require(serial&&serial==op.operation_serial(),"Context731370 input SAME fresh operation changed");
        const auto storage=state->impl_;
        require(storage->creator==std::this_thread::get_id()&&storage->alive.load()&&!storage->failed.load(),
            "Context731370 actual physical Context retired/failed FIRST");
        actual->layout_origin();op.require_origin_without_callbacks();
        const auto canonical=canonical_input.lock();const auto active=actual->context_load_name_frame.lock();
        require(canonical&&canonical.get()==this&&active&&
            NotificationLayoutContextOrdinaryState::Impl::same(active,canonical),
            "Context731370 SAME active closed load frame/control replaced or absent");
        require(NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            &owner->current_operation_loan()==&op&&constructor&&constructor->impl_&&actual_argument&&
            NotificationLayoutContextOrdinaryState::Impl::same(storage->native_constructor_return,constructor)&&
            actual->context_layout_baseline&&
            actual->context_layout_baseline->kind_==NotificationLayoutBaseline::Kind::context&&
            actual->context_layout_baseline->actual_type_=="Context",
            "Context731370 actual constructor/Context/Source/input control changed");
        const auto done=constructor->impl_;
        require(done->creator==std::this_thread::get_id()&&done->alive.load()&&!done->failed.load()&&done->port,
            "Context731370 full constructor retired/failed FIRST");
        require(NotificationLayoutContextOrdinaryState::Impl::same(done->core.lock(),actual)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->source.lock(),owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->projection.lock(),state)&&
            done->prefix&&done->prefix->impl_&&
            NotificationLayoutContextOrdinaryState::Impl::same(storage->native_constructor_prefix,done->prefix),
            "Context731370 full constructor actual identity changed");
        const auto prefix=done->prefix->impl_;
        require(prefix->creator==std::this_thread::get_id()&&prefix->alive.load()&&!prefix->failed.load()&&prefix->port,
            "Context731370 actual constructor prefix retired/failed FIRST");
        require(done->serial&&done->serial==prefix->serial&&
            NotificationLayoutContextOrdinaryState::Impl::same(prefix->core.lock(),actual)&&
            NotificationLayoutContextOrdinaryState::Impl::same(prefix->source.lock(),owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(prefix->projection.lock(),state),
            "Context731370 constructor provenance changed (fresh load never renews constructor serial)");
        prefix->port->require_reached();done->port->require_completed();
        actual->layout_origin();op.require_origin_without_callbacks();
    }
};
detail::NotificationContextNativeLoadNameInputLoan::NotificationContextNativeLoadNameInputLoan(std::shared_ptr<Impl> p)
    :impl_(std::move(p)){}
void detail::NotificationContextNativeLoadNameInputLoan::retire_owned()const noexcept{impl_->alive.store(false);}
detail::NotificationContextNativeLoadNameInputLoan::~NotificationContextNativeLoadNameInputLoan(){retire_owned();}

struct detail::NotificationContextLoadNameLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::shared_ptr<NotificationContextNativeLoadNameInputLoan::Impl> input;
    const std::shared_ptr<NotificationLayoutContextOrdinaryState::Impl> storage;
    const std::shared_ptr<const NotificationContextNativeWideStringValue>*const field;
    const std::shared_ptr<const NotificationContextNativeWideStringValue> value;
    const std::uint64_t revision;
    mutable bool resize_registering=false;
    mutable std::weak_ptr<const NotificationContextResizeRegistrationReturn> resize_return;
    mutable std::weak_ptr<NotificationLayoutResizeRegistration> resize_entry;
    Impl(std::shared_ptr<NotificationContextNativeLoadNameInputLoan::Impl> actual_input,
        std::shared_ptr<NotificationLayoutContextOrdinaryState::Impl> actual_storage,
        const std::shared_ptr<const NotificationContextNativeWideStringValue>*actual_field,
        std::shared_ptr<const NotificationContextNativeWideStringValue> actual_value,std::uint64_t actual_revision)
        :input(std::move(actual_input)),storage(std::move(actual_storage)),field(actual_field),
         value(std::move(actual_value)),revision(actual_revision){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "ContextB4 load-name loan retired/foreign/failed FIRST");
        require(bool(input),"ContextB4 actual input lexical lender absent");input->first(op);
        const auto state=input->context.lock();
        require(state&&input->named_store_revision==revision&&
            NotificationLayoutContextOrdinaryState::Impl::same(state->impl_,storage)&&
            field==&storage->b4_value&&value&&revision&&storage->b4_revision==revision&&
            NotificationLayoutContextOrdinaryState::Impl::same(storage->b4_value,value),
            "ContextB4 SAME physical field/value/control/history changed");
        require(!value->value_.empty()&&value->terminated_data_without_callbacks()[value->value_.size()]==u'\0',
            "ContextB4 actual named UTF16/NUL field unissued");
    }
};
detail::NotificationContextLoadNameLoan::NotificationContextLoadNameLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
void detail::NotificationContextLoadNameLoan::retire_owned()const noexcept{impl_->alive.store(false);}
detail::NotificationContextLoadNameLoan::~NotificationContextLoadNameLoan(){retire_owned();}
void detail::NotificationContextLoadNameLoan::require_current_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
    const auto saved=impl_;saved->first(op);const auto actual=saved->input->core.lock();
    require(actual&&prepared&&NotificationLayoutContextOrdinaryState::Impl::same(actual->context_lua_preparation,prepared),
        "ContextB4 borrower SAME installed518 preparation changed");
    saved->first(op);
}
void detail::NotificationContextLoadNameLoan::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
    require_current_without_callbacks(op,prepared);op.require_current();require_current_without_callbacks(op,prepared);
}
std::u16string detail::NotificationContextLoadNameLoan::b4_utf16_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared)const{
    const auto saved=impl_;
    try{
        require_current_owned(op,prepared);const auto value=saved->value;
        std::u16string result;result.resize(value->value_.size());
        require_current_without_callbacks(op,prepared);op.require_current();require_current_without_callbacks(op,prepared);
        // Retained immutable physical DATA, exact full codeunits; copy cannot allocate.
        std::copy(value->value_.begin(),value->value_.end(),result.begin());
        require_current_owned(op,prepared);return result;
    }catch(...){saved->failed.store(true);throw;}
}

std::shared_ptr<const detail::NotificationContextLoadNameLoan>
BnwWorldViewControlTree::store_context_native_load_name_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const detail::NotificationContextNativeLoadNameInputLoan& input){
    using Input=detail::NotificationContextNativeLoadNameInputLoan;
    using Loan=detail::NotificationContextLoadNameLoan;using Wide=detail::NotificationContextNativeWideStringValue;
    const auto lender=input.impl_;require(bool(lender),"Context731370 input closed lender absent");lender->first(op);
    const auto actual=lender->core.lock();require(actual&&actual.get()==core_.get()&&
        !actual.owner_before(core_)&&!core_.owner_before(actual),"Context731370 foreign actual Tree");
    const auto context=lender->context.lock();
    require(context&&context->impl_,"Context731370 actual physical state retired before capture");
    const auto storage=context->impl_;
    const auto before=storage->b4_value;const auto argument=lender->actual_argument;
    const auto revision=storage->b4_revision;
    require(before&&argument&&revision<UINT64_MAX,"Context731370 constructorB4/input/history unissued or exhausted");
    const auto*const baseline=&*actual->context_layout_baseline;
    const auto sequence=baseline->sequence_;
    const auto history_size=baseline->history_.size();
    const auto effects=actual->effect_revision,topology=actual->topology_revision;
    const auto field=&storage->b4_value;
    const auto current=[&]{lender->first(op);
        require(field==&storage->b4_value&&storage->b4_revision==revision&&
            detail::NotificationLayoutContextOrdinaryState::Impl::same(storage->b4_value,before)&&
            actual->context_layout_baseline&& &*actual->context_layout_baseline==baseline&&
            actual->context_layout_baseline->sequence_==sequence&&
            actual->context_layout_baseline->history_.size()==history_size&&actual->effect_revision==effects&&
            actual->topology_revision==topology,
            "Context731370 B4 preimage/constructor history changed across callback/allocation");};
    const auto full=[&]{current();op.require_current();current();};
    try{
        require(!lender->storing&&!lender->named_store_revision,"Context731370 input assignment replay/reentry");
        lender->storing=true;struct Exit{bool& value;~Exit(){value=false;}}exit{lender->storing};
        full();require(!argument->value_.empty()&&argument->value_.size()<=static_cast<std::size_t>(INT32_MAX),
            "Context731370 selected nonempty positive wide argument required (empty/negative branch unsupported)");
        require(std::find(argument->value_.begin(),argument->value_.end(),u'\0')==argument->value_.end(),
            "Context731370 embeddedNUL load input unsupported");
        //731370 always supplies offset0 and the true wide logical length.
        //81FEDE actual data-pointer equality is a NOOP even when length differs.
        require(argument->terminated_data_without_callbacks()!=before->terminated_data_without_callbacks(),
            "Context731370 alias/NOOP argument cannot issue a new named-load store");
        std::u16string copied;copied.resize(argument->value_.size());full();
        std::copy(argument->value_.begin(),argument->value_.end(),copied.begin());full();
        // Moved owned VALUE; after newhandler the initializer reads no raw field.
        auto pending=std::unique_ptr<Wide>(new Wide(std::move(copied)));full();
        auto value=std::shared_ptr<const Wide>(std::move(pending));full();
        require(value->value_==argument->value_&&value->terminated_data_without_callbacks()[value->value_.size()]==u'\0',
            "Context731370 actual wide assignment/NUL changed");
        auto data=std::make_shared<Loan::Impl>(lender,storage,field,value,revision+1);full();
        auto pending_loan=std::unique_ptr<Loan>(new Loan(data));full();
        auto result=std::shared_ptr<const Loan>(std::move(pending_loan));full();
        // Real nonallocating named field store; exact revision follows the store.
        storage->b4_value=value;storage->b4_revision=revision+1;lender->named_store_revision=revision+1;
        data->first(op);op.require_current();data->first(op);
        require(actual->context_layout_baseline&& &*actual->context_layout_baseline==baseline&&
            actual->context_layout_baseline->sequence_==sequence&&
            actual->context_layout_baseline->history_.size()==history_size&&actual->effect_revision==effects&&
            actual->topology_revision==topology,"Context731370 B4 publication changed unrelated actual history");
        return result;
    }catch(...){lender->failed.store(true);storage->failed.store(true);actual->failed.store(true);throw;}
}
#include "notification_context_resize_registration_core.inc"
