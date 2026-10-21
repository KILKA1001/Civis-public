#include "civ5/runtime/bnw_fresh_advisors.hpp"
#include <thread>
#include <utility>

namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwFreshAdvisorError code,const char* message) {
    throw BnwFreshAdvisorRefusal(code,message);
}
void same_thread(std::thread::id thread) {
    if(thread!=std::this_thread::get_id())
        refuse(BnwFreshAdvisorError::wrong_thread,"Advisor allocation belongs to another thread");
}
std::shared_ptr<const BnwCreationConfig> current_config(std::shared_ptr<const BnwCreationConfig> config) {
    if(!config) refuse(BnwFreshAdvisorError::parent_unavailable,"Advisor lost Game config parent");
    config->require_current();return config;
}
}
BnwFreshAdvisorRefusal::BnwFreshAdvisorRefusal(BnwFreshAdvisorError code,const char* message)
    :std::runtime_error(message),code_(code) {}

struct detail::BnwAdvisorCounselStorage::Impl {
    struct Counsel { std::int32_t advisor; char text[512]; std::int32_t value; };
    const std::thread::id thread=std::this_thread::get_id();
    // No value initialization: native FFastVector<POD>::resize neither calls
    // element constructors nor initializes the 512 text bytes.
    std::unique_ptr<Counsel[]> rows{new Counsel[50]};
    std::size_t count=50;
    std::array<BnwAdvisorWrite,100> writes;
    Impl() {
        for(std::size_t i=0;i<count;++i) {
            rows[i].advisor=-1;
            writes[2*i]={BnwAdvisorWriteField::counsel_advisor,static_cast<std::uint32_t>(i),-1};
            rows[i].value=-1;
            writes[2*i+1]={BnwAdvisorWriteField::counsel_value,static_cast<std::uint32_t>(i),-1};
        }
    }
    void current() const { same_thread(thread); }
};
detail::BnwAdvisorCounselStorage::BnwAdvisorCounselStorage():impl_(std::make_unique<Impl>()) {}
detail::BnwAdvisorCounselStorage::~BnwAdvisorCounselStorage()=default;
std::size_t detail::BnwAdvisorCounselStorage::size() const {impl_->current();return impl_->count;}
BnwAdvisorCounselRow detail::BnwAdvisorCounselStorage::row(std::size_t i) const {
    impl_->current();
    if(i>=impl_->count) refuse(BnwFreshAdvisorError::index_unavailable,"Counsel row outside native logical size");
    return {impl_->rows[i].advisor,impl_->rows[i].value};
}
char detail::BnwAdvisorCounselStorage::text_byte(std::size_t row,std::size_t byte) const {
    impl_->current();
    if(row>=impl_->count || byte>=512)
        refuse(BnwFreshAdvisorError::index_unavailable,"Counsel text outside native extent");
    refuse(BnwFreshAdvisorError::field_unwritten,"Counsel constructor/Reset never writes text");
}
std::span<const BnwAdvisorWrite> detail::BnwAdvisorCounselStorage::constructor_writes() const {
    impl_->current();return impl_->writes;
}
void detail::BnwAdvisorCounselStorage::reset_detached() {
    impl_->current();
    for(std::size_t i=0;i<impl_->count;++i) {
        impl_->rows[i].advisor=-1;impl_->rows[i].value=-1;
    }
}
void detail::BnwAdvisorCounselStorage::uninit_detached() {
    impl_->current();impl_->count=0; // Native clear keeps the allocation.
}

struct detail::BnwAdvisorRecommenderStorage::Impl {
    // Recommended member array constructors precede the four weighted members.
    struct Buildable {
        std::int32_t type,index,turns;
        Buildable():type(0),index(0),turns(0) {}
    };
    struct WeightedInt { std::int32_t element,weight; };
    struct RawBuildable { std::int32_t type,index,turns; };
    struct WeightedBuild { RawBuildable element; std::int32_t weight; };
    const std::thread::id thread=std::this_thread::get_id();
    std::array<Buildable,4> builds;
    std::array<std::int32_t,4> techs; // Native member remains unwritten until ResetTechs.
    // FStaticVector allocates these capacities, logical size zero, without
    // constructing POD WeightedElement entries. No unused slot is read.
    std::unique_ptr<WeightedInt[]> researchable{new WeightedInt[80]};
    std::unique_ptr<WeightedInt[]> final_techs{new WeightedInt[4]};
    std::unique_ptr<WeightedBuild[]> city_buildables{new WeightedBuild[50]};
    std::unique_ptr<WeightedBuild[]> final_buildables{new WeightedBuild[4]};
    std::array<std::uint32_t,4> sizes{0,0,0,0};
    std::array<BnwAdvisorWrite,28> writes;
    Impl() {
        std::size_t w=0;
        for(std::size_t i=0;i<builds.size();++i) {
            const auto slot=static_cast<std::uint32_t>(i);
            writes[w++]={BnwAdvisorWriteField::member_type,slot,builds[i].type};
            writes[w++]={BnwAdvisorWriteField::member_index,slot,builds[i].index};
            writes[w++]={BnwAdvisorWriteField::member_turns,slot,builds[i].turns};
        }
        // CvAdvisorRecommender ctor -> Init -> Reset -> ResetCity first.
        for(std::size_t i=0;i<builds.size();++i) {
            const auto slot=static_cast<std::uint32_t>(i);
            builds[i].type=0;writes[w++]={BnwAdvisorWriteField::reset_type,slot,0};
            builds[i].index=-1;writes[w++]={BnwAdvisorWriteField::reset_index,slot,-1};
            builds[i].turns=-1;writes[w++]={BnwAdvisorWriteField::reset_turns,slot,-1};
        }
        sizes[2]=0;sizes[3]=0; // City then final-round buildables clear.
        for(std::size_t i=0;i<techs.size();++i) {
            techs[i]=-1;writes[w++]={BnwAdvisorWriteField::reset_tech,static_cast<std::uint32_t>(i),-1};
        }
        sizes[0]=0;sizes[1]=0; // Researchable then final-round techs clear.
    }
    void current() const {same_thread(thread);}
};
detail::BnwAdvisorRecommenderStorage::BnwAdvisorRecommenderStorage():impl_(std::make_unique<Impl>()) {}
detail::BnwAdvisorRecommenderStorage::~BnwAdvisorRecommenderStorage()=default;
BnwAdvisorRecommenderSnapshot detail::BnwAdvisorRecommenderStorage::observe() const {
    impl_->current();BnwAdvisorRecommenderSnapshot out;
    for(std::size_t i=0;i<impl_->builds.size();++i)
        out.recommended_builds[i]={impl_->builds[i].type,impl_->builds[i].index,impl_->builds[i].turns};
    out.recommended_techs=impl_->techs;out.weighted_sizes=impl_->sizes;return out;
}
std::span<const BnwAdvisorWrite> detail::BnwAdvisorRecommenderStorage::constructor_writes() const {
    impl_->current();return impl_->writes;
}
void detail::BnwAdvisorRecommenderStorage::reset_detached() {
    impl_->current();
    for(auto& build:impl_->builds) {build.type=0;build.index=-1;build.turns=-1;}
    impl_->sizes[2]=0;impl_->sizes[3]=0;
    for(auto& tech:impl_->techs) tech=-1;
    impl_->sizes[0]=0;impl_->sizes[1]=0;
}
void detail::BnwAdvisorRecommenderStorage::uninit_detached() {impl_->current();}

struct BnwFreshAdvisorCounsel::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    detail::BnwAdvisorCounselStorage storage;
    explicit Impl(std::shared_ptr<const BnwCreationConfig> c):config(current_config(std::move(c))) {}
    void current() const {same_thread(thread);config->require_current();}
};
BnwFreshAdvisorCounsel::BnwFreshAdvisorCounsel(std::shared_ptr<const BnwCreationConfig> c)
    :impl_(std::make_unique<Impl>(std::move(c))) {}
BnwFreshAdvisorCounsel::~BnwFreshAdvisorCounsel()=default;
void BnwFreshAdvisorCounsel::require_current() const {impl_->current();}
std::size_t BnwFreshAdvisorCounsel::size() const {require_current();return impl_->storage.size();}
BnwAdvisorCounselRow BnwFreshAdvisorCounsel::row(std::size_t i) const {require_current();return impl_->storage.row(i);}
char BnwFreshAdvisorCounsel::text_byte(std::size_t i,std::size_t byte) const {
    require_current();return impl_->storage.text_byte(i,byte);
}
struct BnwFreshAdvisorRecommender::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    detail::BnwAdvisorRecommenderStorage storage;
    explicit Impl(std::shared_ptr<const BnwCreationConfig> c):config(current_config(std::move(c))) {}
    void current() const {same_thread(thread);config->require_current();}
};
BnwFreshAdvisorRecommender::BnwFreshAdvisorRecommender(std::shared_ptr<const BnwCreationConfig> c)
    :impl_(std::make_unique<Impl>(std::move(c))) {}
BnwFreshAdvisorRecommender::~BnwFreshAdvisorRecommender()=default;
void BnwFreshAdvisorRecommender::require_current() const {impl_->current();}
BnwAdvisorRecommenderSnapshot BnwFreshAdvisorRecommender::observe() const {
    require_current();return impl_->storage.observe();
}
}
