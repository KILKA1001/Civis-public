#include "civ5/ui/bnw_audio_live_knob_owner.hpp"
#ifdef _WIN32
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "bnw_audio_knob_state_detail.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>

namespace civ5::ui {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
const content::XmlNode& only_child(const content::XmlNode& node,std::string_view name) {
    const content::XmlNode* found=nullptr;
    for(const auto& c:node.children)if(c.name==name){need(!found,"Live audio duplicate required XML field");found=&c;}
    need(found,"Live audio required authored XML field missing");return *found;
}
}
struct BnwAudioLiveKnobOwner::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> retired{false};bool busy=false;
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages;
    std::shared_ptr<const BnwAudioSourceCatalog> catalog;
    std::shared_ptr<const BnwAudioUserSettingsInput> settings;
    std::shared_ptr<const assets::ResourceMountTable> resources;
    const BnwAudioWindowActivationLoan::SourceHandle window;
    std::unique_ptr<audio_settings_detail::KnobNumericGraph> graph;
    std::uint64_t processed_sequence=0;
    bool cached_minimized=false; // Explicit new Civis owner initialization.
    explicit Impl(BnwAudioWindowActivationLoan::SourceHandle source):window(std::move(source)){}
    void check()const{need(!retired.load(),"Live audio knob owner retired");}
    struct Call {
        std::shared_ptr<Impl> p;
        explicit Call(std::shared_ptr<Impl> value):p(std::move(value)){
            need(p&&p->thread==std::this_thread::get_id(),"Live audio knob owner foreign thread");p->check();
            if(p->busy){p->retired.store(true);throw std::runtime_error("Live audio knob owner reentry");}
            p->busy=true;
        }
        ~Call(){p->busy=false;}
    };
    void validate(std::uint64_t expected)const {
        check();window.require_current();check();
        need(expected>0&&window.observed_sequence()==expected,"Live audio unconsumed actual window observation");
        const auto early=packages->early_input_lease();check();need(bool(early),"Live audio requires actual early package client");
        early->require_current();check();
        need(early->actual_hwnd()==reinterpret_cast<std::uintptr_t>(window.window()),
             "Live audio substituted actual package window");check();
        catalog->require_current();check();
        const auto current_resources=catalog->resources();check();
        const auto* package_resources=&packages->resources();check();
        need(current_resources.get()==resources.get()&&package_resources==resources.get(),
             "Live audio substituted resource allocation/epoch");
        settings->require_same_catalog(*catalog);check();
        window.require_current();check();
        need(window.observed_sequence()==expected,"Live audio window changed during source callbacks");
    }
    void apply_master(bool observed) {
        // Native84c370..84c37c compares current byte92 with old cached4108.
        // Native84c43d..84c457 writes oldCached?1f:0f and mode0, THEN
        // 84c45e..84c46a caches current observed byte. No inverse inference.
        if(observed!=cached_minimized)graph->direct("MASTER",cached_minimized?1.0f:0.0f);
        cached_minimized=observed;
    }
};
BnwAudioLiveKnobOwner::BnwAudioLiveKnobOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioLiveKnobOwner::~BnwAudioLiveKnobOwner(){impl_->retired.store(true);}
std::shared_ptr<BnwAudioLiveKnobOwner> BnwAudioLiveKnobOwner::from_first_actual_activation(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog,
    std::shared_ptr<const BnwAudioUserSettingsInput> settings,
    std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    need(packages&&catalog&&settings&&loan,"Live audio requires genuine retained source owners");
    // Lock and compare real factory control blocks before ANY window or
    // resource validation. A no-op shared_ptr with the same get() is refused.
    // Actual73 callback-free canonical helper; compare caller ownership
    // BEFORE any currentness/resource callback or dependency storage.
    const auto canonical_packages=packages->retain_factory_owner();
    need(canonical_packages&&canonical_packages.get()==packages.get()&&
         !canonical_packages.owner_before(packages)&&!packages.owner_before(canonical_packages),
         "Audio supplied forged package factory ownership");
    packages=canonical_packages;
    catalog=catalog->retain_factory_owner(catalog);
    settings=settings->retain_factory_owner(settings);
    loan->require_current();need(loan->sequence()==1,"Live audio first activation history missing");
    auto p=std::make_shared<Impl>(loan->source_handle());Impl::Call call(p);
    p->packages=std::move(packages);p->catalog=std::move(catalog);p->settings=std::move(settings);
    try {
        p->resources=p->catalog->resources();p->check();p->validate(1);loan->require_current();
        const auto layers=p->catalog->layers(BnwAudioXmlKind::parameters);p->check();p->validate(1);loan->require_current();
        need(layers.size()==1&&layers[0].source.has_value(),"Live audio requires one captured actual AudioParams layer");
        const auto& types=only_child(layers[0].document,"SoundTypes");
        p->graph=std::make_unique<audio_settings_detail::KnobNumericGraph>(types);
        const auto values=p->settings->inputs();p->check();p->validate(1);loan->require_current();
        constexpr std::array<std::string_view,4> keys{"Music.Volume","Effects.Volume","Ambience.Volume","Speech.Volume"};
        constexpr std::array<std::string_view,4> knobs{"USER_VOLUME_MUSIC","USER_VOLUME_SFX","USER_VOLUME_AMBIENCE","USER_VOLUME_SPEECH"};
        std::array<std::int32_t,4> raw{};
        for(std::size_t i=0;i<4;++i){
            need(values[i].preference_key==keys[i]&&values[i].knob_name==knobs[i],"Live audio settings source order changed");
            const auto exact=audio_settings_detail::knob_from_saved_volume_percent(values[i].saved_percent);
            need(std::bit_cast<std::uint32_t>(exact)==std::bit_cast<std::uint32_t>(values[i].numeric_value),
                 "Live audio settings numeric input changed");raw[i]=values[i].saved_percent;
        }
        p->graph->load_four_saved_percentages(raw);
        p->validate(1);loan->require_current();
        p->apply_master(loan->native_minimized_word()!=0);p->processed_sequence=1;
        p->validate(1);loan->require_current();
        // Keep local operation storage through any control-block allocation
        // failure; its catch path must not dereference a moved-from pointer.
        return std::shared_ptr<BnwAudioLiveKnobOwner>(new BnwAudioLiveKnobOwner(p));
    }catch(...){p->retired.store(true);throw;}
}
std::shared_ptr<BnwAudioLiveKnobOwner> BnwAudioLiveKnobOwner::from_completed_resources(
    std::shared_ptr<const BnwAudioCompletedResourceOwner> owner,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog,
    std::shared_ptr<const BnwAudioUserSettingsInput> settings,
    std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    need(owner&&catalog&&settings&&loan,"Completed live audio requires genuine source owners");
    owner=owner->retain_factory_owner(owner);catalog=catalog->retain_factory_owner(catalog);
    settings=settings->retain_factory_owner(settings);
    catalog->require_same_completed_resources(*owner);
    // No early HWND/extent loan is promoted to a completed activation/device grant.
    throw std::runtime_error("Completed live audio requires actual completed settings/activation binding and initialized device/MASTER source");
}
void BnwAudioLiveKnobOwner::consume_actual_activation(std::shared_ptr<const BnwAudioWindowActivationLoan> loan) {
    const auto p=impl_;Impl::Call call(p);
    try {
        need(bool(loan),"Live audio actual activation missing");loan->require_current();p->check();
        loan->source_handle().require_same_source(p->window);p->check();
        need(p->processed_sequence<std::numeric_limits<std::uint64_t>::max()&&
             loan->sequence()==p->processed_sequence+1,"Live audio activation missing, stale or repeated");
        const auto sequence=loan->sequence();p->validate(sequence);loan->require_current();
        const bool observed=loan->native_minimized_word()!=0;
        // No caller callback after preflight can authorize these concrete stores.
        p->apply_master(observed);p->processed_sequence=sequence;
        p->validate(sequence);loan->require_current();
    }catch(...){p->retired.store(true);throw;}
}
void BnwAudioLiveKnobOwner::require_current()const {
    const auto p=impl_;Impl::Call call(p);
    try{p->validate(p->processed_sequence);}catch(...){p->retired.store(true);throw;}
}
BnwAudioLiveKnobFields BnwAudioLiveKnobOwner::knob(std::string_view borrowed)const {
    const auto p=impl_;Impl::Call call(p);const std::string owned(borrowed);
    try {
        p->validate(p->processed_sequence);const auto fields=p->graph->fields(owned);
        BnwAudioLiveKnobFields result{owned,std::bit_cast<std::uint32_t>(fields.current),
            std::bit_cast<std::uint32_t>(fields.target),std::bit_cast<std::uint32_t>(fields.slope),
            std::bit_cast<std::uint32_t>(fields.cached_effective),fields.transition_mode,
            fields.parent_index,fields.inherited_flag,p->processed_sequence};
        p->validate(p->processed_sequence);return result;
    }catch(...){p->retired.store(true);throw;}
}
BnwAudioScriptCategoryFactor BnwAudioLiveKnobOwner::script_category_factor(std::string_view borrowed)const {
    const auto p=impl_;Impl::Call call(p);const std::string owned(borrowed);
    try {
        p->validate(p->processed_sequence);const auto rows=p->catalog->rows(BnwAudioXmlKind::scripts_2d,owned);
        p->check();p->validate(p->processed_sequence);
        need(rows.size()==1,"Live audio actual Script2D source missing or ambiguous");
        const auto& type=only_child(rows[0].row,"SoundType");
        need(type.children.empty()&&type.attributes.empty()&&!type.text.empty(),
             "Live audio declared SoundType required; default branch unsupported");
        BnwAudioScriptCategoryFactor result{owned,type.text,p->graph->category_effective(type.text),p->processed_sequence};
        p->validate(p->processed_sequence);return result;
    }catch(...){p->retired.store(true);throw;}
}
}
#endif
