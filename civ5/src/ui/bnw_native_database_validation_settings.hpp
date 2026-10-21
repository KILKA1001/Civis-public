#pragma once
#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <charconv>
#include <optional>
#include <string_view>
#include <vector>
namespace civ5::ui::database_validation_detail {
struct Fields {bool enable_tuner=false,validate_database=false;bool tuner_defaulted=true,validation_defaulted=true;};
// Closed source parser subset; this pure result is not a source capability.
inline Fields source_fields(std::string_view text) {
    const auto need=[](bool b,const char* s){if(!b)throw std::invalid_argument(s);};
    need(text.size()<=1024*1024&&text.find('\0')==text.npos,"Config INI extent/NUL unsupported");
    for(unsigned char c:text)need(c=='\r'||c=='\n'||c=='\t'||(c>=32&&c<127),"Config INI non-ASCII source branch unsupported");
    const auto trim=[](std::string_view s){auto a=s.find_first_not_of(" \t\r");return a==s.npos?std::string_view{}:s.substr(a,s.find_last_not_of(" \t\r")-a+1);};
    const auto equal=[](std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(std::size_t k=0;k<a.size();++k){auto lower=[](unsigned char c){return c>='A'&&c<='Z'?c+32:c;};if(lower(a[k])!=lower(b[k]))return false;}return true;};
    Fields out;bool selected=false,seen_debugging=false;
    for(std::size_t p=0;p<text.size();) {
        auto end=text.find('\n',p);auto line=trim(text.substr(p,end==text.npos?text.npos:end-p));p=end==text.npos?text.size():end+1;
        if(line.empty()||line.front()==';')continue;
        if(line.front()=='['){need(line.size()>2&&line.back()==']',"Unsupported Config INI section syntax");auto s=line.substr(1,line.size()-2);need(s.find_first_of("[]")==s.npos&&trim(s)==s,"Unsupported Config section bracket/spacing branch");selected=equal(s,"Debugging")&&!seen_debugging;if(equal(s,"Debugging"))seen_debugging=true;continue;}
        auto eq=line.find('=');need(eq!=line.npos&&eq!=0,"Unsupported Config INI entry syntax");auto key=trim(line.substr(0,eq)),value=trim(line.substr(eq+1));need(!key.empty()&&key.find_first_of("[];")==key.npos,"Unsupported Config INI key syntax");
        if(!selected)continue;
        bool* target=nullptr;bool* defaulted=nullptr;
        if(equal(key,"EnableTuner")&&out.tuner_defaulted){target=&out.enable_tuner;defaulted=&out.tuner_defaulted;}
        if(equal(key,"ValidateGameDatabase")&&out.validation_defaulted){target=&out.validate_database;defaulted=&out.validation_defaulted;}
        if(!target)continue;
        // Native8274E1 uses atoi. Admit only a finite signed-decimal subset,
        // never pretend overflow/partial CRT conversion has been ported.
        need(!value.empty(),"Config selected integer empty branch unsupported");if(value.front()=='+'){value.remove_prefix(1);need(!value.empty()&&value.front()>='0'&&value.front()<='9',"Config selected plus-sign branch unsupported");}need(!value.empty(),"Config selected integer sign only");
        std::int32_t n=0;auto r=std::from_chars(value.data(),value.data()+value.size(),n);need(r.ec==std::errc{}&&r.ptr==value.data()+value.size(),"Config selected atoi branch unsupported");*target=n!=0;*defaulted=false;
    }
    return out;
}
}
namespace civ5::ui {
struct BnwNativeDatabaseValidationSettingsFrame {
    std::filesystem::path source_path;
    runtime::FileIdentity loaded_file;
    bool source_file_present=false;
    bool native_load_routing_owned=false;
    std::string next_source_dependency="SAME native prefix/CRC namespace Config Load routing";
    std::optional<bool> enable_tuner_cached,validation_cached;
    bool os_candidate_enable_tuner=false,os_candidate_validation=false;
    bool enable_tuner_defaulted=false,validation_defaulted=false;
    std::optional<bool> constructor_auto_save,source_dictionary_dirty;
    std::uint64_t load_generation=0;
};
// Stages bounded fresh Config source inputs. Native Load routing is currently
// pending; current() cannot publish a cached predicate. Root must
// separately authenticate enclosing normal startup/GC order and logs.
class BnwNativeDatabaseValidationSettings final {
  public:
    static std::shared_ptr<const BnwNativeDatabaseValidationSettings> acquire(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const BnwNativeUserDirectoryOwner>);
    ~BnwNativeDatabaseValidationSettings();
    BnwNativeDatabaseValidationSettings(const BnwNativeDatabaseValidationSettings&)=delete;
    BnwNativeDatabaseValidationSettings& operator=(const BnwNativeDatabaseValidationSettings&)=delete;
    // Read-only actual OS input diagnostic; grants no native existence/cache.
    const BnwNativeDatabaseValidationSettingsFrame& input_snapshot()const;
    const BnwNativeDatabaseValidationSettingsFrame& current()const;
    void require_same_resources(const BnwNativePackageDiscoveryOwner&)const;
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativeDatabaseValidationSettings(std::unique_ptr<Impl>);
};
}
