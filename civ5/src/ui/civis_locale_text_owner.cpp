#include "civ5/ui/civis_locale_text_owner.hpp"
#include <cstring>
#include <limits>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace civ5::ui {
namespace {
#ifdef _WIN32
std::wstring wide(std::string_view text){
    if(text.find('\0')!=text.npos||text.size()>static_cast<std::size_t>(INT_MAX))
        throw runtime::ScriptError("Civis locale text contains NUL or exceeds the Windows string API");
    if(text.empty())return {};
    const auto size=static_cast<int>(text.size());const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),size,nullptr,0);
    if(count<=0)throw runtime::ScriptError("Civis locale text is invalid UTF-8");
    std::wstring result(static_cast<std::size_t>(count),L'\0');
    if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),size,result.data(),count)!=count)
        throw runtime::ScriptError("Civis locale UTF-8 conversion failed");
    return result;
}
std::string utf8(const std::wstring &text){
    if(text.empty())return {};
    const auto count=static_cast<int>(text.size());const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),count,nullptr,0,nullptr,nullptr);
    if(size<=0)throw runtime::ScriptError("Civis locale text result is invalid UTF-16");
    std::string result(static_cast<std::size_t>(size),'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),count,result.data(),size,nullptr,nullptr)!=size)
        throw runtime::ScriptError("Civis locale UTF-16 conversion failed");
    return result;
}
#endif
}
struct CivisLocaleTextOwner::Impl {
    std::weak_ptr<SelectedCivilopediaLocale> locale;
    CivilopediaLayoutOwnerKey identity;
    CivisLocaleTextPolicy policy;
    std::thread::id thread=std::this_thread::get_id();bool closed=false;
#ifdef _WIN32
    std::wstring windows_locale;NLSVERSIONINFOEX version{};
#endif
    void require_current()const{
        const auto owner=locale.lock();
        if(std::this_thread::get_id()!=thread||closed||!owner||!owner->current(identity)||owner->provider().current().type!=policy.language)
            throw runtime::ScriptError("Civis locale text owner is closed, stale, expired, or on another thread");
#ifdef _WIN32
        NLSVERSIONINFOEX current{};current.dwNLSVersionInfoSize=sizeof(current);
        if(!GetNLSVersionEx(COMPARE_STRING,windows_locale.c_str(),&current)||std::memcmp(&current,&version,sizeof(version))!=0)
            throw runtime::ScriptError("Civis selected Windows NLS policy changed or is unavailable");
#else
        throw runtime::ScriptError("Civis Windows NLS text backend is unavailable");
#endif
    }
    std::string lower(std::string_view text)const{
        require_current();
#ifdef _WIN32
        const auto input=wide(text);if(input.empty())return {};
        const auto count=static_cast<int>(input.size());
        const auto length=LCMapStringEx(windows_locale.c_str(),policy.casing_flags,input.data(),count,nullptr,0,nullptr,nullptr,0);
        if(length<=0)throw runtime::ScriptError("Civis selected linguistic lowercase mapping failed");
        std::wstring mapped(static_cast<std::size_t>(length),L'\0');
        if(LCMapStringEx(windows_locale.c_str(),policy.casing_flags,input.data(),count,mapped.data(),length,nullptr,nullptr,0)!=length)
            throw runtime::ScriptError("Civis selected linguistic lowercase mapping changed during conversion");
        return utf8(mapped);
#else
        (void)text;throw runtime::ScriptError("Civis Windows NLS text backend is unavailable");
#endif
    }
    std::int64_t compare(std::string_view a,std::string_view b)const{
        require_current();
#ifdef _WIN32
        const auto left=wide(a),right=wide(b);
        const auto result=CompareStringEx(windows_locale.c_str(),policy.comparison_flags,left.data(),static_cast<int>(left.size()),
            right.data(),static_cast<int>(right.size()),nullptr,nullptr,0);
        if(result==CSTR_LESS_THAN)return -1;
        if(result==CSTR_EQUAL)return 0;
        if(result==CSTR_GREATER_THAN)return 1;
        throw runtime::ScriptError("Civis selected word collation failed");
#else
        (void)a;(void)b;throw runtime::ScriptError("Civis Windows NLS text backend is unavailable");
#endif
    }
};
CivisLocaleTextOwner::CivisLocaleTextOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
CivisLocaleTextOwner::~CivisLocaleTextOwner()=default;
std::shared_ptr<CivisLocaleTextOwner> CivisLocaleTextOwner::open(std::shared_ptr<SelectedCivilopediaLocale> locale,
    CivilopediaLayoutOwnerKey identity,std::string language){
    if(!locale||!locale->current(identity)||(language!="en_US"&&language!="ru_RU")||locale->provider().current().type!=language)
        throw runtime::ScriptError("Civis locale text requires the matching selected EN/RU language owner");
    auto impl=std::make_unique<Impl>();impl->locale=locale;impl->identity=std::move(identity);
    impl->policy.backend="Windows NLS linguistic lowercase / word collation";impl->policy.language=std::move(language);
#ifdef _WIN32
    impl->policy.locale_name=impl->policy.language=="en_US"?"en-US":"ru-RU";
    impl->windows_locale=impl->policy.language=="en_US"?L"en-US":L"ru-RU";
    impl->policy.casing_flags=LCMAP_LOWERCASE|LCMAP_LINGUISTIC_CASING;impl->policy.comparison_flags=0;
    impl->version.dwNLSVersionInfoSize=sizeof(impl->version);
    if(!IsValidLocaleName(impl->windows_locale.c_str())||!GetNLSVersionEx(COMPARE_STRING,impl->windows_locale.c_str(),&impl->version))
        throw runtime::ScriptError("Civis selected Windows NLS locale is unavailable");
    impl->policy.nls_version=impl->version.dwNLSVersion;impl->policy.defined_version=impl->version.dwDefinedVersion;
#else
    throw runtime::ScriptError("Civis Windows NLS text backend is unavailable");
#endif
    return std::shared_ptr<CivisLocaleTextOwner>(new CivisLocaleTextOwner(std::move(impl)));
}
runtime::ScriptLocaleTextProvider CivisLocaleTextOwner::provider(){
    impl_->require_current();const std::weak_ptr<CivisLocaleTextOwner> weak=shared_from_this();
    const auto lock=[weak]{auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civis locale text owner expired");return owner;};
    return {[lock](auto text){return lock()->impl_->lower(text);},[lock](auto a,auto b){return lock()->impl_->compare(a,b);}};
}
const CivisLocaleTextPolicy &CivisLocaleTextOwner::policy()const{return impl_->policy;}
void CivisLocaleTextOwner::close(){
    if(std::this_thread::get_id()!=impl_->thread)throw runtime::ScriptError("Civis locale text close requires owner thread");impl_->closed=true;
}
} // namespace civ5::ui
