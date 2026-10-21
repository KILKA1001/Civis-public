#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

namespace civ5::ui::detail {
class BnwNativeColdFilesystemOwner;
// New fresh FS subobject lifetime, born before Default. Its closure is the
// Civis mapping bootstrap under readonly-Original optional-shortcut policy,
// not Native shortcut/Save execution. It cannot import an existing list or
// issue Default/late33/resource/window completion.
class BnwNativeFilesystemMappingOwner final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Parent=::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Input=::civ5::ui::BnwNativeInstallationInputOwner;
    struct Roots final {
        std::array<std::u16string,2> raw; // Actual initial empty string owners.
        std::array<std::filesystem::path,2> physical;
        std::array<bool,2> initialized{false,false};
    };
    struct Descriptor final {
        std::uint32_t root=0;
        std::u16string name,shortcut,cached;
    };
    enum class Phase {born,module,app,root0,root1,config,logs,saves,hook,physical,returned,bound};
    // Civis readonly-Original profile resolves only the optional convenience
    // shortcut role. This is not an HRESULT, native false, or Save success.
    enum class ShortcutSite {general,config};
    enum class ShortcutDisposition {policy_denied_readonly_original};
    enum class OriginalShortcutResult {not_invoked};
    struct OptionalShortcutResolution final {
        const ShortcutSite site;
        const std::size_t descriptor_index,occurrence;
        const std::u16string alias,target,destination;
        const ShortcutDisposition disposition;
        const OriginalShortcutResult original_result;
        OptionalShortcutResolution(ShortcutSite actual_site,std::size_t index,std::size_t actual_occurrence,
            const Descriptor& actual,const std::filesystem::path& original_module_base)
            :site(actual_site),descriptor_index(index),occurrence(actual_occurrence),alias(actual.shortcut),
             target(actual.cached),destination(u16((original_module_base/(wide(actual.shortcut)+L".lnk")).native())),
             disposition(ShortcutDisposition::policy_denied_readonly_original),
             original_result(OriginalShortcutResult::not_invoked){}
    };
    const DWORD process_=GetCurrentProcessId(),thread_=GetCurrentThreadId();
    const std::weak_ptr<const Input> input_;
    const std::filesystem::path original_root_,canonical_payload_;
    const std::shared_ptr<Roots> roots_=std::shared_ptr<Roots>(new Roots);
    std::vector<Descriptor> config_,general_; // Two real vector constructors; no prefilled rows.
    std::u16string app_name_,config_name_;
    std::string module_ansi_,module_parent_ansi_;
    std::filesystem::path logical_cwd_;
    Phase phase_=Phase::born;
    mutable bool failed_=false,checking_=false;
    bool active_=false,mapping_body_returned_=false,mapping_bootstrap_returned_=false,bound_=false,cold_bound_=false;
    class BootstrapOperation;
    const BootstrapOperation* active_bootstrap_=nullptr;
    bool module_returned_=false,app_returned_=false,hook_returned_=false,config_fixup_returned_=false;
    unsigned seed_returns_=0,general_physical_returns_=0,config_physical_returns_=0;
    bool config_existence_observed_=false,config_existed_=false;
    std::vector<OptionalShortcutResolution> optional_shortcut_resolutions_;
    std::size_t optional_shortcut_returns_=0;
    std::weak_ptr<const Parent> parent_;
    const Parent* parent_identity_=nullptr;
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold_;
    static void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
    void creator()const {need(process_==GetCurrentProcessId()&&thread_==GetCurrentThreadId(),"FS bootstrap foreign process/creator");}
    void fail()const noexcept {if(process_==GetCurrentProcessId()&&thread_==GetCurrentThreadId())failed_=true;}
    class BootstrapOperation final {
        friend class BnwNativeFilesystemMappingOwner;
        BnwNativeFilesystemMappingOwner& owner_;
        explicit BootstrapOperation(BnwNativeFilesystemMappingOwner& owner):owner_(owner){
            owner_.creator();
            need(!owner_.failed_&&!owner_.active_&&!owner_.active_bootstrap_&&!owner_.bound_&&
                 !owner_.mapping_body_returned_&&owner_.phase_==Phase::born,"FS bootstrap actual operation replay/reentry");
            owner_.active_=true;owner_.active_bootstrap_=this;
        }
        ~BootstrapOperation()noexcept {
            if(owner_.active_bootstrap_!=this)owner_.fail();
            owner_.active_bootstrap_=nullptr;owner_.active_=false;
        }
        BootstrapOperation(const BootstrapOperation&)=delete;
        BootstrapOperation& operator=(const BootstrapOperation&)=delete;
    };
    static std::u16string u16(const std::wstring& value){return {value.begin(),value.end()};}
    static std::wstring wide(const std::u16string& value){return {value.begin(),value.end()};}
    static std::string utf8(const std::filesystem::path& value){const auto data=value.u8string();return {data.begin(),data.end()};}
    static std::u16string normalized(std::u16string value){
        std::replace(value.begin(),value.end(),u'/',u'\\');
        while(!value.empty()&&value.back()==u'\\')value.pop_back();return value;
    }
    static std::wstring shell_base(int id){
        wchar_t value[MAX_PATH]{};
        // Qualified existing-only branch. Never invoke Shell fCreate or mkdir.
        need(SHGetSpecialFolderPathW(nullptr,value,id,FALSE)!=FALSE,"FS bootstrap existing Shell root unavailable");
        const auto attrs=GetFileAttributesW(value);
        need(attrs!=INVALID_FILE_ATTRIBUTES&&(attrs&FILE_ATTRIBUTE_DIRECTORY),"FS bootstrap Shell directory missing/non-directory");
        return value;
    }
    std::u16string emitted_root(unsigned slot)const {
        need(slot<2&&!app_name_.empty(),"FS bootstrap root lacks actual app-name producer");
        auto root=normalized(u16(shell_base(slot==0?CSIDL_LOCAL_APPDATA:CSIDL_PERSONAL)));
        root+=u"\\My Games\\";root+=app_name_;return root;
    }
    static void require_existing_directory(const std::filesystem::path& path){
        const auto attrs=GetFileAttributesW(path.c_str());
        if(attrs==INVALID_FILE_ATTRIBUTES||!(attrs&FILE_ATTRIBUTE_DIRECTORY))
            throw std::runtime_error("FS bootstrap required directory create/type branch is unowned: "+utf8(path));
        // Actual848390 existing-path branch performs no CreateDirectoryW.
    }
    // Explicit bootstrap-only outputs. No arbitrary path or general permission
    // API: each site derives from its genuine Shell/app/descriptor producer.
    enum class DirectorySite {root0,root1,logs,saves};
    struct DirectoryAttempt {
        std::filesystem::path destination;
        BOOL result=FALSE;
        std::optional<DWORD> error; // Only a genuinely failed CreateDirectoryW has an error observation.
    };
    struct DirectoryEffect {
        DirectorySite site;
        std::filesystem::path destination;
        bool initially_existing=false,returned=false;
        std::vector<DirectoryAttempt> attempts;
    };
    std::vector<DirectoryEffect> directory_effects_;
    class DirectoryPin final {
        const std::filesystem::path path_;
        HANDLE handle_=INVALID_HANDLE_VALUE;
        BY_HANDLE_FILE_INFORMATION identity_{};
        static bool same_directory(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
            return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&
                a.nFileIndexLow==b.nFileIndexLow;
        }
      public:
        explicit DirectoryPin(std::filesystem::path path):path_(std::move(path)){
            handle_=CreateFileW(path_.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            if(handle_==INVALID_HANDLE_VALUE){const auto error=GetLastError();
                throw std::runtime_error("FS directory source pin failed: "+utf8(path_)+" error="+std::to_string(error));}
            try{
                need(GetFileInformationByHandle(handle_,&identity_)!=FALSE&&
                     (identity_.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&!(identity_.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),
                     "FS bootstrap creation requires actual non-reparse directory ancestors");check();
            }catch(...){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;throw;}
        }
        ~DirectoryPin(){if(handle_!=INVALID_HANDLE_VALUE)CloseHandle(handle_);}
        DirectoryPin(const DirectoryPin&)=delete;
        void check()const {
            BY_HANDLE_FILE_INFORMATION held{},named{};
            need(handle_!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(handle_,&held)!=FALSE&&
                 same_directory(held,identity_)&&(held.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&
                 !(held.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),"FS bootstrap pinned directory identity/type changed");
            const auto reopened=CreateFileW(path_.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            need(reopened!=INVALID_HANDLE_VALUE,"FS bootstrap pinned directory name disappeared");
            const bool read=GetFileInformationByHandle(reopened,&named)!=FALSE;CloseHandle(reopened);
            need(read&&same_directory(named,identity_)&&(named.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&
                 !(named.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),"FS bootstrap directory name no longer identifies its pinned source");
        }
    };
    static bool at_or_under(const std::filesystem::path& path,const std::filesystem::path& base){
        if(path.empty()||base.empty()||!path.is_absolute()||!base.is_absolute())return false;
        const auto actual=path.lexically_normal(),root=base.lexically_normal();
        auto left=actual.begin();
        for(auto right=root.begin();right!=root.end();++right){if(right->empty())continue;
            while(left!=actual.end()&&left->empty())++left;
            if(left==actual.end())return false;
            const auto& a=left->native();const auto& b=right->native();
            const auto compared=CompareStringOrdinal(a.data(),static_cast<int>(a.size()),b.data(),static_cast<int>(b.size()),TRUE);
            need(compared!=0,"FS directory path identity comparison failed");
            if(compared!=CSTR_EQUAL)return false;++left;
        }
        return true;
    }
    static void reject_original_directory_output(const std::filesystem::path& path,const std::filesystem::path& original,
        const std::filesystem::path& payload){
        const auto physical=std::filesystem::weakly_canonical(path);
        need(!at_or_under(physical,original)&&!at_or_under(physical,payload),
             "FS bootstrap user-directory output would enter Original/canonical assets");
    }
    void ensure_bootstrap_directory(DirectorySite site,const std::u16string* actual_root_emission=nullptr){
        const bool root=site==DirectorySite::root0||site==DirectorySite::root1;
        const unsigned slot=site==DirectorySite::root0?0u:1u;
        if(root)require_body(slot==0?Phase::app:Phase::root0);else require_body(Phase::physical);
        need(site==DirectorySite::root0||site==DirectorySite::root1||site==DirectorySite::logs||site==DirectorySite::saves,
             "FS bootstrap unknown directory source site");
        for(const auto& prior:directory_effects_)need(prior.site!=site,"FS bootstrap directory source site replay");
        const auto base=std::filesystem::path(shell_base(slot==0?CSIDL_LOCAL_APPDATA:CSIDL_PERSONAL)).lexically_normal();
        const auto app=wide(normalized(app_name_));need(!app.empty(),"FS directory issuer lacks actual application namespace");
        auto destination=base/L"My Games"/app;
        if(!root){const auto index=general_physical_returns_;
            need(index<general_.size()&&general_[index].root==1&&roots_->initialized[1],
                 "FS directory issuer is outside actual general cursor");
            const auto& row=general_[index];const auto* name=site==DirectorySite::logs?u"Logs":u"Saves";
            need(row.name==name&&row.cached==roots_->raw[1]+row.name+u"\\",
                 "FS directory issuer substituted actual Logs/Saves descriptor");destination/=wide(row.name);
        }
        destination=destination.lexically_normal();
        need(base.is_absolute()&&destination.is_absolute()&&at_or_under(destination,base),
             "FS directory output is outside its actual Shell base");
        if(root)need(actual_root_emission&&destination==std::filesystem::path(wide(normalized(*actual_root_emission))).lexically_normal(),
             "FS directory issuer changed the SAME actual root emission");
        source_current();if(root)need(emitted_root(slot)==*actual_root_emission,"FS Shell source changed before root directory effect");
        DirectoryEffect effect{site,destination};
        const auto attrs=GetFileAttributesW(destination.c_str());const auto missing_error=attrs==INVALID_FILE_ATTRIBUTES?GetLastError():ERROR_SUCCESS;
        if(attrs!=INVALID_FILE_ATTRIBUTES){
            need((attrs&FILE_ATTRIBUTE_DIRECTORY)!=0,"FS user namespace output collides with an existing non-directory");
            effect.initially_existing=true;require_existing_directory(destination);source_current();
        }else{
            need(missing_error==ERROR_FILE_NOT_FOUND||missing_error==ERROR_PATH_NOT_FOUND,
                 "FS bootstrap directory attributes failure is not a qualified missing path");
            const auto drive=base.root_name().native();
            need(drive.size()==2&&drive[1]==L':'&&((drive[0]>=L'A'&&drive[0]<=L'Z')||(drive[0]>=L'a'&&drive[0]<=L'z')),
                 "FS bootstrap new-directory branch requires actual local drive source");
            reject_original_directory_output(destination,original_root_,canonical_payload_);
            {
                // Read-only pins of the existing path chain deny rename/write
                // sharing during creation. No ancestor above Shell base is made.
                std::vector<std::unique_ptr<DirectoryPin>> pins;
                auto current=base.root_path();pins.push_back(std::make_unique<DirectoryPin>(current));
                for(const auto& part:base.relative_path())if(!part.empty()){
                    current/=part;pins.push_back(std::make_unique<DirectoryPin>(current));}
                const auto check=[&]{source_current();if(root)need(emitted_root(slot)==*actual_root_emission,"FS actual root emission changed during directory effect");
                    for(const auto& pin:pins)pin->check();
                    need(std::filesystem::weakly_canonical(base)==base,"FS bootstrap Shell-base creation route changed/reparsed");
                    reject_original_directory_output(destination,original_root_,canonical_payload_);};
                check();const auto suffix=destination.lexically_relative(base);
                current=base;
                for(const auto& part:suffix)if(!part.empty()){
                    need(part!=L"."&&part!=L"..","FS bootstrap owned directory suffix changed");current/=part;check();
                    const auto before=GetFileAttributesW(current.c_str());
                    if(before==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();
                        need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                             "FS bootstrap child probe is not a qualified missing path");
                        const auto api_path=current.native()+L"\\";
                        DirectoryAttempt observed;observed.destination=std::filesystem::path(api_path);
                        observed.result=CreateDirectoryW(api_path.c_str(),nullptr);
                        if(observed.result==FALSE)observed.error=GetLastError();
                        effect.attempts.push_back(observed);
                        if(observed.result==FALSE&&observed.error!=ERROR_ALREADY_EXISTS)
                            throw std::runtime_error("FS CreateDirectoryW failed: "+utf8(current)+" error="+std::to_string(*observed.error));
                    }else need((before&FILE_ATTRIBUTE_DIRECTORY)&&!(before&FILE_ATTRIBUTE_REPARSE_POINT),
                         "FS bootstrap directory chain contains non-directory/reparse source");
                    // Even true API return or183 does not grant existence/type.
                    // The actual named object must now pass a genuine pin/POST.
                    pins.push_back(std::make_unique<DirectoryPin>(current));check();
                }
                check();require_existing_directory(destination);
            } // All real directory handles/locals retire without source callbacks.
            source_current();require_existing_directory(destination);
        }
        source_current();if(root)need(emitted_root(slot)==*actual_root_emission,"FS actual root emission changed at directory POST");effect.returned=true;
        directory_effects_.push_back(std::move(effect));source_current();
    }
    void ensure_bootstrap_root_directory(unsigned slot,const std::u16string& actual_emission){
        need(slot<2,"FS bootstrap root directory selector changed");
        ensure_bootstrap_directory(slot==0?DirectorySite::root0:DirectorySite::root1,&actual_emission);
    }
    void ensure_bootstrap_general_directory(std::size_t index){
        require_body(Phase::physical);need(index==general_physical_returns_&&index<general_.size(),
             "FS bootstrap directory is not actual current general descriptor");
        const auto& name=general_[index].name;
        if(name==u"Logs")ensure_bootstrap_directory(DirectorySite::logs);
        else if(name==u"Saves")ensure_bootstrap_directory(DirectorySite::saves);
        else throw std::runtime_error("FS bootstrap directory source is outside actual seed sites");
    }

    static std::string acp_module(const std::wstring& source){
        need(!source.empty()&&source.size()<MAX_PATH&&GetACP()==1251,"FS bootstrap module exceeds qualified ANSI/MAX_PATH source");
        BOOL substituted=FALSE;
        const int size=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,source.data(),static_cast<int>(source.size()),nullptr,0,nullptr,&substituted);
        need(size>0&&size<MAX_PATH&&!substituted,"FS bootstrap module lacks exact source ACP1251 representation");
        std::string value(size,'\0');substituted=FALSE;
        need(WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,source.data(),static_cast<int>(source.size()),value.data(),size,nullptr,&substituted)==size&&!substituted,
             "FS bootstrap module ANSI conversion changed");
        std::wstring back(source.size(),L'\0');
        need(MultiByteToWideChar(CP_ACP,0,value.data(),size,back.data(),static_cast<int>(back.size()))==static_cast<int>(source.size())&&back==source,
             "FS bootstrap module ANSI roundtrip changed");return value;
    }
    void source_current()const {
        creator();need(!failed_,"FS bootstrap source failed");
        if(checking_){fail();throw std::runtime_error("FS bootstrap source/provider reentry");}
        checking_=true;struct Exit{const BnwNativeFilesystemMappingOwner& owner;~Exit(){owner.checking_=false;}} exit{*this};
        try {
            if(bound_)need(parent_identity_&&!parent_.expired(),"FS bootstrap canonical Parent retired");
            if(cold_bound_)need(!cold_.expired(),"FS bootstrap SAME Cold source retired");
            const auto input=input_.lock();need(bool(input),"FS bootstrap actual installation Input retired");
            input->require_current();
            input->require_cold_roots_identity_without_callbacks(original_root_,canonical_payload_);
            need(input->game_root()==original_root_&&input->canonical_payload_root()==canonical_payload_,"FS bootstrap actual Input module roots changed");
            if(module_returned_)need(acp_module((input->game_root()/L"CivilizationV.exe").native())==module_ansi_&&
                 logical_cwd_==input->game_root(),"FS bootstrap logical Original module/CWD source changed");
            for(unsigned slot=0;slot<2;++slot)if(roots_->initialized[slot]){
                need(emitted_root(slot)==roots_->raw[slot],"FS bootstrap paired Shell/app root changed");
                need(std::filesystem::weakly_canonical(std::filesystem::path(wide(roots_->raw[slot])))==roots_->physical[slot],
                     "FS bootstrap paired root canonical source changed");
                require_existing_directory(roots_->physical[slot]);
            }
            for(unsigned n=0;n<general_physical_returns_;++n)require_existing_directory(std::filesystem::path(wide(general_[n].cached)));
            if(config_existence_observed_){const auto attrs=GetFileAttributesW(wide(config_[0].cached).c_str());
                need((attrs!=INVALID_FILE_ATTRIBUTES)==config_existed_,"FS bootstrap actual Config existence source changed during POST");}
            input->require_current();need(!failed_&&(!bound_||!parent_.expired()),"FS bootstrap source retired during provider POST");
        }catch(...){fail();throw;}
    }
    explicit BnwNativeFilesystemMappingOwner(const std::shared_ptr<const Input>& actual)
        :input_(actual),original_root_(actual->game_root()),canonical_payload_(actual->canonical_payload_root()){}
    // Defined beside the complete Input::Impl; pointer/control checks precede providers.
    static std::shared_ptr<const Input> actual_input_control(const std::shared_ptr<const Input>&);
    void require_body(Phase phase)const {
        creator();need(!failed_&&active_&&active_bootstrap_ && &active_bootstrap_->owner_==this&&
             !bound_&&!mapping_bootstrap_returned_&&phase_==phase,"FS bootstrap actual operation/site changed");source_current();
    }
    void module_source(){
        require_body(Phase::born);const auto input=input_.lock();need(bool(input),"FS bootstrap module Input absent");
        const auto actual=(input->game_root()/L"CivilizationV.exe").native();
        module_ansi_=acp_module(actual);
        const auto slash=module_ansi_.find_last_of('\\');
        need(slash!=std::string::npos&&slash+1<module_ansi_.size(),"FS bootstrap823980 module split branch unowned");
        module_parent_ansi_=module_ansi_.substr(0,slash+1); // Actual81DAA0 prefix through LAST backslash.
        logical_cwd_=input->game_root();require_existing_directory(logical_cwd_);
        need(std::filesystem::path(actual).parent_path()==logical_cwd_,"FS bootstrap Input/Cold module parent differs");
        // Qualified logical source routing as existing Cold: no GetModuleFileName(Civis), no SetCurrentDirectory.
        module_returned_=true;phase_=Phase::module;require_body(Phase::module);
    }
    void app_source(){
        require_body(Phase::module);need(app_name_.empty(),"FS bootstrap reused app namespace");
        app_name_=normalized(u"Sid Meier's Civilization 5");app_name_+=u'\\';
        app_returned_=true;phase_=Phase::app;require_body(Phase::app);
    }
    void establish_root(unsigned slot){
        const auto before=slot==0?Phase::app:Phase::root0;require_body(before);
        need(slot<2&&!roots_->initialized[slot]&&roots_->raw[slot].empty(),"FS bootstrap root replay/substitution");
        auto actual=emitted_root(slot);source_current();
        const auto physical=std::filesystem::path(wide(actual));
        ensure_bootstrap_root_directory(slot,actual);require_existing_directory(physical);source_current();
        // Fresh833140: both vectors actually exist; root had no previous value.
        need(config_.empty()&&general_.empty(),"FS bootstrap root update needs genuine earlier-record reconciliation");
        roots_->raw[slot]=std::move(actual);roots_->physical[slot]=std::filesystem::weakly_canonical(physical);
        roots_->initialized[slot]=true;phase_=slot==0?Phase::root0:Phase::root1;require_body(phase_);
    }
    void append_seed(bool config,const std::u16string& name,Phase before,Phase after){
        require_body(before);need(roots_->initialized[0]&&roots_->initialized[1]&&!name.empty(),"FS seed lacks actual paired roots/name");
        auto& list=config?config_:general_;const auto old=list.size();
        {Descriptor local;local.root=1;local.name=normalized(name);local.shortcut=local.name;source_current();
            list.push_back(local);source_current();}
        source_current(); // Actual copied descriptor AND local string destruction precede observation.
        need(list.size()==old+1&&list.back().root==1&&list.back().name==normalized(name)&&
             list.back().shortcut==list.back().name&&list.back().cached.empty(),"FS actual832FA0 seed did not return its appended descriptor");
        ++seed_returns_;phase_=after;require_body(after);
    }
    void seed_sources(){
        require_body(Phase::root1);if(config_name_.empty())config_name_=u"Config.ini";source_current();
        append_seed(true,config_name_,Phase::root1,Phase::config);
        append_seed(false,u"Logs",Phase::config,Phase::logs);
        append_seed(false,u"Saves",Phase::logs,Phase::saves);
    }
    static void actual_closed_slot24_noop()noexcept{} // Concrete pinned532F60 C3, not an injected callback.
    std::u16string resolve_general_61d660(std::size_t index){
        require_body(Phase::physical);need(index==general_physical_returns_&&index<general_.size(),"FS61D660 is not actual current bootstrap index");
        auto& row=general_[index];need(row.root<2&&roots_->initialized[row.root],"FS61D660 lost actual selected root");
        if(row.cached.empty()){auto value=roots_->raw[row.root]+row.name;value+=u'\\';source_current();row.cached=std::move(value);source_current();}
        return row.cached;
    }
    void resolve_optional_bootstrap_shortcut(ShortcutSite site,std::size_t index){
        require_body(Phase::physical);
        need(site==ShortcutSite::general||site==ShortcutSite::config,"Civis optional shortcut unknown actual site");
        // Arguments come only from this operation's actual general/Config
        // cursor. No caller name/path/disposition can resolve the role.
        const bool general=site==ShortcutSite::general;
        need((general?index==general_physical_returns_&&index<general_.size():
             index==config_physical_returns_&&index<config_.size()&&config_existence_observed_&&config_existed_),
             "Civis optional shortcut is outside its actual bootstrap call site");
        const auto& row=general?general_[index]:config_[index];
        need(module_returned_&&row.root<2&&roots_->initialized[row.root]&&!row.shortcut.empty()&&
             row.shortcut==row.name&&row.cached==roots_->raw[row.root]+row.name+(general?u"\\":u""),
             "Civis optional shortcut lost actual emitted descriptor target");
        source_current();const auto old=optional_shortcut_resolutions_.size();
        {
            // The policy decision really happens here. No Original/COM call
            // is made, and no Original result is claimed. Its exact prospective
            // alias/target/destination and occurrence remain diagnostics only.
            const OptionalShortcutResolution actual(site,index,old+1,row,logical_cwd_);
            source_current();optional_shortcut_resolutions_.push_back(actual);source_current();
        }
        source_current(); // Actual diagnostic copy/local destruction completed.
        need(optional_shortcut_resolutions_.size()==old+1&&optional_shortcut_returns_==old,
             "Civis optional shortcut decision replay or incomplete return");
        ++optional_shortcut_returns_;source_current();
    }
    void physical_bootstrap(){
        require_body(Phase::hook);phase_=Phase::physical;require_body(Phase::physical);
        for(std::size_t index=0;index<general_.size();++index){
            const auto path=resolve_general_61d660(index);ensure_bootstrap_general_directory(index);
            require_existing_directory(std::filesystem::path(wide(path)));source_current();
            const auto& row=general_[index];
            if(!row.shortcut.empty()){
                const auto target=resolve_general_61d660(index); // Actual repeated getter sees SAME filled cache.
                need(target==row.cached,"Civis repeated actual path getter changed");
                resolve_optional_bootstrap_shortcut(ShortcutSite::general,index);
            }
            ++general_physical_returns_;source_current();
        }
        //832BB0 actual first Config name fixup; no disk Config file is rewritten.
        if(!config_.empty()&&config_.front().name!=config_name_){config_.front().name=config_name_;config_.front().cached.clear();source_current();}
        config_fixup_returned_=true;
        for(std::size_t index=0;index<config_.size();++index){auto& row=config_[index];
            need(row.root<2&&roots_->initialized[row.root],"FS bootstrap Config actual root absent");
            if(!row.shortcut.empty()){
                if(row.cached.empty()){row.cached=roots_->raw[row.root]+row.name;source_current();}
                //Qualified820950 before Default exists equals the actual directOS
                //existence outcome. A genuinely missing Config skips shortcut work.
                const auto attrs=GetFileAttributesW(wide(row.cached).c_str());
                config_existed_=attrs!=INVALID_FILE_ATTRIBUTES;config_existence_observed_=true;source_current();
                if(attrs!=INVALID_FILE_ATTRIBUTES){
                    resolve_optional_bootstrap_shortcut(ShortcutSite::config,index);
                }
            }
            ++config_physical_returns_;source_current();
        }
        require_body(Phase::physical);
    }
    void mapping_bootstrap(){
        BootstrapOperation operation(*this);
        try {
            module_source();app_source();establish_root(0);establish_root(1);seed_sources();
            require_body(Phase::saves);actual_closed_slot24_noop();source_current();hook_returned_=true;phase_=Phase::hook;
            physical_bootstrap();source_current();mapping_body_returned_=true;
        }catch(...){fail();throw;}
    }
    void mapping_bootstrap_identity_without_callbacks()const {
        creator();need(!failed_&&!active_&&!active_bootstrap_&&mapping_body_returned_&&module_returned_&&app_returned_&&hook_returned_&&
             roots_->initialized[0]&&roots_->initialized[1]&&seed_returns_==3&&config_fixup_returned_&&
             config_.size()==1&&general_.size()==2&&general_physical_returns_==general_.size()&&config_physical_returns_==config_.size(),
             "FS bootstrap lacks actual full constructor/seed/cache/physical return");
        need(config_[0].name==config_name_&&config_[0].shortcut==config_name_&&config_[0].root==1&&
             config_[0].cached==roots_->raw[1]+config_name_&&config_existence_observed_&&
             general_[0].name==u"Logs"&&general_[1].name==u"Saves","FS bootstrap authentic earlier descriptor payload changed");
        for(const auto& row:general_)need(row.root==1&&row.shortcut==row.name&&row.cached==roots_->raw[1]+row.name+u"\\",
             "FS bootstrap real general-entry cached path changed");
        need(directory_effects_.size()==4,"Civis mapping bootstrap omitted an actual directory producer site");
        for(std::size_t n=0;n<directory_effects_.size();++n){const auto& effect=directory_effects_[n];
            const auto site=n==0?DirectorySite::root0:n==1?DirectorySite::root1:n==2?DirectorySite::logs:DirectorySite::saves;
            auto expected=std::filesystem::path(wide(normalized(roots_->raw[n==0?0u:1u])));
            if(n>1)expected/=n==2?L"Logs":L"Saves";
            need(effect.site==site&&effect.returned&&effect.destination==expected.lexically_normal()&&
                 (!effect.initially_existing||effect.attempts.empty()),
                 "Civis mapping bootstrap directory site/target/result changed");
            for(const auto& attempt:effect.attempts)need((attempt.result!=FALSE&&!attempt.error)||
                 (attempt.result==FALSE&&attempt.error&&*attempt.error==ERROR_ALREADY_EXISTS),
                 "Civis mapping bootstrap fabricated a directory API result");
        }
        need(optional_shortcut_resolutions_.size()==2u+(config_existed_?1u:0u)&&
             optional_shortcut_returns_==optional_shortcut_resolutions_.size(),
             "Civis mapping bootstrap omitted an actual optional shortcut policy resolution");
        for(std::size_t n=0;n<optional_shortcut_resolutions_.size();++n){const auto& result=optional_shortcut_resolutions_[n];
            const bool general=n<general_.size();const auto index=general?n:std::size_t{0};
            const auto& row=general?general_[index]:config_[index];
            need(result.site==(general?ShortcutSite::general:ShortcutSite::config)&&result.descriptor_index==index&&
                 result.occurrence==n+1&&result.alias==row.shortcut&&result.target==row.cached&&
                 result.destination==u16((logical_cwd_/(wide(row.shortcut)+L".lnk")).native())&&
                 result.disposition==ShortcutDisposition::policy_denied_readonly_original&&
                 result.original_result==OriginalShortcutResult::not_invoked,
                 "Civis mapping bootstrap substituted its actual denied optional-effect diagnostic");
        }
    }
    static std::shared_ptr<BnwNativeFilesystemMappingOwner> create_fresh_installed(const std::shared_ptr<const Input>& actual){
        const auto canonical=actual_input_control(actual);canonical->require_current();
        const auto result=std::shared_ptr<BnwNativeFilesystemMappingOwner>(new BnwNativeFilesystemMappingOwner(canonical));
        result->mapping_bootstrap();result->mapping_bootstrap_identity_without_callbacks();result->source_current();result->mapping_bootstrap_identity_without_callbacks();
        result->phase_=Phase::returned;result->mapping_bootstrap_returned_=true;return result;
    }
    void bind_parent(const std::shared_ptr<const Parent>& actual){
        creator();mapping_bootstrap_identity_without_callbacks();need(actual&&mapping_bootstrap_returned_&&!bound_&&phase_==Phase::returned&&parent_.expired(),"FS bootstrap Parent bind replay/missing real return");
        parent_=actual;parent_identity_=actual.get();bound_=true;phase_=Phase::bound;source_current();mapping_bootstrap_identity_without_callbacks();
    }
    void bind_actual_cold(const std::shared_ptr<const BnwNativeColdFilesystemOwner>& actual){
        creator();mapping_bootstrap_identity_without_callbacks();need(mapping_bootstrap_returned_&&bound_&&!parent_.expired()&&actual&&!cold_bound_&&cold_.expired(),"FS bootstrap SAME Cold bind replay/before actual factory");
        cold_=actual;cold_bound_=true;source_current();mapping_bootstrap_identity_without_callbacks();
    }
    void require_mapping_bootstrap_returned_current()const {
        mapping_bootstrap_identity_without_callbacks();need(mapping_bootstrap_returned_&&bound_&&cold_bound_,"FS bootstrap has no genuine bound closed donor");
        source_current();mapping_bootstrap_identity_without_callbacks();
    }
    void documents_identity_without_callbacks()const {
        creator();need(!failed_&&mapping_bootstrap_returned_&&mapping_body_returned_&&!active_&&!active_bootstrap_&&bound_&&cold_bound_&&phase_==Phase::bound&&
             module_returned_&&app_returned_&&hook_returned_&&config_fixup_returned_&&seed_returns_==3&&
             roots_->initialized[0]&&roots_->initialized[1]&&parent_identity_&&!input_.expired()&&!parent_.expired()&&!cold_.expired(),
             "FS Documents has no genuine same bootstrap/Roots/Parent/Cold history");
        // Typed Documents metadata provenance only. Mutable list/cache and
        // full-bootstrap currentness belong to their separate closed lender.
    }
    std::filesystem::path copy_documents()const {
        documents_identity_without_callbacks();
        // Typed Documents metadata only, matching the old Directory getter's
        // physical source. Its existing Parent/Boot caller owns Input/fence
        // bookends; this cannot authorize a FS writer/full bootstrap lender.
        const auto check=[&]{documents_identity_without_callbacks();
            need(emitted_root(1)==roots_->raw[1]&&std::filesystem::weakly_canonical(std::filesystem::path(wide(roots_->raw[1])))==roots_->physical[1],
                 "FS Documents paired producer changed");require_existing_directory(roots_->physical[1]);
            need(!input_.expired()&&!parent_.expired()&&!cold_.expired(),"FS Documents provider POST retired ancestry");};
        try {check();auto value=roots_->physical[1];check();return value;}catch(...){fail();throw;}
    }
    // PRIVATE9a same-storage memory writer. The Parent's actual E3b method is
    // the sole caller; no external ordinal/name/list/count can construct it.
    struct LateMappingsHistory final {
        const std::size_t initial_extent;
        const std::vector<Descriptor> earlier_general,earlier_config;
        const std::array<std::u16string,2> earlier_raw_roots;
        const std::array<std::filesystem::path,2> earlier_physical_roots;
        const std::shared_ptr<const Roots> actual_roots;
        std::size_t append_returns=0;
        bool failed=false,body_returned=false,operation_retired=false,header_retired=false,returned=false;
        explicit LateMappingsHistory(const BnwNativeFilesystemMappingOwner& owner)
            :initial_extent(owner.general_.size()),earlier_general(owner.general_),earlier_config(owner.config_),
             earlier_raw_roots(owner.roots_->raw),earlier_physical_roots(owner.roots_->physical),actual_roots(owner.roots_){}
    };
    class LateMappingsOperation final {
        friend class BnwNativeFilesystemMappingOwner;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        BnwNativeFilesystemMappingOwner& owner_;
        const std::thread::id creator_=std::this_thread::get_id();
        std::size_t next_site_=0;
        bool body_returned_=false;
        explicit LateMappingsOperation(BnwNativeFilesystemMappingOwner&);
        ~LateMappingsOperation()noexcept;
        LateMappingsOperation(const LateMappingsOperation&)=delete;
        LateMappingsOperation& operator=(const LateMappingsOperation&)=delete;
        static const char16_t* literal_for_actual_site(std::size_t);
        void identity()const;
        void current()const;
        void append_actual_next_site();
        void finish_actual_body();
    };
    const LateMappingsOperation* late_writer_=nullptr;
    std::unique_ptr<LateMappingsHistory> late_mappings_;
    void late_mappings_identity_without_callbacks()const;
    void require_late_mappings_returned_current()const;

  public:
    ~BnwNativeFilesystemMappingOwner()=default; // Actual owned values only; no I/O/controller during destruction.
    BnwNativeFilesystemMappingOwner(const BnwNativeFilesystemMappingOwner&)=delete;
};
} // namespace civ5::ui::detail

namespace civ5::ui {
// Closed typed adoption of the SAME paired Roots, not a caller path/ready DTO.
class BnwNativePackageDiscoveryOwner::InstalledFilesystemDocumentsLoan final {
    friend class BnwNativePackageDiscoveryOwner;
    friend class BnwNativeUserDirectoryOwner;
    using FS=detail::BnwNativeFilesystemMappingOwner;
    const DWORD process_=GetCurrentProcessId(),thread_=GetCurrentThreadId();
    const std::weak_ptr<FS> owner_;
    const std::weak_ptr<const FS::Roots> roots_;
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent_;
    const BnwNativePackageDiscoveryOwner* parent_identity_;
    InstalledFilesystemDocumentsLoan(const std::shared_ptr<FS>& owner,const BnwNativePackageDiscoveryOwner& actual)
        :owner_(owner),roots_(owner->roots_),parent_(owner->parent_),parent_identity_(&actual){
        FS::need(owner->parent_identity_==&actual,"FS Documents loan substituted Parent");(void)copy_actual_directory();
    }
    std::filesystem::path copy_actual_directory()const {
        FS::need(process_==GetCurrentProcessId()&&thread_==GetCurrentThreadId(),"FS Documents loan foreign creator");
        const auto owner=owner_.lock();const auto roots=roots_.lock();
        FS::need(owner&&roots&&roots.get()==owner->roots_.get()&&FS::same(roots,owner->roots_)&&
             owner->parent_identity_==parent_identity_&&FS::same(parent_,owner->parent_)&&!parent_.expired(),
             "FS Documents loan lost SAME actual Roots/owner/Parent control");
        return owner->copy_documents();
    }
    void require_actual_directory_current(const std::filesystem::path& value)const {
        FS::need(copy_actual_directory()==value,"FS Documents facade changed actual adopted root");
    }
  public:
    ~InstalledFilesystemDocumentsLoan()=default;
    InstalledFilesystemDocumentsLoan(const InstalledFilesystemDocumentsLoan&)=delete;
};
} // namespace civ5::ui
