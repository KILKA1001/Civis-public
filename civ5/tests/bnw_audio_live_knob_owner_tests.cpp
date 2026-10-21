#include "civ5/ui/bnw_audio_live_knob_owner.hpp"
#ifdef _WIN32
#include "civ5/ui/bnw_audio_saved_volume_numeric.hpp"
#include <bit>
#include <iostream>
#include <type_traits>
#include "bnw_native_completed_resource_bank_capability.hpp"

namespace civ5::ui::retained_audio_knob_tests {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void refuses(F&& f,const char* why) {
    bool caught=false;try{f();}catch(const std::exception&){caught=true;}
    need(caught,why);
}
static_assert(!std::is_default_constructible_v<BnwAudioLiveKnobOwner>);
static_assert(!std::is_default_constructible_v<BnwAudioCompletedResourceOwner>);
static_assert(!std::is_copy_constructible_v<BnwAudioCompletedResourceOwner>);
static_assert(!std::is_default_constructible_v<detail::BnwNativeCompletedResourceBankCapability>);
static_assert(!std::is_default_constructible_v<BnwAudioWindowActivationLoan>);
static_assert(!std::is_default_constructible_v<BnwAudioWindowActivationLoan::SourceHandle>);

// This positive integration scenario is invoked INSIDE the real bound
// WndProc's first activation callback, after authentic EARLY resource/settings
// producers exist. No snapshots, literal XML or caller success callback can
// manufacture these privately constructed owner/loan inputs. AUTHORED UNRUN.
std::shared_ptr<BnwAudioLiveKnobOwner> first_actual_source(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog,
    std::shared_ptr<const BnwAudioUserSettingsInput> settings,
    std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    need(loan&&loan->sequence()==1,"Fixture lacks actual first window callback");
    const bool minimized=loan->native_minimized_word()!=0;
    const auto values=settings->inputs();
    auto owner=BnwAudioLiveKnobOwner::from_first_actual_activation(packages,catalog,settings,loan);
    for(const auto& value:values){const auto row=owner->knob(value.knob_name);
        need(row.current_bits==std::bit_cast<std::uint32_t>(value.numeric_value)&&
             row.target_bits==0x3f800000u&&row.slope_bits==0&&row.cached_effective_bits==0x3f800000u&&
             row.transition_mode==0&&!row.inherited_flag&&row.observed_window_sequence==1,
             "Actual source percentage/default-record fields changed");}
    const auto master=owner->knob("MASTER");
    need(master.current_bits==(minimized?0u:0x3f800000u)&&master.target_bits==0x3f800000u&&
         master.cached_effective_bits==0x3f800000u&&master.parent_index==-1,
         "Actual first observation MASTER source order changed");
    const auto actual=owner->script_category_factor("AS2D_INTERFACE_NEW_ERA");
    need(actual.category=="GAME_SFX"&&actual.observed_window_sequence==1,
         "Actual NewEra SoundType chain changed");
    const auto factor=minimized?0.0f:values[1].numeric_value;
    need(std::bit_cast<std::uint32_t>(actual.category_factor)==std::bit_cast<std::uint32_t>(factor),
         "Actual retained settings/category parent factor changed");
    return owner;
}
// AUTHORED UNRUN: each invocation requires genuine producer owners and the
// actual first activation loan. Forged aliases must refuse before observers.
template<class T> std::shared_ptr<const T> forged_noop(const std::shared_ptr<const T>& actual) {
    return std::shared_ptr<const T>(actual.get(), [](const T*) {});
}
void refuse_forged_factory_ownership(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog,
    std::shared_ptr<const BnwAudioUserSettingsInput> settings,
    std::shared_ptr<const BnwNativeUserDirectoryOwner> directory,
    std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    const auto resources=catalog->resources(); std::size_t callbacks=0;
    assets::ScopedResourceResolveObserver observer(*resources,
        [&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){++callbacks;});
    refuses([&]{(void)BnwAudioLiveKnobOwner::from_first_actual_activation(
        forged_noop(packages),catalog,settings,loan);},"Forged package control block accepted");
    refuses([&]{(void)BnwAudioLiveKnobOwner::from_first_actual_activation(
        packages,forged_noop(catalog),settings,loan);},"Forged catalog control block accepted");
    refuses([&]{(void)BnwAudioLiveKnobOwner::from_first_actual_activation(
        packages,catalog,forged_noop(settings),loan);},"Forged settings control block accepted");
    refuses([&]{(void)BnwAudioUserSettingsInput::from_existing_native_directory(
        forged_noop(packages),directory,catalog);},"Settings accepted forged package ownership");
    refuses([&]{(void)BnwAudioUserSettingsInput::from_existing_native_directory(
        packages,forged_noop(directory),catalog);},"Settings accepted forged directory ownership");
    refuses([&]{(void)BnwAudioUserSettingsInput::from_existing_native_directory(
        packages,directory,forged_noop(catalog));},"Settings accepted forged catalog ownership");
    refuses([&]{(void)BnwNativeUserDirectoryOwner::create(forged_noop(packages));},
        "Directory accepted forged package ownership");
    need(callbacks==0,"Forged factory ownership reached resource callbacks");
}
// Releasing caller references during a genuine catalog callback must not
// destroy the factory-authenticated inputs already retained by the operation.
void release_callers_during_factory(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog,
    std::shared_ptr<const BnwAudioUserSettingsInput> settings,
    std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    const auto resources=catalog->resources(); const auto weak_settings=std::weak_ptr(settings);
    bool fired=false;
    assets::ScopedResourceResolveObserver observer(*resources,
        [&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
            if(!fired){fired=true;settings.reset();catalog.reset();packages.reset();
                need(!weak_settings.expired(),"Actual settings factory owner was not pinned before callback");}
        });
    auto owner=BnwAudioLiveKnobOwner::from_first_actual_activation(packages,catalog,settings,loan);
    need(fired&&!settings&&!catalog&&!packages&&!weak_settings.expired(),
         "Actual release callback did not exercise retained owner lifetime");
    (void)owner->knob("USER_VOLUME_SFX");
    owner.reset();need(weak_settings.expired(),"Canonical weak factory pin formed a Settings owner cycle");
}
// Called by the real WndProc fixture for the next actual callback. Missing or
// repeated observations must throw; this function supplies no authority.
void next_actual_source(const std::shared_ptr<BnwAudioLiveKnobOwner>& owner,
                        std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    const auto sequence=loan->sequence();const auto minimized=loan->native_minimized_word()!=0;
    owner->consume_actual_activation(loan);const auto current=owner->knob("MASTER");
    need(current.observed_window_sequence==sequence&&current.transition_mode==0,
         "Actual window sequence was not consumed");
    need(current.current_bits==(minimized?0u:0x3f800000u),
         "Actual repeated binary minimize source transition changed");
}
// Resource callbacks can drop the actual facade during a borrowed method and
// mutate caller storage. The held operation must retire before later access.
void retire_during_actual_source(std::shared_ptr<BnwAudioLiveKnobOwner> owner,
                                std::shared_ptr<const BnwAudioSourceCatalog> catalog) {
    need(owner&&owner.use_count()==1,"Retirement fixture must move the sole actual facade owner");
    const auto resources=catalog->resources();auto* borrowed=owner.get();bool armed=true;
    std::string name="USER_VOLUME_SFX";
    assets::ScopedResourceResolveObserver observer(*resources,
        [&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
            if(armed){armed=false;name=std::string(4096,'X');owner.reset();}
        });
    refuses([&]{(void)borrowed->knob(name);},"Dropped facade did not retire held operation");
    need(!owner&&!armed&&name.size()==4096,"Fixture callback did not actually drop facade");
}
// AUTHORED UNRUN: supplied capability must come from the actual completed startup.
void completed_capability_owner_and_query_barrier(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    need(bool(cap),"Completed fixture lacks actual private-factory capability");
    refuses([&]{(void)BnwAudioCompletedResourceOwner::from_completed_bank(forged_noop(cap));},
            "Completed audio accepted no-op capability control block");
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(cap);
    const std::weak_ptr weak_cap=cap;cap.reset();
    owner->require_current();need(!weak_cap.expired(),"Audio holder dropped canonical producer storage");
    bool invoked=false;
    refuses([&]{owner->with_resource_query_loan([&](const assets::ResourceMountTable&){invoked=true;});},
            "Producer63 missing actual native query silently succeeded");
    need(!invoked,"Missing native query invoked a fabricated table callback");
    refuses([&]{owner->require_current();},"Failed query holder was not sticky retired");
}
// AUTHORED UNRUN: table-loan publication alone must never enable generic
// filesystem XML on the strict path, even after producer query support arrives.
void completed_catalog_native_read_remains_unavailable(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(cap);
    const std::array roots{BnwAudioXmlRoot{BnwAudioXmlKind::parameters,"Assets/Sounds/XML/AudioParams.xml"}};
    bool rejected=false;
    try{(void)BnwAudioSourceCatalog::from_completed_resources(owner,roots);}
    catch(const std::runtime_error& e){
        rejected=std::string_view(e.what())=="Completed audio XML requires genuine native SelectedRead/FileReader70 bounded bytes loan";
    }
    need(rejected,"Completed XML used generic filesystem read or wrong native barrier");
    owner->require_current(); // Refused XML does not substitute/retire genuine owner identity.
}
void completed_refuses_diagnostic_catalog(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap,
    std::shared_ptr<const BnwAudioSourceCatalog> diagnostic) {
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(cap);
    refuses([&]{diagnostic->require_same_completed_resources(*owner);},
            "Raw table equality promoted diagnostic catalog to completed source");
    refuses([&]{(void)BnwAudioUserSettingsInput::from_completed_resources(owner,diagnostic);},
            "Diagnostic catalog promoted to completed settings");
}
// The fixture supplies two separately executed full-startup receivers. This
// assertion cannot admit either source: the catalog must already be genuine.
void completed_refuses_another_actual_source(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> other_cap,
    std::shared_ptr<const BnwAudioSourceCatalog> actual_catalog) {
    auto other=BnwAudioCompletedResourceOwner::from_completed_bank(other_cap);
    refuses([&]{actual_catalog->require_same_completed_resources(*other);},
            "Completed catalog accepted another actual source/table owner");
}
struct CompletedCallableProbe {
    std::shared_ptr<const BnwAudioCompletedResourceOwner> owner;
    bool armed=false,copy_case=false,fired=false,invoked=false;
};
struct CompletedCallable {
    std::shared_ptr<CompletedCallableProbe> probe;bool copied=false;
    explicit CompletedCallable(std::shared_ptr<CompletedCallableProbe> p):probe(std::move(p)){}
    CompletedCallable(const CompletedCallable& other):probe(other.probe),copied(true) {
        if(probe->armed&&probe->copy_case){probe->armed=false;probe->fired=true;
            refuses([&]{probe->owner->require_current();},"Callable COPY reentry accepted");}
    }
    ~CompletedCallable(){if(copied&&probe->armed&&!probe->copy_case){
        probe->armed=false;probe->fired=true;
        refuses([&]{probe->owner->require_current();},"Callable destruction reentry accepted");}}
    void operator()(const assets::ResourceMountTable&)const{probe->invoked=true;}
};
// COPY reentry executes before the unresolved native query. Destruction runs
// while unwinding its explicit barrier; both must poison this audio owner.
void completed_callable_reentry(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap,bool copy_case) {
    auto probe=std::make_shared<CompletedCallableProbe>();
    probe->owner=BnwAudioCompletedResourceOwner::from_completed_bank(cap);probe->copy_case=copy_case;
    const std::function<void(const assets::ResourceMountTable&)> input=CompletedCallable(probe);
    probe->armed=true;
    refuses([&]{probe->owner->with_resource_query_loan(input);},"Callable reentry/query barrier skipped");
    need(probe->fired&&!probe->invoked,"Actual callable COPY/destruction boundary not exercised");
    refuses([&]{probe->owner->require_current();},"Callable reentry did not remain retired");
}
// Execute the genuine producer's real COPY/invoke/destruction input-epoch
// callback, releasing the caller capability during that actual callback. No
// invented file observer or supplied success acknowledgement is required.
void completed_drop_caller_during_actual_callback(
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    const std::weak_ptr weak_cap=cap;
    std::shared_ptr<const BnwAudioCompletedResourceOwner> owner;bool fired=false;
    cap->with_actual_pinned_input_epoch([&](const auto& actual_epoch){
        need(bool(actual_epoch),"Genuine completed callback lacks pinned input epoch");
        owner=BnwAudioCompletedResourceOwner::from_completed_bank(cap);
        cap.reset();fired=true;owner->require_current();
        need(!weak_cap.expired(),"Canonical storage expired inside actual producer callback");
    });
    need(fired&&!cap&&owner&&!weak_cap.expired(),"Actual callback release was not retained after producer return");
    owner->require_current();
}
// After actual Category65 queries exist, invoke under the real table observer.
// Source65 is not replaced by a caller-created table or successful acknowledgement.
void completed_actual_query_reentry(
    const std::shared_ptr<const BnwAudioCompletedResourceOwner>& owner) {
    bool entered=false;
    refuses([&]{owner->with_resource_query_loan([&](const assets::ResourceMountTable&){
        entered=true;refuses([&]{owner->require_current();},"Completed holder reentry accepted");
    });},"Caught reentry permitted completed query publication");
    if(entered)refuses([&]{owner->require_current();},"Completed holder reentry not sticky");
}
// AUTHORED UNRUN: genuine completed Parent+capability; actual selected read
// may still refuse a MISS. No local-file hash supplies winning-source authority.
void first_actual_native_parameters(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap);
    const std::array roots{BnwAudioXmlRoot{BnwAudioXmlKind::parameters,"Assets/Sounds/XML/AudioParams.xml"}};
    auto catalog=BnwAudioSourceCatalog::from_completed_resources(owner,roots);
    const auto before=catalog->native_parameters();catalog->require_current();const auto after=catalog->native_parameters();
    need(before.basename=="AudioParams.xml"&&before.sha256.size()==64&&before.sha256==after.sha256&&
         before.sound_types.name=="SoundTypes"&&!before.sound_types.children.empty(),"Actual native parameters snapshot/currentness mismatch");
    refuses([&]{(void)catalog->layers(BnwAudioXmlKind::parameters);},"Native parameters invented UiAudioSource provenance");
    refuses([&]{(void)catalog->resources();},"Native parameters leaked borrowed table");
    refuses([&]{(void)BnwAudioUserSettingsInput::from_completed_resources(owner,catalog);},"Native parameters granted completed Directory/settings");
}
void refuse_native_parent_or_cap_alias(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    refuses([&]{(void)BnwAudioCompletedResourceOwner::from_completed_bank(forged_noop(parent),cap);},"Native factory accepted no-op Parent");
    refuses([&]{(void)BnwAudioCompletedResourceOwner::from_completed_bank(parent,forged_noop(cap));},"Native factory accepted no-op cap");
}
void refuse_native_wrong_actual_parent(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> other_parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    refuses([&]{(void)BnwAudioCompletedResourceOwner::from_completed_bank(other_parent,cap);},"Native factory accepted another actual Parent");
}
void native_actual_callback_release(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap);
    const std::weak_ptr weak_parent=parent;const std::weak_ptr weak_cap=cap;bool fired=false;
    owner->with_native_parameters_bytes([&](std::span<const std::uint8_t> bytes){
        fired=true;parent.reset();cap.reset();need(!bytes.empty()&&!weak_parent.expired()&&!weak_cap.expired(),"Native callback lost actual canonical owners");
    });need(fired&&!parent&&!cap&&!weak_parent.expired()&&!weak_cap.expired(),"Native callback release not exercised/retained after return");
    owner->require_current();
}
struct NativeCallableProbe {std::shared_ptr<const BnwAudioCompletedResourceOwner> owner;bool armed=false,copy_case=false,release=false,fired=false,invoked=false;};
struct NativeCallable {
    std::shared_ptr<NativeCallableProbe> probe;bool copied=false;
    explicit NativeCallable(std::shared_ptr<NativeCallableProbe> p):probe(std::move(p)){}
    void fire()const{probe->armed=false;probe->fired=true;if(probe->release)probe->owner.reset();
        else refuses([&]{probe->owner->require_current();},"Native byte callable reentry accepted");}
    NativeCallable(const NativeCallable& other):probe(other.probe),copied(true){if(probe->armed&&probe->copy_case)fire();}
    ~NativeCallable(){if(copied&&probe->armed&&!probe->copy_case)fire();}
    void operator()(std::span<const std::uint8_t>)const{probe->invoked=true;}
};
// copy_case/release select four deterministic genuine-owner lifetime cases,
// not producer success flags. Destructor case needs a real selected payload.
void native_callable_lifetime(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap,bool copy_case,bool release) {
    auto probe=std::make_shared<NativeCallableProbe>();probe->owner=BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap);
    probe->copy_case=copy_case;probe->release=release;
    const auto weak=std::weak_ptr(probe->owner);const auto* borrowed=probe->owner.get();
    const std::function<void(std::span<const std::uint8_t>)> input=NativeCallable(probe);probe->armed=true;
    refuses([&]{borrowed->with_native_parameters_bytes(input);},"Native callable release/reentry permitted publication");
    need(probe->fired,"Native callable actual COPY/destruction not exercised");
    if(!copy_case)need(probe->invoked,"Native destructor coverage requires actual selected byte callback; MISS is unsupported");
    if(release)need(!probe->owner&&weak.expired(),"Native callable sole facade not released or owner cycle");
    else refuses([&]{probe->owner->require_current();},"Native callable reentry not sticky retired");
}
void native_callback_invoke_reentry(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    auto owner=BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap);std::size_t count=0;
    refuses([&]{owner->with_native_parameters_bytes([&](std::span<const std::uint8_t>){++count;
        refuses([&]{owner->require_current();},"Native invoke reentry accepted");});},"Caught native invoke reentry published");
    need(count==1,"Native byte consumer not exactly once");
    refuses([&]{owner->require_current();},"Native invoke reentry not sticky");
}
void native_foreign_first_admission(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent,
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap) {
    bool refused=false;
    std::thread foreign([parent,cap,&refused]{try{(void)BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap);}
        catch(const std::exception&){refused=true;}});foreign.join();
    need(refused,"Foreign first native audio admission accepted");
    (void)BnwAudioCompletedResourceOwner::from_completed_bank(parent,cap); // Creator thread still valid.
}
void changed_actual_settings(const std::shared_ptr<BnwAudioLiveKnobOwner>& owner) {
    // Fixture must change the actual retained UserSettings bytes before call.
    // The owner's actual raw reader/hash checks, not a supplied boolean, refuse.
    refuses([&]{owner->require_current();},"Changed actual settings not refused");
    refuses([&]{(void)owner->knob("MASTER");},"Rejected settings owner not sticky retired");
}
}
int main(){
    try{
        civ5::ui::retained_audio_knob_tests::refuses([]{
            (void)civ5::ui::BnwAudioLiveKnobOwner::from_first_actual_activation({}, {}, {}, {});
        },"Null owners/loan accepted");
        civ5::ui::retained_audio_knob_tests::refuses([]{(void)civ5::ui::BnwAudioCompletedResourceOwner::from_completed_bank({});},"Null completed capability accepted");
        civ5::ui::retained_audio_knob_tests::refuses([]{(void)civ5::ui::BnwAudioSourceCatalog::from_completed_resources({},{});},"Null completed catalog owners accepted");
        civ5::ui::retained_audio_knob_tests::refuses([]{(void)civ5::ui::BnwAudioUserSettingsInput::from_completed_resources({},{});},"Null completed settings owners accepted");
        // Positive scenarios above require the real producer fixture and are
        // not claimed by this negative-only standalone invocation.
        std::cout<<"negative retained-owner construction checks only; genuine producer fixture unrun\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
#else
int main(){return 77;}
#endif
