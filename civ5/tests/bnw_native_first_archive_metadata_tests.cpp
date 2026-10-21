#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "../src/ui/bnw_native_ui_skin_retained_occurrence.hpp"
#include "../src/ui/bnw_native_ui_skin_raw_text.hpp"
#include "../src/ui/bnw_native_common13_source.hpp"
#include "../src/ui/bnw_native_package_category2_pair.hpp"
#include "../src/ui/bnw_native_package_category_source_owner.hpp"
#include "../src/ui/bnw_native_archive_registry_source.hpp"
#include "../src/ui/bnw_native_archive_basename_cache.hpp"
#include "../src/ui/bnw_native_global_resource_warm_owner.hpp"
#include <type_traits>
#include <iostream>
#include <thread>
#include <cstring>
#include <map>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Identity comparisons deliberately provide no public currentness bypass.
namespace archive_identity_api_tests {
using namespace civ5::ui::detail;
template<class T>concept PublicJournalHeaderIdentity=requires(const T& source,
    const BnwNativeColdArchiveRequest& request,const BnwNativeArchiveConstruction& construction) {
    source.require_primary32_source_identities_without_callbacks(request,construction);
};
template<class T>concept PublicJournalPublicationIdentity=requires(const T& source,
    const BnwNativeArchiveConstruction& construction) {
    source.require_primary32_construction_identity_without_callbacks(construction);
};
template<class T>concept PublicJournalCachedIdentity=requires(const T& source,const civ5::assets::FpkSource* payload) {
    source.require_primary32_cached_fields_identity_without_callbacks(std::int64_t{},payload);
};
template<class T>concept PublicColdLeaseFields=requires(const T& source,const std::filesystem::path& root) {
    source.require_cold_roots_profile_identity_without_callbacks(root,root);
};
template<class T>concept PublicOwnedCategoryFactory=requires(const T& source,
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,const BnwNativeGlobalResourceWarmOwner& warm,
    const civ5::ui::BnwNativePackageDiscoveryOwner& parent) {
    source.request_from_owned_category(channel,warm,parent);
};
template<class T>concept PublicOwnedCategoryLookup=requires(const T& source,
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,
    const BnwNativeGlobalResourceWarmOwner& warm,const civ5::ui::BnwNativePackageDiscoveryOwner& parent) {
    source.lookup_owned_category_before_constructor(request,channel,warm,parent);
};
template<class T>concept PublicOwnedCategoryHit=requires(const T& source,const BnwNativeArchiveBasenameCacheOwner& cache,
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,
    const BnwNativeGlobalResourceWarmOwner& warm,const civ5::ui::BnwNativePackageDiscoveryOwner& parent) {
    source.hit_owned_category(cache,request,channel,warm,parent);
};
template<class T>concept PublicValidatedConstructionPath=requires(const T& source) {
    source.direct_original_path_candidate_after_validation_without_callbacks();
};
template<class T>concept PublicOwnedCategoryCommonScope=requires(const T& warm,
    const civ5::ui::BnwNativePackageDiscoveryOwner& parent,const BnwNativeColdFilesystemOwner& cold,
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,const std::function<void()>& body) {
    warm.with_owned_archive_factory_common_scope(parent,cold,channel,body,body);
};
static_assert(!PublicValidatedConstructionPath<BnwNativeArchiveConstruction>);
static_assert(!PublicOwnedCategoryCommonScope<BnwNativeGlobalResourceWarmOwner>);
static_assert(!PublicOwnedCategoryFactory<BnwNativeColdFilesystemOwner>);
static_assert(!PublicOwnedCategoryLookup<BnwNativeArchiveBasenameCacheOwner>);
static_assert(!PublicOwnedCategoryHit<BnwNativeArchiveCacheLookupOwner>);
static_assert(!PublicColdLeaseFields<civ5::ui::BnwNativeEarlyInputLease>);
static_assert(!PublicJournalHeaderIdentity<BnwNativeArchiveV6StreamHeaderOwner>);
static_assert(!PublicJournalPublicationIdentity<BnwNativeArchiveCachePublicationOwner>);
static_assert(!PublicJournalCachedIdentity<BnwNativeArchiveConstruction>);
}

using TextInput = civ5::ui::detail::BnwNativeUiSkinSelectedSourceOwner;
using CompletionPin = civ5::ui::detail::BnwNativeCategoryCompletionParentPin;
static_assert(!std::is_default_constructible_v<CompletionPin> && !std::is_copy_constructible_v<CompletionPin> &&
              !std::is_move_constructible_v<CompletionPin>);
static_assert(!std::is_constructible_v<CompletionPin,
    const std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner>&,
    const civ5::ui::BnwNativePackagePreSkinOwner&>);
static_assert(!std::is_default_constructible_v<TextInput> && !std::is_copy_constructible_v<TextInput>);
template<class T> concept PublicTextInputRead = requires(const T& input) { input.current_diagnostic(); };
static_assert(!PublicTextInputRead<TextInput>);
using SkinInput = civ5::ui::detail::BnwNativeUiSkinRetainedOccurrence;
static_assert(!std::is_default_constructible_v<SkinInput> && !std::is_copy_constructible_v<SkinInput>);
template<class T> concept PublicSkinInputIssuer = requires(const T& parent) { parent.skin_inputs(); };
static_assert(!PublicSkinInputIssuer<civ5::ui::BnwNativePackageDiscoveryOwner>);
template<class T> concept PublicSkinNodeRead = requires(const T& token) { token.borrowed_node(); };
static_assert(!PublicSkinNodeRead<SkinInput>);
static_assert(!std::is_constructible_v<SkinInput,
    std::weak_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner>,
    std::shared_ptr<const civ5::ui::BnwNativeEarlyInputLease>,
    std::shared_ptr<const void>, std::shared_ptr<const void>,
    std::size_t, std::size_t, std::size_t, const civ5::content::XmlNode&>);
using Pair = civ5::ui::detail::BnwNativePackageCategory2Pair;
using PairRead = civ5::ui::detail::BnwNativePackageCategory2ConstructorRead;
template<class T> concept PublicPairIssuer = requires(const T& parent,
    const std::shared_ptr<const T>& p) { parent.lend_native_package_category2_constructor_reads(p); };
static_assert(!std::is_default_constructible_v<Pair> && !std::is_copy_constructible_v<Pair>);
static_assert(!std::is_default_constructible_v<PairRead> && !std::is_copy_constructible_v<PairRead>);
static_assert(!PublicPairIssuer<civ5::ui::BnwNativePackageDiscoveryOwner>);
static_assert(civ5::ui::detail::native_package_category2_pair_detail::constructor_zero({0,0}));
static_assert(!civ5::ui::detail::native_package_category2_pair_detail::constructor_zero({1,0}));
static_assert(!civ5::ui::detail::native_package_category2_pair_detail::constructor_zero({0,0xffffffff}));
namespace {
void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
template<class F> void refuses(F&& fn) {
    bool refused = false;
    try { fn(); } catch (const std::exception&) { refused = true; }
    need(refused, "Native first archive accepted unavailable/replayed/stale source");
}
struct Window {
    HWND h = CreateWindowExW(0,L"STATIC",L"Civis archive CPU gate",WS_OVERLAPPEDWINDOW,
        0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    ~Window() { if(h) DestroyWindow(h); }
};
}
// Existing regression TU only: genuine factory inputs, no returned source authority.
namespace civ5::ui::detail {
struct BnwNativeFirstArchiveColdRegression final {
    static void run(const char* save,const char* game,const char* payload,HWND hwnd,
        const runtime::BnwStockLuaClientExtentProvider& base_client,
        const std::shared_ptr<const BnwNativeEarlyInputLease>& distinct_input) {
        const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
        RECT original{};check(GetWindowRect(hwnd,&original)!=FALSE,"Cold fixture cannot read actual HWND");
        const auto width=original.right-original.left,height=original.bottom-original.top;
        bool resize=false,actual_resize=false;std::size_t provider_calls=0;
        auto callback_client=base_client;
        callback_client.current=[&]{
            ++provider_calls;const auto sampled=base_client.current();
            if(resize){
                resize=false;
                check(SetWindowPos(hwnd,nullptr,0,0,width+37,height+19,
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE,"Cold fixture actual HWND resize failed");
                RECT changed{};
                actual_resize=sampled&&GetClientRect(hwnd,&changed)!=FALSE&&
                    (changed.right-changed.left!=sampled->width||changed.bottom-changed.top!=sampled->height);
            }
            return sampled; // Return the real client sample captured BEFORE SetWindowPos.
        };
        auto callback_lease=BnwNativeEarlyInputLease::acquire(save,game,payload,
            reinterpret_cast<std::uintptr_t>(hwnd),callback_client);
        const auto cold=BnwNativeColdFilesystemOwner::acquire(callback_lease);
        check(distinct_input&&callback_lease.get()!=distinct_input.get(),"Cold wrong-input control needs a distinct genuine Lease");
        const auto invoke=[&](unsigned operation){
            if(operation==0)cold->require_current();
            else if(operation==1)cold->require_same_input(*callback_lease);
            else cold->require_fresh_direct_original_branch();
        };
        for(unsigned operation=0;operation<3;++operation){
            const auto before_positive=provider_calls;invoke(operation);
            const auto positive_calls=provider_calls-before_positive;
            check(positive_calls>0,"Cold positive operation skipped the actual client provider");
            const auto before_negative=provider_calls;resize=true;actual_resize=false;std::string reason;
            try{invoke(operation);}catch(const std::exception& error){reason=error.what();}
            const auto negative_calls=provider_calls-before_negative;
            const bool witnessed=!resize&&actual_resize;
            check(SetWindowPos(hwnd,nullptr,0,0,width,height,
                SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE,"Cold fixture cannot restore actual HWND");
            check(witnessed&&negative_calls>0&&reason=="Early BNW owned HWND changed during actual client callback",
                "Cold operation accepted actual provider HWND mutation/old extent or failed for an unrelated reason");
            const auto before_recovery=provider_calls;invoke(operation);distinct_input->require_current();
            const auto recovery_calls=provider_calls-before_recovery;
            check(recovery_calls>0,"Cold normal source did not recover after restoring its HWND");
            std::cout<<"COLD_ADJACENT operation="<<(operation==0?"current":operation==1?"same_input":"fresh_direct")
                <<" positive=actual positive_client_current_calls="<<positive_calls
                <<" resized=actual old_extent_returned=actual refusal=owned_HWND_POST recovered=actual"
                <<" negative_client_current_calls="<<negative_calls<<" recovery_client_current_calls="<<recovery_calls<<'\n';
        }
        const auto before_wrong=provider_calls;std::string wrong_reason;
        try{cold->require_same_input(*distinct_input);}catch(const std::exception& error){wrong_reason=error.what();}
        check(wrong_reason=="Cold filesystem belongs to another EARLY lease"&&provider_calls>before_wrong,
            "Cold accepted a distinct genuine Lease or skipped its original source validation");
        cold->require_current();callback_lease->require_current();distinct_input->require_current();
        std::cout<<"COLD_ADJACENT wrong_distinct_genuine_lease=exact actual_factory=qualified no_source_write=1\n";
    }
};
}

void text_kernel_cases() {
    using civ5::ui::native_ui_skin_text_detail::retain_runs;
    using civ5::ui::native_ui_skin_text_detail::getter_values;
    using civ5::ui::native_ui_skin_text_detail::transform;
    using civ5::ui::native_ui_skin_text_detail::require_captured_bytes;
    // Same size and XmlLite-normalized DOM, but different native raw bytes.
    const std::string pinned_raw="<UISkin><Skin><Directory>A\rB</Directory></Skin></UISkin>";
    const std::string swapped_raw="<UISkin><Skin><Directory>A\nB</Directory></Skin></UISkin>";
    const auto pinned_dom=civ5::content::read_xml_metadata(pinned_raw);
    const auto swapped_dom=civ5::content::read_xml_metadata(swapped_raw);
    need(pinned_raw.size()==swapped_raw.size() &&
         pinned_dom.children.at(0).children.at(0).text==swapped_dom.children.at(0).children.at(0).text,
         "Captured-buffer regression did not reach same-size normalized DOM collision");
    const auto witness=civ5::runtime::identify_bytes(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(pinned_raw.data()),pinned_raw.size()));
    require_captured_bytes(pinned_raw,witness);
    refuses([&]{require_captured_bytes(swapped_raw,witness);});
    refuses([&]{require_captured_bytes(pinned_raw+" ",witness);});
    const auto pinned_runs=retain_runs(pinned_dom,pinned_raw);
    const auto swapped_runs=retain_runs(swapped_dom,swapped_raw);
    need(getter_values(pinned_dom,pinned_runs,"Skin","Directory")==std::vector<std::string>{"A\rB"} &&
         getter_values(swapped_dom,swapped_runs,"Skin","Directory")==std::vector<std::string>{"A\nB"},
         "Raw CR/LF mismatch stopped distinguishing actual native text bytes");
    const std::string raw = "<UISkin><GameplaySkin><Directory>  A\r\nB &amp; &#65; &lt; &#x42;  <!--gap-->later<![CDATA[ignored]]></Directory>"
        "<Paks><File>one</File><File>&#xE9;</File><File></File><File>three</File></Paks></GameplaySkin>"
        "<Skin><Directory><![CDATA[ignored]]></Directory><File><Other>x</Other></File></Skin>"
        "<GameplaySkin><Directory>secondslot</Directory></GameplaySkin></UISkin>";
    const auto dom = civ5::content::read_xml_metadata(raw);
    const auto lexical = retain_runs(dom,raw);
    need(getter_values(dom,lexical,"GameplaySkin","Directory")==std::vector<std::string>{"A\r\nB & A < B"},
         "Raw text lost CRLF/first scalar/trim/exact entities or chose repeated slot");
    need(getter_values(dom,lexical,"GameplaySkin","Paks")==std::vector<std::string>{"one","one","","three"},
         "Native File text getter failure did not retain previous scratch or empty did not clear it");
    need(getter_values(dom,lexical,"Skin","Directory")==std::vector<std::string>{""} &&
         getter_values(dom,lexical,"Skin","File").empty(),"CDATA/scalar versus child File count differs");
    need(getter_values(dom,lexical,"gameplayskin","Directory").empty(),"Native matching invented case folding");
    const auto lock_dom=civ5::content::read_xml_metadata("<UISkin><Skin><Directory>&#32;<!--gap-->later</Directory></Skin></UISkin>");
    const auto lock_runs=retain_runs(lock_dom,"<UISkin><Skin><Directory>&#32;<!--gap-->later</Directory></Skin></UISkin>");
    need(getter_values(lock_dom,lock_runs,"Skin","Directory")==std::vector<std::string>{""},
         "Native first-buffer-byte lock was replaced by trimmed-length test");
    need(transform("&#A;")==std::string(1,'\n') && transform("&#1A;")==std::string(1,char(20)) &&
         transform("&#4294967297;")==std::string(1,char(1)) &&
         transform("&#xD800;")==std::string("\xed\xa0\x80",3) && transform("&#0;")==std::string(1,'\0'),
         "Native decimal shared digit map/DWORD wrapping/numeric byte emitter differs");
    need(transform("&AMP; &unknown;")=="&AMP; &unknown;","Native unknown entities were normalized");
    need(transform("&#;")==std::string(1,'\0') && transform("&#x;")==std::string(1,'\0'),
         "Native numeric parser invented a digit-presence requirement");
    refuses([&]{(void)transform("&#x110000;");});
    refuses([&]{(void)transform("&#65");});
    refuses([&]{(void)transform("&#xD800");});
    refuses([&]{(void)transform("&#X41;");});
    const std::string empty_first="<UISkin><Skin><Directory>   <!--gap--><?pi ignored?><![CDATA[ignored]]>later</Directory>"
        "<Directory>next</Directory></Skin></UISkin>";
    const auto empty_dom=civ5::content::read_xml_metadata(empty_first);
    const auto empty_runs=retain_runs(empty_dom,empty_first);
    need(getter_values(empty_dom,empty_runs,"Skin","Directory")==std::vector<std::string>{"later","next"},
         "Empty literal run/CDATA/PI locked value or terminal sibling order changed");
    auto foreign=dom; refuses([&]{(void)retain_runs(foreign,"<Foreign/>");});
}
using CategorySource=civ5::ui::BnwNativePackageCategorySourceOwner;
using NamespaceChannel=civ5::ui::detail::BnwNativeCategoryNamespaceChannel;
using ArchiveChannel=civ5::ui::detail::BnwNativeCategoryArchiveChannel;
static_assert(!std::is_default_constructible_v<CategorySource> && !std::is_copy_constructible_v<CategorySource>);
static_assert(!std::is_default_constructible_v<NamespaceChannel> && !std::is_copy_constructible_v<NamespaceChannel>);
static_assert(!std::is_default_constructible_v<ArchiveChannel> && !std::is_copy_constructible_v<ArchiveChannel>);
template<class T> concept PublicCategoryRecordWriter=requires(const T& value){value.record_index();};
template<class T> concept PublicCategoryArenaIssuer=requires(const T& value){value.claim_initial_arena();};
template<class T> concept PublicCategoryCacheWriter=requires(const T& value){value.cache_storage();};
static_assert(!PublicCategoryRecordWriter<NamespaceChannel> && !PublicCategoryArenaIssuer<NamespaceChannel> &&
              !PublicCategoryCacheWriter<NamespaceChannel>);
void category_raw_parser_cases() {
    using civ5::ui::detail::native_category_xml_filter_ascii;
    using civ5::ui::detail::native_category_matched_filter_mode;
    need(native_category_xml_filter_ascii("A.XML") && native_category_xml_filter_ascii(".xml") &&
         native_category_xml_filter_ascii("a.GgXmL") && native_category_xml_filter_ascii("part;two.xml") &&
         !native_category_xml_filter_ascii("a.xml.backup") && !native_category_xml_filter_ascii("a.dds") &&
         native_category_matched_filter_mode(7)==2 && native_category_matched_filter_mode(1)==2,
         "Actual retained native XML wildcard/filter flags differ (XML7 must become2)");
    refuses([&]{(void)native_category_xml_filter_ascii("path/a.xml");});
    refuses([&]{(void)native_category_xml_filter_ascii(std::string("a\0.xml",6));});
    using civ5::ui::native_category_source_detail::category_values;
    using civ5::ui::native_ui_skin_text_detail::retain_runs;
    const std::string raw="<Civ5Package><Common><Directory>first</Directory>"
        "<Paks><File>one.fpk</File><File>&#xE9;</File><File></File><File>last.fpk</File></Paks>"
        "<Directory><![CDATA[ignored]]></Directory></Common><common><Directory>wrongcase</Directory></common>"
        "<Common><Directory>secondslot</Directory></Common><Directory>root-after</Directory>"
        "<Gameplay><Directory>game</Directory></Gameplay><Gameplay><Directory>ignored</Directory></Gameplay></Civ5Package>";
    const auto dom=civ5::content::read_xml_metadata(raw);const auto runs=retain_runs(dom,raw);
    need(category_values(dom,runs,0,"Directory")==std::vector<std::string>{"first","","root-after"},
         "Category Common first node/raw CDATA/root-after order differs");
    need(category_values(dom,runs,1,"Directory")==std::vector<std::string>{"game"},
         "Category Gameplay first node or Common-only root fallback differs");
    need(category_values(dom,runs,0,"Paks")==std::vector<std::string>{"one.fpk","one.fpk","","last.fpk"},
         "Category native ASCII getter scratch/empty clearing differs");
    need(category_values(dom,runs,0,"directory").empty(),"Category selector invented case folding");
    const auto copied=dom;
    refuses([&]{(void)category_values(copied,runs,0,"Directory");});
    refuses([&]{(void)category_values(dom,runs,2,"Directory");});
    const std::string empty="<Civ5Package><Directory>outside</Directory></Civ5Package>";
    const auto empty_dom=civ5::content::read_xml_metadata(empty);const auto empty_runs=retain_runs(empty_dom,empty);
    need(category_values(empty_dom,empty_runs,0,"Directory")==std::vector<std::string>{"outside"} &&
         category_values(empty_dom,empty_runs,1,"Directory").empty(),
         "Category absent Common must retain genuine root fallback; absent Gameplay must stay empty");
}
// Private traversal regressions issue no Package, namespace, stat or completion
// authority. Scripted FindA data verifies exactly where the real OS traversal
// opens child cursors; the production factory always uses NativeEnumeration.
void category_directory_capture_cases() {
    namespace directory = civ5::ui::native_category_directory_detail;
    need(directory::normalized_query_ascii("DLC/Pack/Art")=="DLC\\Pack\\Art\\" &&
         directory::normalized_query_ascii("C:/Game/Art\\\\")=="C:\\Game\\Art\\\\" &&
         directory::normalized_query_ascii(".\\Art")==".\\Art\\",
         "Native Directory query changed relative/absolute form or collapsed repeated separators");
    refuses([&]{(void)directory::normalized_query_ascii("");});
    refuses([&]{(void)directory::normalized_query_ascii(std::string("A\0B",3));});
    const auto row=[](const std::string& name,DWORD attributes,std::uint64_t bytes=0,
                      std::uint64_t written=1) {
        WIN32_FIND_DATAA data{};
        need(name.size()<MAX_PATH,"Scripted FindA record too long");
        std::memcpy(data.cFileName,name.c_str(),name.size()+1);
        data.dwFileAttributes=attributes;
        data.nFileSizeHigh=DWORD(bytes>>32);data.nFileSizeLow=DWORD(bytes);
        data.ftLastWriteTime.dwHighDateTime=DWORD(written>>32);
        data.ftLastWriteTime.dwLowDateTime=DWORD(written);return data;
    };
    struct Enumeration {
        std::map<std::string,std::vector<WIN32_FIND_DATAA>> rows;
        std::vector<std::string> opened;
        DWORD terminal=ERROR_NO_MORE_FILES;
        unsigned live=0,closed=0;
        struct Cursor {
            Enumeration& owner;
            const std::vector<WIN32_FIND_DATAA>& rows;
            WIN32_FIND_DATAA data{};
            std::size_t index=0;
            Cursor(Enumeration& owner,const std::vector<WIN32_FIND_DATAA>& rows)
                :owner(owner),rows(rows){++owner.live;if(!rows.empty())data=rows.front();}
            Cursor(const Cursor&)=delete;
            ~Cursor(){--owner.live;++owner.closed;}
            bool valid()const{return !rows.empty();}
            bool next(){if(++index>=rows.size())return false;data=rows[index];return true;}
            DWORD error()const{return owner.terminal;}
        };
        Cursor open(const std::string& query) {
            opened.push_back(query);
            need(rows.contains(query),"Excluded child was opened or native query bytes changed");
            return Cursor(*this,rows.at(query));
        }
    };
    const auto make=[&] {
        Enumeration e;
        e.rows["IO\\*.*"]={row(".",16),row("..",16),
            row("z.bin",32,0x100000002ULL,0x200000003ULL),
            row("Scenarios",16),row("Sounds",16),row("Other",16),
            row("Otherish",16),row("a.bin",32,5,9),
            row(std::string(1,char(0xFF)),FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_REPARSE_POINT)};
        e.rows["IO\\Sounds\\*.*"]={row("Speech",16),row("keep.bin",32,3,8)};
        e.rows["IO\\Otherish\\*.*"]={row("keep.bin",32,4,10)};
        return e;
    };
    const std::vector<std::string> provider_rows{"Root\\Other"};
    auto enumeration=make();unsigned callbacks=0;
    const auto entries=directory::capture_entries("IO","Root",provider_rows,enumeration,[&]{++callbacks;});
    need(enumeration.opened==std::vector<std::string>{"IO\\*.*","IO\\Sounds\\*.*","IO\\Otherish\\*.*"} &&
         enumeration.live==0 && enumeration.closed==3 && callbacks>=8,
         "Directory exclusions must run before EACH child FindA in native recursive order");
    std::vector<std::string> raw;
    for(const auto& entry:entries)raw.push_back(entry.raw_relative);
    need(raw==std::vector<std::string>{"z.bin","Scenarios","Sounds","Sounds\\Speech",
        "Sounds\\keep.bin","Other","Otherish","Otherish\\keep.bin","a.bin"} &&
         entries.front().native_name=="z.bin" && entries.front().bytes==0x100000002ULL &&
         entries.front().written==0x200000003ULL && entries[1].attributes==16,
         "Category native capture sorted rows or lost raw filename/attributes/high stat words");
    // Whole native input controls exclusions independently of absolute I/O.
    Enumeration excluded;
    const auto none=directory::capture_entries("UNRELATED_IO","Root/Scenarios",{},excluded,[]{});
    need(none.empty() && excluded.opened.empty(),"Excluded root performed actual FindA");
    auto literal=make();literal.rows["IO\\Other\\*.*"]={row("keep.bin",32)};
    (void)directory::capture_entries("IO","Root",std::vector<std::string>{" Root\\Other"},literal,[]{});
    need(std::find(literal.opened.begin(),literal.opened.end(),"IO\\Other\\*.*")!=literal.opened.end(),
         "Native provider row was trimmed or matched a reconstructed basename");
    auto changed=make();changed.rows["IO\\Sounds\\*.*"][1].ftLastWriteTime.dwLowDateTime=99;
    need(directory::capture_entries("IO","Root",provider_rows,changed,[]{})!=entries,
         "Nested actual write-time mutation was omitted from capture equality");
    changed=make();std::swap(changed.rows["IO\\*.*"][2],changed.rows["IO\\*.*"][7]);
    need(directory::capture_entries("IO","Root",provider_rows,changed,[]{})!=entries,
         "Native order mutation was hidden by sorting");
    changed=make();changed.rows["IO\\Otherish\\*.*"][0].dwFileAttributes=FILE_ATTRIBUTE_REPARSE_POINT;
    refuses([&]{(void)directory::capture_entries("IO","Root",provider_rows,changed,[]{});});
    need(changed.live==0,"Rejected nested cursor leaked an actual enumeration lifetime");
    auto incomplete=make();incomplete.terminal=ERROR_ACCESS_DENIED;
    refuses([&]{(void)directory::capture_entries("IO","Root",provider_rows,incomplete,[]{});});
    auto retired=make();unsigned checked=0;
    refuses([&]{(void)directory::capture_entries("IO","Root",provider_rows,retired,[&]{
        if(++checked==2)throw std::runtime_error("Genuine currentness callback refused before child");});});
    need(retired.opened==std::vector<std::string>{"IO\\*.*"} && retired.live==0,
         "Currentness failure opened a child or retained an enumeration cursor");
    auto final=make();bool final_checked=false;
    refuses([&]{(void)directory::capture_entries("IO","Root",provider_rows,final,[&]{
        if(!final.opened.empty() && final.live==0){final_checked=true;
            throw std::runtime_error("Genuine currentness refused after closing final cursor");}});});
    need(final_checked,"Final callback preceded destruction of the actual last cursor");
    directory::DirectoryWalk witness;witness.code_page=GetACP();witness.provider_rows=provider_rows;
    refuses([&]{(void)directory::matches(witness,std::vector<std::string>{},[]{});});
    witness.code_page^=1;
    refuses([&]{(void)directory::matches(witness,provider_rows,[]{});});
}
void common_source_literal_cases(){
    using namespace civ5::ui::detail;
    const std::array<std::string,13> expected={"BuildingModels","CityModels","EffectData","FeatureModels",
        "HutsBarbarianFortModels","ImprovementModels","LeaderModels","MiscData","SoundData","TempModels",
        "TerrainModels","UnitModels","WonderModels"};
    for(std::size_t n=0;n<expected.size();++n)need(bnw_common13_raw_request(n)==
        "Resource\\Common\\"+expected[n]+".fpk","Common authentic literal/order changed");
    refuses([]{(void)bnw_common13_raw_request(13);});
    static_assert(!std::is_default_constructible_v<BnwNativeCommon13ArchiveChannel>);
    static_assert(!std::is_copy_constructible_v<BnwNativeCommon13ArchiveChannel>);
}
void genuine_common_source_cases(const std::shared_ptr<const civ5::ui::BnwNativeEarlyInputLease>& lease,
    std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner>& callback_owner,
    bool& reenter_pair,bool& pair_reentry_caught,bool& release_owner,bool drop_last){
    using namespace civ5::ui;
    auto package=BnwNativePackageDiscoveryOwner::create(lease);const auto* table=&package->resources();
    refuses([&]{detail::BnwNativeDefaultCommonProduction::categories(package);});
    refuses([&]{detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(package);});
    need(!package->current().common_high_quality_registration_complete,"Early failure fabricated Common completion");
    bool foreign_primary_refused=false;
    std::thread foreign_primary([&]{try{detail::BnwNativeDefaultCommonProduction::primary(package);}
        catch(const std::exception&){foreign_primary_refused=true;}});foreign_primary.join();
    need(foreign_primary_refused && !package->current().primary_dx9_registration_complete,
         "Foreign Primary attempt consumed or poisoned creating-thread producer");
    detail::BnwNativeDefaultCommonProduction::primary(package);
    need(package->current().primary_dx9_registration_complete && &package->resources()==table &&
         package->early_input_lease().get()==lease.get(),"Default publication changed SAME Package/table/lease");
    auto settings=BnwNativeResourceBootSettingsOwner::create(package);
    package->register_native_dx9_texture_quality_branch(std::move(settings));
    bool foreign_refused=false;
    std::thread foreign([&]{try{package->register_native_common_high_quality_prefix();}
        catch(const std::exception&){foreign_refused=true;}});foreign.join();
    need(foreign_refused && package->current().low_resolution_source_branch_complete,
         "Foreign Common attempt mutated/poisoned creating-thread Package");
    package->register_native_common_high_quality_prefix();
    const auto& frame=package->current();
    need(frame.common_high_quality_registration_complete && &package->resources()==table,
         "Actual Common producer did not complete SAME table");
    const auto& order=frame.resource_registration_order;
    need(order.size()==48,"Actual Primary32/Common13/loose3 order differs");
    need(order[32]=="Resource/Common/BuildingModels.fpk" && order[44]=="Resource/Common/WonderModels.fpk" &&
         order[45]=="Resource/Overrides" && order[46]=="Assets" && order[47]=="Assets/Sounds/Speech/Shared",
         "Common source order was replaced by count acknowledgement");
    detail::BnwNativeDefaultCommonProduction::categories(package);
    package->require_native_package_category_sources_complete();
    refuses([&]{detail::BnwNativeDefaultCommonProduction::primary(package);});
    need(package->current().common_high_quality_registration_complete,
         "Completed duplicate Primary poisoned successful same-thread producer");
    refuses([&]{package->register_native_common_high_quality_prefix();});
    // A real sealed installed Package, not a synthetic descriptor/absence DTO.
    detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(package);
    detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(package);
    bool foreign_pair_refused=false;
    std::thread foreign_pair([&]{try{detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(package);}
        catch(const std::exception&){foreign_pair_refused=true;}});foreign_pair.join();
    need(foreign_pair_refused,"Foreign pair read was admitted");
    detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(package);
    // Native NULL query remains valid after an independent successful pair read.
    package->require_native_package_category_sources_complete();
    callback_owner=std::move(package); // exactly the last external Package facade
    if(drop_last){
        release_owner=true;
        refuses([&]{detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(callback_owner);});
        release_owner=false;
        need(!callback_owner,"Actual lease callback did not release the last external Package");
    }else{
        reenter_pair=true;pair_reentry_caught=false;
        refuses([&]{detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(callback_owner);});
        reenter_pair=false;
        need(pair_reentry_caught,"Actual installed lease did not catch repeated active pair read");
        refuses([&]{detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(callback_owner);});
        callback_owner.reset();
    }
}
void genuine_primary_default_case(const std::shared_ptr<const civ5::ui::BnwNativeEarlyInputLease>& lease) {
    using namespace civ5::ui;
    const auto package=BnwNativePackageDiscoveryOwner::create(lease);
    const auto* table=&package->resources();
    bool foreign_refused=false;
    std::thread foreign([&]{try{detail::BnwNativeDefaultCommonProduction::primary(package);}
        catch(const std::exception&){foreign_refused=true;}});
    foreign.join();
    need(foreign_refused&&!package->current().primary_dx9_registration_complete,
         "Foreign Primary consumed the genuine creating-thread attempt");
    detail::BnwNativeDefaultCommonProduction::primary(package);
    const auto& frame=package->current();
    need(frame.primary_dx9_registration_complete&&frame.archives_registered==32&&
         frame.default_indexed_names==4882&&frame.resource_registration_order.size()==32&&
         frame.resource_registration_order.front()=="Resource/DX9/AmericanWashingtonTextures.fpk"&&
         frame.resource_registration_order.back()=="Resource/DX9/WonderTextures.fpk",
         "Primary Default publication differs from the actual source records/order");
    need(&package->resources()==table&&package->early_input_lease().get()==lease.get()&&
         !frame.low_resolution_source_branch_complete&&!frame.common_high_quality_registration_complete&&
         !frame.native_skin_loading_complete,
         "Primary Default replaced its table/lease or fabricated later readiness");
    refuses([&]{detail::BnwNativeDefaultCommonProduction::primary(package);});
    need(package->current().primary_dx9_registration_complete && &package->resources()==table,
         "Duplicate Primary poisoned the completed genuine Default publication");
    std::cout<<"PASS genuine primary Default publication: stock32/4882, SAME table/lease, source order, "
        "foreign/replay refused; Common/Skin and full game remain unverified\n"<<std::flush;
}
int main(int argc, char** argv) {
    try {
        need(argc == 4 || (argc == 5 && (std::string_view(argv[4]) == "--first-archive-only" ||
             std::string_view(argv[4]) == "--primary-default-only")),
             "Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS [--first-archive-only|--primary-default-only]");
        const bool first_archive_only = argc == 5 && std::string_view(argv[4]) == "--first-archive-only";
        const bool primary_default_only = argc == 5 && std::string_view(argv[4]) == "--primary-default-only";
        if (first_archive_only)
            std::cout << "SUBSET first-archive-only: full Primary32/Common cases excluded; not full regression acceptance\n";
        text_kernel_cases();category_raw_parser_cases();category_directory_capture_cases();common_source_literal_cases();
        Window window; need(window.h && !IsWindowVisible(window.h), "Actual hidden owned HWND missing");
        std::uint64_t revision = 1;
        bool reenter = false, release_owner = false, reenter_text = false;
        bool reenter_primary=false,primary_reentry_caught=false;
        bool reenter_pair=false,pair_reentry_caught=false;
        std::uint64_t admission_callback = 0, reenter_at_admission_callback = 0;
        std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> callback_owner;
        auto client = civ5::runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(window.h), "first-archive-metadata", [&] {
                if (reenter_at_admission_callback &&
                    ++admission_callback == reenter_at_admission_callback && callback_owner) {
                    try { (void)callback_owner->current(); } catch (const std::exception&) {}
                }
                if(reenter_primary && callback_owner){
                    bool actual_partial=false;
                    try{civ5::ui::detail::BnwNativeDefaultCommonProduction::require_active_primary_partial_effects(*callback_owner);
                        actual_partial=true;}catch(const std::exception&){}
                    if(actual_partial){
                        reenter_primary=false; // one genuine attack; not a completion/count input
                        try{civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);}
                        catch(const std::exception&){primary_reentry_caught=true;}
                    }
                }
                if(reenter_pair && callback_owner){
                    reenter_pair=false;
                    try{civ5::ui::detail::BnwNativePackageCategory2PairOperation::require_owned_constructor_reads(callback_owner);}
                    catch(const std::exception&){pair_reentry_caught=true;}
                }
                if (reenter_text && callback_owner) {
                    try { (void)callback_owner->inspect_native_ui_skin_retained_text_inputs(); }
                    catch (const std::exception&) {}
                }
                if (release_owner) callback_owner.reset();
                if (reenter && callback_owner) {
                    try { (void)callback_owner->current(); } catch (const std::exception&) {}
                }
                return revision;
            });
        auto lease = civ5::ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],
            reinterpret_cast<std::uintptr_t>(window.h),client);
        if (primary_default_only) {
            std::cout<<"SUBSET primary-default-only: actual Default publication; later Common/category cases excluded\n"<<std::flush;
            genuine_primary_default_case(lease);
            return 0;
        }
        civ5::ui::detail::BnwNativeFirstArchiveColdRegression::run(
            argv[1],argv[2],argv[3],window.h,client,lease);
        // The default invocation retains every full Primary32/Common case.
        // The explicit smoke mode exercises the genuine ordinal0 path and its
        // lifetime/reentry checks below without repeating complete startup.
        if (!first_archive_only) {
        genuine_common_source_cases(lease,callback_owner,reenter_pair,pair_reentry_caught,release_owner,false);
        genuine_common_source_cases(lease,callback_owner,reenter_pair,pair_reentry_caught,release_owner,true);
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        reenter_primary=true;primary_reentry_caught=false;
        refuses([&]{civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);});
        reenter_primary=false;
        need(primary_reentry_caught,"Private Primary recursion was not caught inside actual installed-lease callback");
        civ5::ui::detail::BnwNativeDefaultCommonProduction::require_failed_primary_quarantine(*callback_owner);
        refuses([&]{(void)callback_owner->current();});
        refuses([&]{civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);});
        civ5::ui::detail::BnwNativeDefaultCommonProduction::require_failed_primary_quarantine(*callback_owner);
        callback_owner.reset();
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);
        callback_owner->register_native_dx9_texture_quality_branch(civ5::ui::BnwNativeResourceBootSettingsOwner::create(callback_owner));
        reenter=true;refuses([&]{callback_owner->register_native_common_high_quality_prefix();});
        reenter=false;refuses([&]{(void)callback_owner->current();});callback_owner.reset();
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);
        callback_owner->register_native_dx9_texture_quality_branch(civ5::ui::BnwNativeResourceBootSettingsOwner::create(callback_owner));
        admission_callback=0;reenter_at_admission_callback=6;
        refuses([&]{callback_owner->register_native_common_high_quality_prefix();});
        reenter_at_admission_callback=0;need(admission_callback>=6,"Common late callback witness not exercised");
        refuses([&]{(void)callback_owner->current();});callback_owner.reset();
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        civ5::ui::detail::BnwNativeDefaultCommonProduction::primary(callback_owner);
        callback_owner->register_native_dx9_texture_quality_branch(civ5::ui::BnwNativeResourceBootSettingsOwner::create(callback_owner));
        const auto* common_last=callback_owner.get();release_owner=true;
        refuses([&]{common_last->register_native_common_high_quality_prefix();});release_owner=false;
        need(!callback_owner,"Common source did not exercise actual final outside Package release");
        }
        auto package = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const auto* table = &package->resources();
        // Genuine EARLY Package cannot issue a completion from null caller inputs.
        refuses([&]{package->prepare_native_package_category_sources({},{});});
        refuses([&]{package->require_native_package_category_sources_complete();});
        need(&package->resources()==table && table->mounts().empty(),
             "Unavailable category provider fabricated/changed SAME table publication");
        // Real Package + real lease/client callback; these three references
        // model the precisely identified inner/fence/outer lifetime bookend.
        // This predicate test issues NO completion token/namespace/provider.
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const std::weak_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> completion_weak=callback_owner;
        {
            const auto outer_pre_skin_pin=callback_owner;
            const auto inner_source_pin=callback_owner;
            civ5::ui::detail::BnwNativeCallbackOwnerState lifetime_state;
            civ5::ui::detail::BnwNativeCallbackOwnerGuard<civ5::ui::BnwNativePackageDiscoveryOwner>
                lifetime_fence(lifetime_state,completion_weak);
            need(civ5::ui::native_category_source_detail::completion_borrow_has_external_parent(
                inner_source_pin,outer_pre_skin_pin,lifetime_fence),"Current external completion Package refused");
            release_owner=true;lease->require_current();release_owner=false;
            need(!callback_owner && lifetime_fence.valid() && inner_source_pin.use_count()==3,
                 "Actual callback did not leave exactly the three identified internal pins");
            need(!civ5::ui::native_category_source_detail::completion_borrow_has_external_parent(
                inner_source_pin,outer_pre_skin_pin,lifetime_fence),
                 "Completion lifetime admitted last external release masked by PreSkin pin");
        }
        need(completion_weak.expired(),"Completion lifetime pins leaked actual Package facade");
        const auto contexts = package->current().skin_contexts;
        need(contexts > 0, "Actual UISkin retained source unavailable");
        package->require_native_ui_skin_retained_inputs();
        const auto native_text=package->inspect_native_ui_skin_retained_text_inputs();
        need(native_text.size()==contexts*10,"SAME retained UISkin text consumer lost two-slot selector order");
        need(std::any_of(native_text.begin(),native_text.end(),[](const auto& row) {
            return (row.selector=="Directory" || row.selector=="Paks" || row.selector=="File") && !row.getter_values.empty();
        }),"Actual installed UISkin text consumer did not reach resource declarations");
        bool foreign_text_refused=false;
        std::thread foreign_text([&] {
            try {(void)package->inspect_native_ui_skin_retained_text_inputs();}
            catch (const std::exception&) {foreign_text_refused=true;}
        });
        foreign_text.join();need(foreign_text_refused,"Foreign thread read actual native raw text input");
        need(package->inspect_native_ui_skin_retained_text_inputs().size()==native_text.size(),
             "Foreign thread poisoned creating-thread raw input");
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        reenter_text=true;
        refuses([&]{(void)callback_owner->inspect_native_ui_skin_retained_text_inputs();});
        reenter_text=false;
        refuses([&]{(void)callback_owner->inspect_native_ui_skin_retained_text_inputs();});
        callback_owner.reset();
        callback_owner=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const auto* raw_last_facade=callback_owner.get();
        release_owner=true;
        refuses([&]{(void)raw_last_facade->inspect_native_ui_skin_retained_text_inputs();});
        release_owner=false;need(!callback_owner,"Raw consumer did not exercise last outside Package release");
        need(package->current().skin_contexts == contexts && !package->current().native_skin_loading_complete &&
             &package->resources() == table && package->resources().mounts().empty() && package->resources().keys().empty(),
             "Retained UISkin token fabricated constructor/load/namespace publication");
        const auto channels = package->color_provider().current();
        need(channels && channels->contexts.size() == contexts, "Actual retained UISkin channel binding absent");
        for (const auto& channel : channels->contexts)
            for (const auto& enabled : channel.enabled)
                need(!enabled.has_value(), "Retained input invented native slot enabled state");
        bool foreign_skin_refused = false;
        std::thread foreign_skin([&] {
            try { package->require_native_ui_skin_retained_inputs(); }
            catch (const std::exception&) { foreign_skin_refused = true; }
        });
        foreign_skin.join(); need(foreign_skin_refused, "Foreign thread read retained UISkin input");
        package->require_native_ui_skin_retained_inputs();
        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        reenter = true;
        refuses([&] { callback_owner->require_native_ui_skin_retained_inputs(); });
        reenter = false;
        refuses([&] { callback_owner->require_native_ui_skin_retained_inputs(); });
        callback_owner.reset();
        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const auto* skin_last_facade = callback_owner.get();
        release_owner = true;
        refuses([&] { skin_last_facade->require_native_ui_skin_retained_inputs(); });
        release_owner = false;
        need(!callback_owner, "UISkin source did not exercise last Package release");
        bool foreign_refused = false;
        std::thread foreign([&] { try { (void)package->prepare_native_dx9_first_archive_metadata(); }
            catch(const std::exception&) { foreign_refused = true; } });
        foreign.join(); need(foreign_refused, "Foreign thread reached source writes");
        const auto metadata = package->prepare_native_dx9_first_archive_metadata();
        need(metadata.raw_prefix == u"Resource\\DX9\\AmericanWashingtonTextures.fpk" &&
             metadata.prefix_index == 0 && metadata.original_byte_length > 14 &&
             metadata.declared_entry_count > 0 &&
             metadata.constructor_archive_priority==1&&metadata.constructor_enabled==1&&
             metadata.constructor_prefix_filter_count==0&&
             metadata.declared_entry_count == metadata.canonical_metadata_entries,
             "Actual source ordinal/header/prefix/metadata mismatch");
        need(&package->resources() == table && package->resources().keys().empty() &&
             !package->current().primary_dx9_registration_complete &&
             package->current().archive_registration_requests == 0,
             "Metadata frontier fabricated native default/cache registration");
        refuses([&] { package->require_native_dx9_first_archive_entry_admission(); });
        const auto admitted = package->admit_native_dx9_first_archive_entries();
        need(admitted.declared_entry_count == metadata.declared_entry_count &&
             admitted.admitted_entry_count > 0 &&
             admitted.admitted_entry_count <= admitted.declared_entry_count,
             "Actual source admission loop did not accumulate its own returns");
        refuses([&] { (void)package->admit_native_dx9_first_archive_entries(); });
        refuses([&] { package->require_native_dx9_first_archive_entry_admission(); });
        need(package->resources().keys().empty() && !package->current().primary_dx9_registration_complete,
             "Owned admission count fabricated cache publication/default table readiness");
        const auto cached=package->publish_native_dx9_first_archive_cache();
        need(cached.checked_original_mtime==metadata.checked_original_mtime &&
             cached.canonical_metadata_entries==metadata.canonical_metadata_entries,
             "Actual cache publication lost original stat/canonical source identity");
        refuses([&] { (void)package->publish_native_dx9_first_archive_cache(); });
        need(package->resources().keys().empty() && !package->current().primary_dx9_registration_complete,
             "First cache store fabricated primary32/default VFS completion");
        refuses([&] { (void)package->prepare_native_dx9_first_archive_metadata(); });
        refuses([&] { package->register_native_dx9_primary_prefix(); });

        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        reenter = true;
        refuses([&] { (void)callback_owner->prepare_native_dx9_first_archive_metadata(); });
        reenter = false;
        refuses([&] { (void)callback_owner->current(); });
        callback_owner.reset();

        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        (void)callback_owner->prepare_native_dx9_first_archive_metadata();
        // Package.validate uses two lease checks, each with two revision reads.
        // The sixth revision callback therefore occurs inside the child source
        // preparation, after the outer Package validation. A caught parent
        // reentry must reach the genuine private source fence before admission.
        admission_callback = 0; reenter_at_admission_callback = 6;
        refuses([&] { (void)callback_owner->admit_native_dx9_first_archive_entries(); });
        reenter_at_admission_callback = 0;
        need(admission_callback >= 6, "Late admission callback was not exercised");
        refuses([&] { (void)callback_owner->current(); });
        callback_owner.reset();

        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        (void)callback_owner->prepare_native_dx9_first_archive_metadata();
        (void)callback_owner->admit_native_dx9_first_archive_entries();
        admission_callback=0;reenter_at_admission_callback=6;
        refuses([&] { (void)callback_owner->publish_native_dx9_first_archive_cache(); });
        reenter_at_admission_callback=0;
        need(admission_callback>=6,"Late cache-publication child callback was not exercised");
        refuses([&] { (void)callback_owner->current(); });
        callback_owner.reset();

        callback_owner = civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const auto* last_facade = callback_owner.get();
        release_owner = true;
        refuses([&] { (void)last_facade->prepare_native_dx9_first_archive_metadata(); });
        release_owner = false;
        need(!callback_owner, "Last external facade callback was not exercised");
        ++revision;
        refuses([&] {(void)package->inspect_native_ui_skin_retained_text_inputs();});
        refuses([&] { package->require_native_ui_skin_retained_inputs(); });
        refuses([&] { package->require_native_dx9_first_archive_entry_admission(); });
        std::cout << "PASS genuine ordinal0 stat -> v6 header -> shared prefix0 -> canonical metadata "
                  << metadata.canonical_metadata_entries
                  << " admitted=" << admitted.admitted_entry_count
                  << "; foreign/replay/reentry/last-facade/stale refused; cache publication remains unavailable\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
