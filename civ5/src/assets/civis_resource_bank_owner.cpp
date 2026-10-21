#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/assets/detail/civis_port_namespace_custody.hpp"
#include "civ5/assets/detail/civis_loaded_xml_physical_path_loan.hpp"
#include "civ5/assets/detail/civis_loaded_bytes_physical_path_loan.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "../ui/bnw_native_owned_startup_completion.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/lua_assets.hpp"
#include "civ5/platform/windows_path.hpp"
#include "../ui/bnw_native_category_pattern_match.hpp"
#include "../ui/bnw_native_loose_walk.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace civ5::assets {
namespace fs = std::filesystem;
namespace {
void need(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
fs::path utf8(std::string_view s) { return fs::path(std::u8string(s.begin(),s.end())); }
bool same(const runtime::FileIdentity& a,const runtime::FileIdentity& b) {
    return a.bytes==b.bytes && a.sha256==b.sha256;
}
fs::path relative(std::string_view text) {
    AssetKey key{std::string(text)};
    need(!text.empty() && key.safe_relative_path(),"Civis recipe contains an unsafe relative path");
    return utf8(text);
}
// Explicit File '*' selectors use the actual nonrecursive FindA order.
// This cold Civis recipe reuses private parser/enumerator primitives only;
// it does not issue an Original namespace/category completion.
void add_declared_file(LooseResourceIndex& index,const fs::path& source_root,
                       const std::string& declaration) {
    const auto requested=relative(declaration);
    namespace patterns=ui::native_category_patterns_detail;
    namespace native=ui::native_loose_detail;
    if(!patterns::explicit_file_uses_find_a_ascii(declaration)) {
        index.add_file(requested);return;
    }
    const auto parent=requested.parent_path();
    need(path_utf8(parent).find_first_of("*?")==std::string::npos,
         "Civis File selector has a wildcard directory");
    const auto root=fs::canonical(source_root);
    const auto folder=(root/parent).lexically_normal();
    // Probe real ancestors, never the literal wildcard operand. Absent
    // directories remain a genuine empty FindA result; other errors fail.
    const auto code_page=GetACP();
    for(auto path=folder;path!=root;path=path.parent_path()) {
        need(!path.empty(),"Civis File selector parent escaped its root");
        const auto attributes=GetFileAttributesA(native::acp_path(path).c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES)
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Civis File selector traverses a reparse point");
        else {
            const auto error=GetLastError();
            need(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND,
                 "Civis File selector parent probe failed");
        }
    }
    native::NativeEnumeration enumeration;
    auto cursor=enumeration.open(native::acp_path(folder/requested.filename()));
    if(!cursor.valid()) {
        const auto error=cursor.error();
        need(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND,
             "Civis File selector enumeration failed");
        need(GetACP()==code_page,"Civis File selector code page changed");return;
    }
    do {
        const auto& row=cursor.data;
        const std::string name=row.cFileName;patterns::ascii_input(name);
        if(patterns::explicit_find_a_attributes_selected(row.dwFileAttributes)) {
            need(!(row.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),
                 "Civis File selector selected a reparse point");
            index.add_file(parent/fs::path(native::acp_wide(name)));
        }
    } while(cursor.next());
    need(cursor.error()==ERROR_NO_MORE_FILES,"Civis File selector traversal incomplete");
    need(GetACP()==code_page,"Civis File selector code page changed");
}
struct SourcePin {
    const fs::path path;
    HANDLE handle=INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION stamp{};
    explicit SourcePin(fs::path p):path(std::move(p)) {
        handle=CreateFileW(platform::extended_windows_path(path).c_str(),GENERIC_READ,
            FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        need(handle!=INVALID_HANDLE_VALUE,"Civis resource source cannot be locked read-only");
        if (!GetFileInformationByHandle(handle,&stamp)) {
            CloseHandle(handle);handle=INVALID_HANDLE_VALUE;
            throw std::runtime_error("Civis resource source identity is unavailable");
        }
        if(stamp.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
            CloseHandle(handle);handle=INVALID_HANDLE_VALUE;
            throw std::runtime_error("Civis resource is a directory");
        }
    }
    ~SourcePin() { if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); }
    SourcePin(const SourcePin&)=delete;
    static bool equal(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b) {
        return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber &&
            a.nFileIndexHigh==b.nFileIndexHigh && a.nFileIndexLow==b.nFileIndexLow &&
            a.nFileSizeHigh==b.nFileSizeHigh && a.nFileSizeLow==b.nFileSizeLow &&
            a.ftLastWriteTime.dwHighDateTime==b.ftLastWriteTime.dwHighDateTime &&
            a.ftLastWriteTime.dwLowDateTime==b.ftLastWriteTime.dwLowDateTime;
    }
    Bytes read_range(std::uint64_t offset,std::uint64_t size) const {
        need(size<=std::numeric_limits<std::size_t>::max() &&
            offset<=static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max()),
            "Civis retained stream range is unrepresentable");
        LARGE_INTEGER position{};position.QuadPart=static_cast<LONGLONG>(offset);
        need(SetFilePointerEx(handle,position,nullptr,FILE_BEGIN)!=FALSE,
            "Civis retained stream cannot seek");
        Bytes bytes(static_cast<std::size_t>(size));std::size_t at=0;
        while(at<bytes.size()) {
            const auto wanted=static_cast<DWORD>(std::min<std::size_t>(
                bytes.size()-at,std::numeric_limits<DWORD>::max()));
            DWORD count=0;
            need(ReadFile(handle,bytes.data()+at,wanted,&count,nullptr)!=FALSE && count==wanted,
                "Civis retained source ended before its declared payload");
            at+=count;
        }
        return bytes;
    }
    void held_current() const {
        BY_HANDLE_FILE_INFORMATION held{};
        need(GetFileInformationByHandle(handle,&held) && equal(held,stamp),
            "Civis retained resource source changed");
    }
    std::array<DWORD,3> identity_key() const {
        return {stamp.dwVolumeSerialNumber,stamp.nFileIndexHigh,stamp.nFileIndexLow};
    }
    void current() const {
        held_current();BY_HANDLE_FILE_INFORMATION named{};
        auto h=CreateFileW(platform::extended_windows_path(path).c_str(),GENERIC_READ,
            FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        need(h!=INVALID_HANDLE_VALUE,"Civis resource source path retired");
        const bool ok=GetFileInformationByHandle(h,&named)!=FALSE;CloseHandle(h);
        need(ok && equal(named,stamp),"Civis resource path names another source");
    }
};
std::uint64_t xml_cost(const content::XmlNode& n) {
    std::uint64_t total=sizeof(n)+n.name.capacity()+n.text.capacity();
    const auto add=[&](std::uint64_t x) {
        need(x<=UINT64_MAX-total,"Civis parsed resource budget overflow");total+=x;
    };
    add(n.attributes.capacity()*sizeof(n.attributes[0]));
    add(n.children.capacity()*sizeof(content::XmlNode));
    for(const auto& [a,b]:n.attributes) { add(a.capacity());add(b.capacity()); }
    for(const auto& c:n.children) add(xml_cost(c));
    return total;
}
std::vector<std::string> declarations(const content::PackageManifest& p,
    content::PackageCategory category,std::uint64_t occurrence,std::string selector) {
    content::PackageResourceQuery q;
    q.category=category;q.category_occurrence=occurrence;q.enabled_categories={true,true,true};
    q.selector=std::move(selector);std::vector<std::string> result;
    for(const auto& d:content::package_resources(p,q)) result.push_back(d.value);
    return result;
}
}
namespace detail {
struct CivisResourceBankState {
    const std::thread::id creator=std::this_thread::get_id();
    const DWORD process=GetCurrentProcessId();
    std::atomic<bool> alive{true};
    mutable std::atomic<bool> failed{false};
    bool active=false,published=false;
    struct Entry {MountedResource winner;std::vector<MountedResource> all;};
    std::map<std::string,Entry> names,loose_fallback;
    std::vector<std::string> keys;
    const std::uint64_t epoch=1;
    CivisResourceBankBudget budget;
    std::shared_ptr<const ui::BnwNativeEarlyInputLease> input;
    std::weak_ptr<CivisResourceBankOwner> canonical_owner;
    fs::path save,root,original;
    std::string recipe_identity;
    std::vector<CivisResourceMountDescriptor> mounts;
    content::Catalog catalog;
    runtime::SavedContentProfile profile;
    std::unique_ptr<content::LooseResourcePlan> physical;
    std::unique_ptr<ResourceMountTable> memory;
    std::shared_ptr<const detail::CivisPortNamespaceCustody> port_namespace_custody;
    // Separate genuine native source controls. No early Package is installed in CivisHost.
    std::shared_ptr<const ui::detail::BnwNativeOwnedStartupCompletion> context_native_include_source;
    std::map<fs::path,std::unique_ptr<SourcePin>> pins;
    std::set<fs::path> anchor_paths;
    std::set<std::array<DWORD,3>> anchor_ids;
    std::map<std::array<DWORD,3>,fs::path> loaded_sources;
    std::map<std::string,CivisLoadedResource> cache;
    std::uint64_t resident=0;
    void pure(bool inside=false) const {
        need(std::this_thread::get_id()==creator && GetCurrentProcessId()==process,
            "Civis bank belongs to another creator process/thread");
        if(active && !inside) failed.store(true);
        need(alive.load() && !failed.load() && (inside || !active),
            "Civis bank retired, failed or reentered");
        need(static_cast<bool>(input),"Civis actual early input retired");
        need(!canonical_owner.expired(),"Civis canonical bank owner retired");
    }
    SourcePin& pin(const fs::path& path) {
        auto actual=fs::canonical(platform::extended_windows_path(path));
        actual=platform::unextended_windows_path(actual);
        auto found=pins.find(actual);if(found!=pins.end()) return *found->second;
        need(pins.size()<budget.backing_files,"Civis namespace exceeds backing-file budget");
        auto value=std::make_unique<SourcePin>(actual);
        auto [at,added]=pins.emplace(actual,std::move(value));return *at->second;
    }
    const Entry* entry(const AssetKey& key) const {
        if(key.original_name().empty()) return nullptr;
        auto full=names.find(key.normalized_name());if(full!=names.end()) return &full->second;
        auto base=loose_fallback.find(resource_basename(key.original_name()));
        return base==loose_fallback.end()?nullptr:&base->second;
    }
    void add_anchor(const SourcePin& actual) {
        anchor_paths.insert(actual.path);anchor_ids.insert(actual.identity_key());
    }
    void client_boundary() const {
        pure(true);
        const auto actual=input;need(static_cast<bool>(actual),"Civis actual input expired at source boundary");
        actual->require_current();pure(true);
    }
    void source_boundary(const std::set<fs::path>& additionally_named={}) const {
        client_boundary();
        for(const auto& path:anchor_paths) pins.at(path)->current();
        // Immutable cache authority is its still-owned read-only FILE handle,
        // not a fresh directory walk or lookup of every namespace spelling.
        // Dedup by actual volume/FileId, including symlink and hard-link aliases.
        for(const auto& [id,path]:loaded_sources)
            if(!anchor_ids.contains(id)) pins.at(path)->held_current();
        // A real read's selected named sources must remain SAME even if a
        // client callback replaces a path after the retained-handle read.
        for(const auto& path:additionally_named)
            if(!anchor_paths.contains(path)) pins.at(path)->current();
        pure(true);
    }
    void full_namespace_for_publication() const {
        client_boundary();
        for(const auto& [path,p]:pins) p->current();
        pure(true);
    }
};
struct Operation {
    std::shared_ptr<CivisResourceBankState> s;
    bool complete=false;
    explicit Operation(std::shared_ptr<CivisResourceBankState> value):s(std::move(value)) {
        if(s->creator!=std::this_thread::get_id())
            throw std::runtime_error("Civis bank operation belongs to another creator");
        if(s->active) {s->failed.store(true);throw std::runtime_error("Civis bank operation reentered");}
        s->pure();s->active=true;
    }
    ~Operation() {if(!complete)s->failed.store(true);s->active=false;}
    void finish() {s->pure(true);complete=true;}
};
}
using detail::CivisResourceBankState;

#include "civis_loaded_bytes_physical_path_core.inc"

struct detail::CivisLoadedXmlPhysicalPathLoan::Impl {
    const std::weak_ptr<CivisResourceBankState> bank;
    const std::uint64_t epoch;
    const CivisResourceBankState::Entry* const entry;
    const AssetHandle* const winner;
    const SourcePin* const source;
    const std::string physical_id;
    const fs::path path;
    const std::shared_ptr<const Bytes> bytes;
    const std::shared_ptr<const content::XmlNode> xml;
    const runtime::FileIdentity payload;
    std::atomic<bool> alive{true};
    Impl(const std::shared_ptr<CivisResourceBankState>& state,const CivisResourceBankState::Entry* actual_entry,
        const AssetHandle* actual_winner,const SourcePin* actual_source,std::string id,
        const fs::path& actual_path,const CivisLoadedResource& loaded):
        bank(state),epoch(state->epoch),entry(actual_entry),winner(actual_winner),source(actual_source),
        physical_id(std::move(id)),path(actual_path),bytes(loaded.bytes),xml(loaded.xml),payload(loaded.payload_identity){}
    std::shared_ptr<CivisResourceBankState> first()const {
        const auto state=bank.lock();
        need(alive.load()&&state&&state->epoch==epoch&&state->published,"XML physical path loan retired");
        state->pure();
        need(std::get_if<AssetHandle>(&entry->winner.asset)==winner,"XML winning source changed");
        const auto held=state->pins.find(path);
        need(held!=state->pins.end()&&held->second.get()==source,"XML original physical SourcePin changed");
        const auto cached=state->cache.find(physical_id);
        need(cached!=state->cache.end(),"XML original loaded cache entry retired");
        const auto& value=cached->second;
        need(value.bytes.get()==bytes.get()&&!value.bytes.owner_before(bytes)&&!bytes.owner_before(value.bytes)&&
            value.xml.get()==xml.get()&&!value.xml.owner_before(xml)&&!xml.owner_before(value.xml)&&
            value.payload_identity.bytes==payload.bytes&&value.payload_identity.sha256==payload.sha256,
            "XML original loaded payload/parse identity changed");
        state->pure();return state;
    }
};
detail::CivisLoadedXmlPhysicalPathLoan::CivisLoadedXmlPhysicalPathLoan(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::CivisLoadedXmlPhysicalPathLoan::~CivisLoadedXmlPhysicalPathLoan(){impl_->alive.store(false);}
void detail::CivisLoadedXmlPhysicalPathLoan::require_current_without_callbacks()const {
    const auto value=impl_;(void)value->first();
}
void detail::CivisLoadedXmlPhysicalPathLoan::require_same_bank_without_callbacks(const CivisResourceBankOwner& actual)const {
    const auto value=impl_;const auto state=value->first();const auto owner=state->canonical_owner.lock();
    need(owner&&owner.get()==&actual,"XML physical source belongs to another canonical Bank");
    (void)value->first();
}
fs::path detail::CivisLoadedXmlPhysicalPathLoan::physical_path_owned()const {
    const auto value=impl_;(void)value->first();auto result=value->path;(void)value->first();return result;
}
std::shared_ptr<const content::XmlNode> detail::CivisLoadedXmlPhysicalPathLoan::actual_xml_owned()const {
    const auto value=impl_;(void)value->first();const auto result=value->xml;(void)value->first();return result;
}
std::shared_ptr<const detail::CivisLoadedXmlPhysicalPathLoan>
CivisResourceBankOwner::capture_loaded_xml_physical_path_owned(const std::shared_ptr<CivisResourceBankState>& state,const AssetKey& key){
    using Loan=detail::CivisLoadedXmlPhysicalPathLoan;
    const auto s=state;s->pure();need(s->published,"XML physical source requires published Bank");
    const auto entry=s->entry(key);need(entry,"XML physical source indexed key missing");
    const auto h=std::get_if<AssetHandle>(&entry->winner.asset);
    need(h&&h->candidate().provenance.kind==SourceKind::loose&&h->offset_==0,
        "XML physical watcher source needs actual loose winner; container/missing/ambiguous unsupported");
    const auto id=physical_id(*h);s->pure();
    const auto cached=s->cache.find(id);need(cached!=s->cache.end(),"XML physical source is not already loaded");
    const auto loaded=cached->second;
    need(loaded.bytes&&loaded.xml&&loaded.payload_identity.bytes==h->candidate().payload_size&&
        loaded.bytes->size()==loaded.payload_identity.bytes,"XML physical source lacks actual loaded parse");
    const auto held=s->pins.find(h->file_);
    need(held!=s->pins.end(),"XML actual winning file has no exact captured SourcePin");
    const auto*const source=held->second.get();
    const auto size=(static_cast<std::uint64_t>(source->stamp.nFileSizeHigh)<<32)|source->stamp.nFileSizeLow;
    need(size==loaded.payload_identity.bytes,"XML winning payload is not the complete physical file");
    // Copy all physical inputs before allocating; no reconstructed recipe path.
    const auto path=source->path;s->pure();
    auto data=std::make_shared<Loan::Impl>(s,entry,h,source,id,path,loaded);
    (void)data->first();
    auto result=std::shared_ptr<const Loan>(new Loan(data));
    (void)data->first();return result;
}
std::shared_ptr<const detail::CivisLoadedXmlPhysicalPathLoan>
CivisResourceBankCapability::loaded_xml_physical_path_owned(const AssetKey& key)const {
    require_current();const auto state=state_.lock();
    need(state&&state->epoch==epoch_,"XML path capability Bank epoch changed");
    return CivisResourceBankOwner::capture_loaded_xml_physical_path_owned(state,key);
}

CivisResourceBankCapability::CivisResourceBankCapability(
    const std::shared_ptr<CivisResourceBankState>& s):state_(s),epoch_(s->epoch) {}
void CivisResourceBankCapability::require_current() const {
    auto s=state_.lock();need(s && s->epoch==epoch_ && s->published,"Civis capability retired");s->pure();
}
CivisLoadedResource CivisResourceBankCapability::loaded(const AssetKey& key) const {
    require_current();auto s=state_.lock();s->pure();
    const auto entry=s->entry(key);
    need(entry,"Required Civis loaded resource is missing");
    const auto h=std::get_if<AssetHandle>(&entry->winner.asset);
    need(h,"Required Civis loaded resource is missing or ambiguous");
    const auto found=s->cache.find(CivisResourceBankOwner::physical_id(*h));
    need(found!=s->cache.end(),"Civis resource has not been loaded");
    s->pure();auto value=found->second;value.candidate=h->candidate();return value;
}
CivisResourceBankOwner::CivisResourceBankOwner(std::shared_ptr<CivisResourceBankState> s):state_(std::move(s)) {}
CivisResourceBankOwner::~CivisResourceBankOwner() {state_->alive.store(false);}
std::string CivisResourceBankOwner::physical_id(const AssetHandle& h) {
    return path_utf8(h.file_)+"\n"+std::to_string(h.offset_)+"\n"+std::to_string(h.candidate().payload_size);
}
void CivisResourceBankOwner::capture_handle(CivisResourceBankState& s,const AssetHandle& h) {
    auto& pin=s.pin(h.file_);pin.current();
    const auto size=(std::uint64_t(pin.stamp.nFileSizeHigh)<<32)|pin.stamp.nFileSizeLow;
    need(h.offset_<=size && h.candidate().payload_size<=size-h.offset_,"Civis indexed payload exceeds source");
}
ResourceMountHit CivisResourceBankOwner::memory_lookup(
    const std::weak_ptr<CivisResourceBankState>& weak,const AssetKey& key) {
    auto s=weak.lock();need(s && s->published,"Civis memory namespace retired");s->pure();
    const auto entry=s->entry(key);if(!entry) return {};
    const auto& selected=entry->winner;auto h=std::get_if<AssetHandle>(&selected.asset);
    if(!h) return {selected.asset,true};
    auto found=s->cache.find(physical_id(*h));
    if (!(found!=s->cache.end()))
        throw std::runtime_error(std::string("Indexed Civis resource must be loaded before memory-only lookup") +
            ": requested=[" + key.original_name() + "] normalized=[" + key.normalized_name() +
            "] variant=" + std::to_string(selected.asset.index()) +
            " candidate=[" + h->candidate().original_name + "] source_id=[" + h->candidate().provenance.source_id +
            "] source_path=[" + h->candidate().provenance.source_relative_path +
            "] archive=[" + h->candidate().provenance.archive_relative_path.value_or("<none>") +
            "] kind=" + (h->candidate().provenance.kind==SourceKind::fpk ? "fpk" : "loose") +
            " mount=" + (selected.mount_id ? std::to_string(*selected.mount_id) : "<default>"));
    return {AssetHandle(key,h->candidate(),found->second.bytes),true};
}
std::vector<ResourceMountHit> CivisResourceBankOwner::memory_lookup_all(
    const std::weak_ptr<CivisResourceBankState>& weak,const AssetKey& key) {
    auto s=weak.lock();need(s && s->published,"Civis memory namespace retired");s->pure();
    std::vector<ResourceMountHit> out;
    const auto entry=s->entry(key);if(!entry) return out;
    for(const auto& selected:entry->all) {
        auto h=std::get_if<AssetHandle>(&selected.asset);
        if(!h) {out.push_back({selected.asset,true});continue;}
        auto found=s->cache.find(physical_id(*h));
        need(found!=s->cache.end(),"All-matching Civis payload must be loaded before memory lookup");
        out.push_back({AssetHandle(key,h->candidate(),found->second.bytes),true});
    }
    return out;
}
std::shared_ptr<CivisResourceBankOwner> CivisResourceBankOwner::capture(
    const std::shared_ptr<const ui::BnwNativeEarlyInputLease>& input,
    std::span<const CivisRequiredResource> required,CivisResourceBankBudget budget) {
    need(input && !required.empty(),"Civis bank needs genuine input and a required closure");
    need(budget.resource_bytes && budget.resident_bytes>=budget.resource_bytes &&
        budget.backing_files && budget.indexed_names,"Civis resource budget is invalid");
    // Authenticate supplied control before the first client callback. A
    // no-op-deleter alias cannot substitute its control for the real factory.
    const auto input_owner=input->retain_owner();
    need(input_owner.get()==input.get() && !input_owner.owner_before(input) &&
        !input.owner_before(input_owner),"Civis input is not its canonical factory control");
    auto s=std::make_shared<CivisResourceBankState>();s->input=input_owner;s->budget=budget;
    const auto input_post=[&] {s->pure(true);
        const auto current=input_owner->retain_owner();
        need(current.get()==input_owner.get() && !current.owner_before(input_owner) &&
            !input_owner.owner_before(current),"Civis input factory control changed");};
    auto owner=std::shared_ptr<CivisResourceBankOwner>(new CivisResourceBankOwner(s));
    s->canonical_owner=owner;
    detail::Operation op(s);input_owner->require_current();input_post();
    auto next_input_check=std::chrono::steady_clock::now();
    const auto cooperate=[&] {
        const auto now=std::chrono::steady_clock::now();
        if(now<next_input_check)return;
        next_input_check=now+std::chrono::milliseconds(100);
        // Use the genuine callback and its existing owner POST. A UI may
        // service messages here; no readiness or source check is cached.
        input_owner->require_current();input_post();
    };
    const auto trace=[](const char* phase) {
        std::fprintf(stderr,"[CIVIS_BANK] phase=%s\n",phase);std::fflush(stderr);
    };
    s->original=input_owner->game_root();input_post();s->root=input_owner->canonical_payload_root();input_post();
    s->profile=input_owner->profile();input_post();s->catalog=input_owner->catalog();input_post();
    s->save=input_owner->save_path();input_post();
    s->add_anchor(s->pin(s->save));s->add_anchor(s->pin(s->original/"CivilizationV.exe"));
    s->add_anchor(s->pin(s->original/"Assets/DLC/Expansion2/CvGameCore_Expansion2.dll"));
    std::string identity="Civis.BNW.ZeroMod.Common.DX9.v1\n"+path_utf8(s->save)+"\n"+
        path_utf8(s->original)+"\n"+path_utf8(s->root)+"\n";
    need(s->profile.can_activate && s->profile.all_requirements_located && s->profile.enabled_mods.empty() &&
        !s->profile.native_mods_require_port,"Civis bank requires the actual verified zero-mod profile");
    bool bnw=false;std::set<std::size_t> selected;
    for(auto i:s->profile.enabled_dlc) {
        cooperate();
        need(selected.insert(i).second,"Civis selected DLC occurs twice");
        auto& package=s->catalog.packages.at(i);
        need(package.kind==content::PackageKind::dlc,"Civis selected recipe contains a non-DLC package");
        bnw |= content::canonical_guid(package.id)=="6da07636-4123-4018-b643-6575b4ec336b";
        const auto rel=package.path.lexically_relative(s->original);
        need(!rel.empty() && AssetKey(path_utf8(rel)).safe_relative_path(),"Civis selected manifest escaped installation");
        auto& original=s->pin(package.path);s->add_anchor(original);original.current();
        const auto first=runtime::identify_file(package.path);original.current();
        identity+=std::to_string(i)+" "+content::canonical_guid(package.id)+" "+
            std::to_string(package.version)+" "+first.sha256+"\n";
        const auto mirror=s->root/rel;auto& held=s->pin(mirror);s->add_anchor(held);held.current();
        need(same(first,runtime::identify_file(mirror)),"Civis selected payload manifest differs from actual input");
        package=content::read_manifest(mirror);held.current();
        need(same(first,runtime::identify_file(mirror)),"Civis selected manifest changed during parse");
    }
    need(bnw,"Civis resource recipe does not select BNW");
    trace("selected-loose-plan");cooperate();
    s->physical=std::make_unique<content::LooseResourcePlan>(
        content::build_selected_loose_resource_plan(s->root,s->profile.enabled_dlc,s->catalog));
    const auto selected_loose=s->physical->resources.mounts();
    for(const auto& actual:selected_loose) s->physical->resources.unmount(actual.id);
    trace("base-ui");cooperate();
    content::mount_base_game_lua(s->physical->resources,s->root);
    auto base_ui=std::make_shared<LooseResourceIndex>(s->root);base_ui->scan({"Assets/UI",{},true});
    std::uint64_t id=1;
    for(const auto& m:s->physical->resources.mounts()) {
        need(m.id<UINT64_MAX,"Civis mount ID overflow");id=std::max(id,m.id+1);
    }
    const auto mount=[&](std::shared_ptr<const AssetSource> source,bool preferred) {
        need(id<UINT64_MAX,"Civis mount ID overflow");
        s->physical->resources.mount({id++,0,0,resource_lookup(source,preferred),true,preferred,
            resource_lookup_all(source,preferred),resource_keys(source),source});
    };
    mount(base_ui,false);
    // Exact BASE XML inputs of the actual five-table unit art compiler.
    // Admit only these authored files, before all selected preferred sources.
    auto base_art=std::make_shared<LooseResourceIndex>(s->root);
    for(const auto* key:{"Assets/Units/Civ5ArtDefines_UnitMembers.xml",
                        "Assets/Units/Civ5ArtDefines_Units.xml",
                        "Assets/Units/Civ5ArtDefines_UnitMemberTemplates.xml"})
        base_art->add_file(relative(key));
    mount(base_art,false);
    // A concrete Civis desktop recipe. Declaration order uses the existing
    // table's register/resolve rules; no invented global priority sort.
    const auto archive=[&](const std::string& name,bool required_archive,bool preferred) {
        cooperate();
        need(name.find_first_of("/\\:")==std::string::npos &&
            normalize_asset_name(path_utf8(utf8(name).extension()))==".fpk","Civis Pak basename is unsupported");
        auto rel=std::string("Resource/DX9/")+name;
        if(!fs::exists(s->root/utf8(rel))) {
            if(!required_archive) return; // Existing optional base-DX9 recipe.
            // Actual declared Paks use the existing first-present DX9 then
            // Common lookup; a present but invalid DX9 source never falls back.
            rel=std::string("Resource/Common/")+name;
        }
        auto& held=s->pin(s->root/utf8(rel));held.current();
        auto opened=FpkSource::open(held.path,rel);
        auto source=std::get_if<std::shared_ptr<FpkSource>>(&opened);
        need(source && *source,"Civis declared FPK cannot be parsed");held.current();mount(*source,preferred);
    };
    static constexpr const char* base_paks[]={"AmericanWashingtonTextures","ArabiaHarunAl-RashidTextures",
        "AztecMontezumaTextures","BuildingTextures","ChineseWuTextures","CityTextures","DecalTextures",
        "EffectTextures","EgyptianRamessesTextures","EnglandElizabethTextures","FeatureTextures",
        "FrenchNapoleonTextures","GermanBismarckTextures","GreeceAlexanderTextures","HutsBarbarianFortTextures",
        "ImprovementTextures","IndianGandhiTextures","IroquoisHiawathaTextures","JapaneseOdaNobunugaTextures",
        "LeadersTextures","MiscTextures","OttomanSuliemanTextures","PersiaDariusTextures","RomanCaesarTextures",
        "RussianCatherineTextures","SiamRamkhamaengTextures","SonghaiAskiaTextures","StrategicViewTextures",
        "TerrainTextures","UITextures","UnitsTextures","WonderTextures"};
    trace("base-archives");
    for(const auto name:base_paks) archive(std::string(name)+".fpk",false,false);
    // Own audio's mandatory BASE archive; selected DLC keeps its later precedence.
    {
        constexpr auto rel="Resource/Common/SoundData.fpk";
        auto& held=s->pin(s->root/utf8(rel));held.current();
        auto opened=FpkSource::open(held.path,rel);
        auto source=std::get_if<std::shared_ptr<FpkSource>>(&opened);
        need(source && *source,"Civis mandatory base SoundData FPK cannot be parsed");
        held.current();mount(*source,false);
    }
    std::size_t loose_cursor=0;
    trace("selected-archives");
    for(auto i:s->profile.enabled_dlc) {
        cooperate();
        const auto& p=s->catalog.packages.at(i);
        for(auto category:{content::PackageCategory::common,content::PackageCategory::gameplay}) {
            for(const auto& name:declarations(p,category,0,"Paks")) archive(name,true,true);
            // Consume precisely the corresponding provider from the actual
            // existing builder, retaining its lookup/all/key closures.
            const auto dirs=declarations(p,category,0,"Directory");
            const auto maps=declarations(p,category,0,"MapDirectory");
            const auto files=declarations(p,category,0,"File");
            if(!dirs.empty() || !maps.empty() || !files.empty()) {
                need(loose_cursor<selected_loose.size(),"Civis selected loose recipe is incomplete");
                auto actual=selected_loose[loose_cursor++];
                need(id<UINT64_MAX,"Civis mount ID overflow");actual.id=id++;
                s->physical->resources.mount(std::move(actual));
            }
        }
        std::uint64_t occurrence=0;
        for(const auto& skin:p.document.children) {
            if(skin.name!="UISkin") continue;
            const auto own=occurrence++;
            if(skin.attribute("platform")!="Common") continue;
            for(const auto block:{"Skin","GameplaySkin"}) {
                for(const auto& name:declarations(p,content::PackageCategory::ui_skin,own,std::string(block)+"/Paks"))
                    archive(name,true,true);
                auto index=std::make_shared<LooseResourceIndex>(p.path.parent_path());
                for(const auto& dir:declarations(p,content::PackageCategory::ui_skin,own,std::string(block)+"/Directory"))
                    index->scan({relative(dir),{},true});
                for(const auto selector:{"File","Style","Color"})
                    for(const auto& file:declarations(p,content::PackageCategory::ui_skin,own,std::string(block)+"/"+selector))
                        if(std::string_view(selector)=="File") add_declared_file(*index,p.path.parent_path(),file);
                        else index->add_file(relative(file));
                mount(index,true);
            }
        }
    }
    need(loose_cursor==selected_loose.size(),"Civis selected loose providers were not all consumed");
    for(const auto& m:s->physical->resources.mounts()) {
        s->mounts.push_back({m.id,m.group,m.priority,m.enabled,m.preferred});
        identity+="mount "+std::to_string(m.id)+" "+std::to_string(m.group)+" "+
            std::to_string(m.priority)+" "+std::to_string(m.enabled)+" "+
            std::to_string(m.preferred)+"\n";
    }
    s->keys=s->physical->resources.keys();
    need(s->keys.size()<=budget.indexed_names,"Civis namespace exceeds indexed-name budget");
    std::set<std::string> normalized;
    for(const auto& key:s->keys) normalized.insert(normalize_asset_name(key));
    // The selected loose indices have heterogeneous roots. Publish each
    // genuine loose candidate's payload-root-qualified name while retaining
    // its physical source and the table's actual winner/all-hit ordering.
    const auto project_loose=[&](const AssetKey& requested,const AssetHandle& h) {
        const auto& source=h.candidate().provenance;
        if(source.kind!=SourceKind::loose) return h;
        need(h.loose_root_.has_value(),"Civis loose candidate has no actual source root");
        const auto source_root=fs::canonical(*h.loose_root_);
        need(fs::canonical(utf8(source.source_id))==source_root &&
             fs::canonical(source_root/relative(source.source_relative_path))==h.file_,
             "Civis loose candidate physical provenance differs");
        const auto qualified=path_utf8(h.file_.lexically_relative(s->root));
        const AssetKey qualified_key(qualified);
        need(!qualified.empty() && qualified_key.safe_relative_path(),
             "Civis loose candidate escaped canonical payload root");
        AssetCandidate candidate{qualified,qualified_key.normalized_name(),source,h.candidate().payload_size};
        return AssetHandle(requested,std::move(candidate),h.file_,h.offset_,h.loose_root_);
    };
    const auto snapshot=[&](const AssetKey& key) {
        cooperate();
        CivisResourceBankState::Entry result{s->physical->resources.resolve(key),
            s->physical->resources.resolve_all(key)};
        const auto capture=[&](MountedResource& row) {
            if(auto h=std::get_if<AssetHandle>(&row.asset)) {
                capture_handle(*s,*h);*h=project_loose(key,*h);
            }
        };
        capture(result.winner);
        for(auto& row:result.all) capture(row);
        return result;
    };
    std::map<std::string,std::string> qualified_names;
    trace("namespace-snapshot");
    for(const auto& key:s->keys) {
        auto entry=snapshot(AssetKey(key));
        const auto collect=[&](const MountedResource& row) {
            if(const auto h=std::get_if<AssetHandle>(&row.asset);
               h && h->candidate().provenance.kind==SourceKind::loose)
                qualified_names.emplace(h->candidate().normalized_name,h->candidate().original_name);
        };
        collect(entry.winner);for(const auto& row:entry.all) collect(row);
        s->names.emplace(normalize_asset_name(key),std::move(entry));
    }
    for(const auto& [name,key]:qualified_names) {
        if(normalized.insert(name).second) {
            need(normalized.size()<=budget.indexed_names,"Civis qualified namespace exceeds indexed-name budget");
            // Resolve the new genuine spelling through the same physical
            // table; copying a source-local result would change FPK lookup.
            s->names.emplace(name,snapshot(AssetKey(key)));s->keys.push_back(key);
        }
    }
    std::sort(s->keys.begin(),s->keys.end(),[](const auto& left,const auto& right) {
        const auto l=normalize_asset_name(left),r=normalize_asset_name(right);
        return l==r?left<right:l<r;
    });
    for(const auto& key:s->keys) {
        const auto basename=resource_basename(key);
        cooperate();
        if(s->loose_fallback.contains(basename)) continue;
        // Any unknown full path cannot match a captured FPK key. Produce its
        // genuine loose-basename fallback using a provably absent FPK spelling.
        std::uint64_t ordinal=0;std::string absent;
        do {absent="_civis_absent_"+std::to_string(ordinal++)+"/"+basename;}
        while(normalized.contains(normalize_asset_name(absent)));
        s->loose_fallback.emplace(basename,snapshot(AssetKey(absent)));
    }
    // No physical resolver survives publication: preserve actual ordered
    // winner/MISS/ambiguity/all-hit recipes, then release all index callbacks.
    // Retain the actual port table/source controls before releasing physical.
    // This does not bind Native Package Default/FIRST/App or issue Lua names.
    const auto custody_current=[&]{s->pure(true);input_post();s->source_boundary();s->pure(true);};
    custody_current();
    auto retained_port_table=s->physical->resources;custody_current();
    auto port_custody=std::make_shared<const detail::CivisPortNamespaceCustody>(std::move(retained_port_table),custody_current);
    custody_current();port_custody->require_same_table(s->physical->resources,custody_current);
    s->port_namespace_custody=std::move(port_custody);custody_current();
    // Execute the existing source-owned Default/Common/categories/Skin actors
    // from this SAME captured input. OWN port tables never become native authority.
    trace("native-include");cooperate();
    auto native_include_source=ui::BnwNativePackageDiscoveryOwner::capture_bank_context_lua_include_source_owned(
        input_owner,custody_current);custody_current();
    native_include_source->require_current();custody_current();
    s->context_native_include_source=std::move(native_include_source);custody_current();
    s->physical.reset();
    for(const auto& [path,pin]:s->pins) {
        const auto& q=pin->stamp;
        identity+="source "+path_utf8(path)+" "+std::to_string(q.dwVolumeSerialNumber)+" "+
            std::to_string(q.nFileIndexHigh)+" "+std::to_string(q.nFileIndexLow)+" "+
            std::to_string(q.nFileSizeHigh)+" "+std::to_string(q.nFileSizeLow)+" "+
            std::to_string(q.ftLastWriteTime.dwHighDateTime)+" "+
            std::to_string(q.ftLastWriteTime.dwLowDateTime)+"\n";
    }
    s->recipe_identity=runtime::identify_bytes(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(identity.data()),identity.size())).sha256;
    std::set<std::string> required_names;
    op.finish();s->active=false;
    trace("required-resources");
    for(const auto& item:required) {
        need(required_names.insert(normalize_asset_name(item.name)).second,"Civis required resource occurs twice");
        owner->load_selected(AssetKey(item.name),item.parse,item.selection);
    }
    const auto weak=std::weak_ptr<CivisResourceBankState>(s);
    s->memory=std::make_unique<ResourceMountTable>(
        [weak](const AssetKey& key){return memory_lookup(weak,key);},
        [weak](const AssetKey& key){return memory_lookup_all(weak,key);},
        [weak](std::string_view prefix) {
            auto state=weak.lock();need(state && state->published,"Civis namespace retired");state->pure();
            const auto normalized_prefix=normalize_asset_name(std::string(prefix));
            std::vector<std::string> out;
            for(const auto& key:state->keys) {
                const auto n=normalize_asset_name(key);
                if(normalized_prefix.empty() || n==normalized_prefix || n.starts_with(normalized_prefix+'/'))
                    out.push_back(key);
            }
            return out;
        });
    {detail::Operation final(s);s->full_namespace_for_publication();input_post();s->published=true;final.finish();}
    return owner;
}
void CivisResourceBankOwner::load(const AssetKey& key,CivisResourceParse parse) {
    load_selected(key,parse,CivisResourceSelection::winner);
}
void CivisResourceBankOwner::load_all(const AssetKey& key,CivisResourceParse parse) {
    load_selected(key,parse,CivisResourceSelection::all_matching);
}
void CivisResourceBankOwner::load_selected(const AssetKey& key,CivisResourceParse parse,
    CivisResourceSelection selection) {
    const auto s=state_;detail::Operation op(s);
    need(parse==CivisResourceParse::bytes || parse==CivisResourceParse::xml,
        "Civis resource parse mode is invalid");
    need(selection==CivisResourceSelection::winner || selection==CivisResourceSelection::all_matching,
        "Civis resource selection mode is invalid");
    const auto entry=s->entry(key);need(entry,(std::string("Required Civis resource is missing: ")+key.original_name()).c_str());
    std::vector<const AssetHandle*> handles;
    if(selection==CivisResourceSelection::winner) {
        const auto h=std::get_if<AssetHandle>(&entry->winner.asset);
        need(h,(std::string("Required Civis winner is missing or ambiguous: ")+key.original_name()).c_str());handles.push_back(h);
    } else {
        need(!entry->all.empty(),"Required Civis all-matching resource is missing");
        for(const auto& row:entry->all) {
            const auto h=std::get_if<AssetHandle>(&row.asset);
            need(h,"Required Civis all-matching resource is ambiguous");handles.push_back(h);
        }
    }
    bool already_loaded=true;
    for(const auto h:handles) {
        const auto found=s->cache.find(physical_id(*h));
        if(found==s->cache.end() || (parse==CivisResourceParse::xml && !found->second.xml))
            already_loaded=false;
    }
    // Already materialized immutable values need only pure owner checks.
    // The consumer separately bookends its real callbacks with require_boundary.
    if(already_loaded) {op.finish();return;}
    std::map<std::string,SourcePin*> read_sources;
    std::set<fs::path> staged_named_sources;
    std::map<std::array<DWORD,3>,fs::path> staged_loaded_sources;
    for(const auto h:handles) {
        const auto id=physical_id(*h);
        if(s->cache.contains(id) || read_sources.contains(id)) continue;
        const auto pin=s->pins.find(platform::unextended_windows_path(
            fs::canonical(platform::extended_windows_path(h->file_))));
        need(pin!=s->pins.end(),"Civis asset has no captured backing source");
        read_sources.emplace(id,pin->second.get());
        staged_named_sources.insert(pin->second->path);
        staged_loaded_sources.emplace(pin->second->identity_key(),pin->second->path);
    }
    s->source_boundary(staged_named_sources);
    std::map<std::string,CivisLoadedResource> node;
    std::uint64_t total_cost=0;
    for(const auto h:handles) {
        const auto id=physical_id(*h);if(node.contains(id)) continue;
        auto existing=s->cache.find(id);
        if(existing!=s->cache.end() && (parse==CivisResourceParse::bytes || existing->second.xml)) continue;
        CivisLoadedResource staged;std::uint64_t cost=0;
        if(existing==s->cache.end()) {
            auto& held=*read_sources.at(id);held.current();
            need(h->candidate().payload_size<=s->budget.resource_bytes &&
                h->candidate().payload_size<=s->budget.resident_bytes-s->resident-total_cost,
                "Civis payload exceeds resident budget");
            auto bytes=std::make_shared<const Bytes>(held.read_range(h->offset_,h->candidate().payload_size));held.current();
            staged={h->candidate(),runtime::identify_bytes(*bytes),bytes,{}};cost=bytes->capacity();
        } else staged=existing->second;
        if(parse==CivisResourceParse::xml) {
            const auto& bytes=*staged.bytes;
            auto xml=std::make_shared<const content::XmlNode>(content::read_xml_metadata(
                std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size())));
            const auto parsed_cost=xml_cost(*xml);
            need(parsed_cost<=UINT64_MAX-cost,"Civis parse budget overflow");
            cost+=parsed_cost;staged.xml=std::move(xml);
        }
        need(cost<=s->budget.resident_bytes-s->resident-total_cost,
            "Civis parsed cache exceeds resident budget");
        total_cost+=cost;node.emplace(id,std::move(staged));
    }
    // Nodes are allocated BEFORE callbacks. No partially staged all-hit cache
    // is visible while active. A failure quarantines the entire session.
    s->source_boundary(staged_named_sources);s->pure(true);
    for(const auto& [id,value]:node) s->cache.erase(id);
    s->cache.merge(node);s->loaded_sources.merge(staged_loaded_sources);
    s->resident+=total_cost;op.finish();
}
void CivisResourceBankOwner::require_current() const {
    const auto s=state_;s->pure();need(s->published && s->memory,"Civis bank is not published");
}
const ResourceMountTable& CivisResourceBankOwner::resources() const {
    const auto s=state_;s->pure();need(s->published && s->memory,"Civis bank is not published");
    return *s->memory;
}
bool CivisResourceBankOwner::has_indexed(const AssetKey& key) const {
    const auto s=state_;s->pure();need(s->published,"Civis bank is not published");
    const auto entry=s->entry(key);
    return entry && !std::holds_alternative<std::monostate>(entry->winner.asset);
}
const std::filesystem::path& CivisResourceBankOwner::original_root_for_font_program_without_callbacks() const {
    const auto s=state_;s->pure();need(s->published,"Civis bank is not published");
    const auto& value=s->original;s->pure();return value;
}
CivisResourceBankSourceRecipe CivisResourceBankOwner::source_recipe() const {
    const auto s=state_;s->pure();need(s->published,"Civis bank is not published");
    CivisResourceBankSourceRecipe value{s->save,s->original,s->root,s->catalog,s->profile,s->mounts,s->recipe_identity};
    s->pure();return value;
}
#include "bnw_bank_context_lua_include_collection_core.inc"
runtime::BnwStockLuaClientExtent CivisResourceBankOwner::actual_client_extent() const {
    const auto s = state_;
    detail::Operation op(s);
    s->source_boundary();
    const auto extent = s->input->client_extent();
    s->source_boundary();
    op.finish();
    return extent;
}
std::uintptr_t CivisResourceBankOwner::actual_hwnd() const {
    const auto s = state_;
    detail::Operation op(s);
    s->source_boundary();
    const auto hwnd = s->input->actual_hwnd();
    s->source_boundary();
    op.finish();
    return hwnd;
}
void CivisResourceBankOwner::require_boundary() const {
    const auto s=state_;detail::Operation op(s);s->source_boundary();op.finish();
}
std::shared_ptr<const ui::BnwNativeEarlyInputLease> CivisResourceBankOwner::frontend_party_input_owned()const {
    const auto s=state_;s->pure();
    need(s->published && s->memory && s->input,"Civis frontend source input is not published");
    const auto actual=s->input->retain_owner();s->pure();
    need(actual.get()==s->input.get() && !actual.owner_before(s->input) &&
         !s->input.owner_before(actual),"Civis frontend source changed actual input control");
    s->pure();return actual;
}
std::shared_ptr<CivisResourceBankOwner> CivisResourceBankOwner::retain_owner() const {
    const auto s=state_;s->pure();
    auto owner=s->canonical_owner.lock();
    need(owner && owner.get()==this,"Civis borrowed facade is not its canonical bank");
    s->pure();return owner;
}
CivisResourceBankCapability CivisResourceBankOwner::capability() const {
    const auto s=state_;s->pure();need(s->published,"Civis bank is not published");return CivisResourceBankCapability(s);
}
const ResourceMountTable& CivisResourceBankOwner::resources(const CivisResourceBankCapability& cap) const {
    const auto s=state_;cap.require_current();s->pure();
    need(cap.state_.lock().get()==s.get() && cap.epoch_==s->epoch,"Civis capability belongs to another bank");
    return *s->memory;
}
} // namespace civ5::assets
