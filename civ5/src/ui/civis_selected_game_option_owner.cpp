#include "civ5/ui/civis_selected_game_option_owner.hpp"
#include <limits>
#include <thread>

namespace civ5::ui {
namespace {
constexpr std::string_view option_key="GAMEOPTION_NO_RELIGION";
constexpr std::int64_t sdk_token=21; // CvEnums.h1063–1088, not an Info ID.
std::string_view c_string(std::string_view name){return name.substr(0,name.find('\0'));}
bool hash(std::string_view value){
    return value.size()==64&&value.find_first_not_of("0123456789abcdefABCDEF")==value.npos;
}
void require_native_i32(std::int64_t value){
    if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())
        throw runtime::ScriptError("Civis selected option source value exceeds the verified signed32 contract");
}
}
struct CivisSelectedGameOptionOwner::Impl {
    CivilopediaLayoutOwnerKey identity;
    std::weak_ptr<CivisCivilopediaCallbackOwner> owner;
    std::vector<saves::PregameOption> raw_options;
    CivisSelectedGameOptionResolution resolved;
    const std::thread::id thread=std::this_thread::get_id();
    bool closed=false;
    void require_current()const{
        if(std::this_thread::get_id()!=thread)throw runtime::ScriptError("Civis selected option requires its owner thread");
        if(closed)throw runtime::ScriptError("Civis selected option owner is closed");
        const auto current=owner.lock();
        if(!current||!current->current(identity))throw runtime::ScriptError("Civis selected option context owner expired or became stale");
    }
    bool is_option(std::int64_t token)const{
        require_current();
        if(token!=sdk_token)throw runtime::ScriptError("Civis selected option token is outside the declared NO_RELIGION capability");
        return resolved.value==1; // CvGame.cpp7159–7173.
    }
};
CivisSelectedGameOptionOwner::CivisSelectedGameOptionOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
CivisSelectedGameOptionOwner::~CivisSelectedGameOptionOwner()=default;
std::shared_ptr<CivisSelectedGameOptionOwner> CivisSelectedGameOptionOwner::open(
    const saves::PregameObservation &raw,const content::RulesDatabase &rules,
    CivilopediaLayoutOwnerKey identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner){
    if(raw.layout!=saves::GameStateLayout::expansion2_version1||!owner||!owner->current(identity)||
       identity.owner_token.empty()||!hash(identity.payload_sha256)||!hash(identity.rules_sha256))
        throw runtime::ScriptError("Civis selected option requires selected BNW Pregame and matching source/context identity");
    auto impl=std::make_unique<Impl>();impl->identity=std::move(identity);impl->owner=owner;
    impl->raw_options=raw.game_options;impl->resolved.rules_sha256=impl->identity.rules_sha256;
    for(const auto &option:impl->raw_options){
        // CustomOption loads a C string into its 64-byte name array. Reject
        // invalid native input explicitly rather than emulate CRT aborts.
        if(c_string(option.name).size()>=64)throw runtime::ScriptError("Civis selected option name exceeds verified native64 storage");
        require_native_i32(option.value);
    }
    for(std::size_t i=0;i<impl->raw_options.size();++i){
        const auto &option=impl->raw_options[i];
        // CvPreGame.cpp934–935 uses strlen(savedName) in strncmp: an empty
        // or shorter prefix matches too. Ordered first match is retained.
        if(option_key.starts_with(c_string(option.name))){
            impl->resolved.value=option.value;impl->resolved.origin=CivisGameOptionResolutionOrigin::saved_prefix;
            impl->resolved.saved_ordinal=i;break;
        }
    }
    if(!impl->resolved.saved_ordinal){
        bool found=false;
        const auto report=rules.query_rows({"SELECT \"Default\" FROM GameOptions WHERE Type=? LIMIT 1",{std::string(option_key)}},
            [&](const auto &row){
                const auto value=row.integer_at(0);require_native_i32(value);
                impl->resolved.value=value;impl->resolved.origin=CivisGameOptionResolutionOrigin::selected_default;found=true;
            },1);
        if(!report.all_succeeded())throw runtime::ScriptError("Civis selected option rules lookup failed: "+report.error);
        // SyncGameOptionsWithEnumList initializes value0 before GetGameOption.
        // A genuinely absent default leaves that source-defined zero intact.
        if(!found)impl->resolved.origin=CivisGameOptionResolutionOrigin::missing_default_zero;
    }
    impl->require_current();
    return std::shared_ptr<CivisSelectedGameOptionOwner>(new CivisSelectedGameOptionOwner(std::move(impl)));
}
runtime::ScriptSelectedGameOptionProvider CivisSelectedGameOptionOwner::provider(){
    impl_->require_current();const std::weak_ptr<CivisSelectedGameOptionOwner> weak=shared_from_this();
    return {sdk_token,[weak](auto token){
        const auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civis selected option owner expired");
        return owner->impl_->is_option(token);
    }};
}
CivisSelectedGameOptionResolution CivisSelectedGameOptionOwner::resolution()const{impl_->require_current();return impl_->resolved;}
void CivisSelectedGameOptionOwner::close(){
    if(std::this_thread::get_id()!=impl_->thread)throw runtime::ScriptError("Civis selected option close requires owner thread");
    impl_->closed=true;
}
const std::array<CivisGameOptionSourcePin,5> &CivisSelectedGameOptionOwner::source_pins(){
    static constexpr std::array<CivisGameOptionSourcePin,5> pins{{
        {"CvGameCoreSource/CvGameCoreDLLUtil/include/CvEnums.h","6781f34a6a6a9fac1cbaad3a3477afabf540669af9f860f57ad54b967c22da9b"},
        {"CvGameCoreSource/CvGameCoreDLL_Expansion2/Lua/CvLuaEnums.cpp","f0ac65c6beba33c4cf027e1544d5499d20a33f230bd7f81fe79c8846076ac0d0"},
        {"CvGameCoreSource/CvGameCoreDLL_Expansion2/Lua/CvLuaGame.cpp","6591ccbb83d4d863b6fb2e556f5d420dfeeb50a5f3eb4fb81978bb75e7ae9f2f"},
        {"CvGameCoreSource/CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"},
        {"CvGameCoreSource/CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"}
    }};
    return pins;
}
}
