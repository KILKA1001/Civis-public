#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/runtime/detail/bnw_worldview_canvas_xml_metadata.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/runtime/detail/bnw_context_lua_profile_config_loan.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <limits>
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include <xmmintrin.h>
#include "../ui/bnw_native_color_load_kernel.hpp"
#include <set>
#include <array>
#include <atomic>
#include <fstream>
#include <iterator>
#include <utility>
namespace civ5::runtime {
namespace {
void require(bool value,const char* why){if(!value)throw ScriptError(why);}
constexpr std::uint64_t budget=64ULL*1024*1024;
const content::XmlNode* unique(const content::XmlNode& root,std::string_view kind,
                              std::string_view attr,std::string_view name){
    const content::XmlNode* result=nullptr;
    if(root.name==kind&&root.attribute(std::string(attr))==name)result=&root;
    for(const auto& child:root.children)if(const auto* found=unique(child,kind,attr,name)){
        require(!result,"WorldView authored XML member is ambiguous");result=found;
    }
    return result;
}
const content::XmlNode& direct(const content::XmlNode& root,std::string_view kind,
                              std::string_view id){
    const content::XmlNode* result=nullptr;
    for(const auto& child:root.children)if(child.name==kind&&child.attribute("ID")==id){
        require(!result,"WorldView direct XML member is ambiguous");result=&child;
    }
    require(result,"WorldView direct XML member is absent");return *result;
}
assets::Bytes physical(const std::filesystem::path& path){
    const auto before=identify_file(path);require(before.bytes<=budget,"WorldView source exceeds budget");
    std::ifstream file(path,std::ios::binary);require(static_cast<bool>(file),"WorldView source unavailable");
    assets::Bytes result((std::istreambuf_iterator<char>(file)),{});
    require(result.size()==before.bytes&&identify_bytes(result).sha256==before.sha256,
            "WorldView source changed during capture");return result;
}
content::XmlNode xml(const assets::Bytes& bytes){
    return content::read_xml_metadata(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
}
struct Spec {std::string context,stem,lua_path,xml_path,lua_sha,xml_sha;};
const std::array<Spec,6> specs{{
    {"WorldView","Assets/UI/InGame/WorldView/WorldView","Assets/DLC/Expansion2/UI/InGame/WorldView/WorldView.lua","Assets/UI/InGame/WorldView/WorldView.xml",
     "3d998dc1b08b19cc94e5f174999a396790368e367a4ea5cb023b8a5c96f83df2","91237b9b480f2b3d52513b414d4a373e85595a4d54c1118ac7f4ad2951a8e96e"},
    {"UnitFlagManager","Assets/UI/InGame/UnitFlagManager","Assets/DLC/Expansion2/UI/InGame/UnitFlagManager.lua","Assets/UI/InGame/UnitFlagManager.xml",
     "d2bfe09c86633e29df346c4cd2f2feacf1cdb5c747798fb34b07eb279a84572f","b14f9bc64a1a81696d4890f39eaf0fe9fced625519c5000626350de867553363"},
    {"ActionInfoPanel","Assets/UI/InGame/WorldView/ActionInfoPanel","Assets/DLC/Expansion2/UI/InGame/WorldView/ActionInfoPanel.lua","Assets/DLC/Expansion2/UI/InGame/WorldView/ActionInfoPanel.xml",
     "b8d9c480fffff3890e9be6eb453b6a59ad6f875096dff035c2658a39168e2b09","cbe379ec9de17676cfbe8141e6b8676bfe0916c2d01f1879cfa567c52b8440ea"},
    {"NotificationPanel","Assets/UI/InGame/WorldView/NotificationPanel","Assets/DLC/Expansion2/UI/InGame/WorldView/NotificationPanel.lua","Assets/DLC/Expansion2/UI/InGame/WorldView/NotificationPanel.xml",
     "f4276f645a1413600684934f014221d018fa9899d829541675184af87c0879fc","34c3415dd93fcc54e9477269a3f2117a62a64468ca983e001f9cc6338e4896ac"},
    {"InGame","Assets/UI/InGame/InGame","Assets/DLC/Expansion2/UI/InGame/InGame.lua","Assets/DLC/Expansion2/UI/InGame/InGame.xml",
     "77bbd2010f8150135beca34cc708ede330d6b98a258618cb1159650f4d8402ff","94400b55698da6b4b54aed1f6c6f477c2faa1149edf78f72a4d2430dc640a8f4"},
    {"YieldIconManager","Assets/UI/InGame/YieldIconManager","Assets/DLC/Expansion2/UI/InGame/YieldIconManager.lua","Assets/UI/InGame/YieldIconManager.xml",
     "55a68343a410edf55e63b3d8dd5d56af01ee619971d8809259af156fcebc84d1","3ba7cecbb6d0c781c1831bc4567e1828312e752921bb55e40ff595ab5ea0a8f3"}
}};
constexpr const char* declaration_path="Assets/DLC/Expansion2/UI/InGame/InGame.xml";
constexpr const char* declaration_sha="94400b55698da6b4b54aed1f6c6f477c2faa1149edf78f72a4d2430dc640a8f4";
}
struct BnwSelectedWorldViewContextOwner::State {
    struct Source {
        assets::AssetHandle lua_handle,xml_handle;
        assets::Bytes lua_bytes,xml_bytes;
        content::XmlNode layout;
        Source(assets::AssetHandle lua,assets::AssetHandle layout_handle):
            lua_handle(std::move(lua)),xml_handle(std::move(layout_handle)),
            lua_bytes(lua_handle.read_bytes(budget)),xml_bytes(xml_handle.read_bytes(budget)),layout(xml(xml_bytes)){}
    };
    const std::thread::id creator=std::this_thread::get_id();
    const std::filesystem::path root;
    // Retained weak descriptor is identity only: no old serial is admitted.
    const BnwStockLuaAttemptLease lease;
    const std::shared_ptr<const BnwCivisOwnedStockSourceBundle> civis_sources;
    assets::Bytes declaration_bytes;
    content::XmlNode declaration;
    std::vector<Source> sources;
    std::vector<Spec> profile;
    std::vector<std::shared_ptr<const detail::ContextLoaderRequestCustody>> loader_requests;
    std::map<std::string,std::string,std::less<>> profile_parents{{"WorldView","InGame"},{"UnitFlagManager","InGame"},{"YieldIconManager","InGame"},{"ActionInfoPanel","WorldView"},{"NotificationPanel","ActionInfoPanel"}};
    void capture_client_context_profile_owned(const assets::ResourceMountTable&,
        const BnwStockLuaAttemptLease&,std::uint64_t);
    // SAME canonical client-owned selected slot. Weak facade retention avoids
    // Source -> Tree -> Source cycles; Bootstrap's actual root slot owns Tree.
    std::weak_ptr<ui::BnwWorldViewControlTree> client_context_slot1;
    struct ClientContextPublication {std::uint64_t serial;std::weak_ptr<ui::BnwWorldViewControlTree> root;};
    std::vector<ClientContextPublication> client_context_publications;
    std::shared_ptr<const detail::BnwWorldViewCanvasXmlMetadata> canvas_xml_metadata;
    mutable bool canvas_xml_querying=false;
    mutable bool image_color_querying=false;
    std::weak_ptr<ui::BnwEngineUiTurnOwner> tooltip_locale;
    bool tooltip_locale_bound=false;
    mutable bool tooltip_querying=false;
    std::shared_ptr<const assets::CivisResourceBankOwner> lua_profile_bank;
    std::shared_ptr<const BnwSelectedLanguageInputOwner> lua_profile_selected_input;
    mutable std::weak_ptr<const BnwContextLuaProfileConfigLoan> lua_debug_config_return;
    mutable bool lua_debug_config_querying=false;
    mutable std::atomic<bool> failed{false};
    mutable bool busy=false;
    std::atomic<bool> alive{true};
    std::weak_ptr<BnwSelectedWorldViewContextOwner> canonical_owner;
    const std::weak_ptr<const LiveSessionState> captured_world;
    mutable std::uint64_t native_generation=0;
    const BnwWorldViewSourceOperationLoan* active=nullptr;
    std::uint64_t scope_serial=0;
    State(const std::filesystem::path& path,const BnwStockLuaAttemptLease& operation,
          std::shared_ptr<const BnwCivisOwnedStockSourceBundle> own):root(path),lease(operation),
          civis_sources(std::move(own)),profile(specs.begin(),specs.end()),captured_world(operation.world_lease()){}
    void require_health_storage_without_callbacks(
        const BnwSelectedWorldViewContextOwner& expected_owner,
        const std::weak_ptr<const LiveSessionState>& world,std::uint64_t generation)const {
        // Foreign calls may poison only the atomic before any mutable read.
        if(creator!=std::this_thread::get_id()){
            failed.store(true);throw ScriptError("UnitFlag metadata storage foreign thread");
        }
        try {
            require(alive.load()&&!failed.load(),"UnitFlag metadata storage retired or failed");
            const auto canonical=canonical_owner.lock();
            require(canonical&&canonical.get()==&expected_owner&&canonical->state_.get()==this,
                    "UnitFlag metadata canonical Source owner/storage substituted");
            const auto actual_world=world.lock(),captured=captured_world.lock();
            require(actual_world&&captured&&actual_world.get()==captured.get()&&
                    !world.owner_before(captured_world)&&!captured_world.owner_before(world)&&
                    generation&&native_generation==generation,
                    "UnitFlag metadata actual world/control/generation changed");
        }catch(...){failed.store(true);throw;}
    }
    FileIdentity source_identity(const std::filesystem::path& path)const{
        return civis_sources?civis_sources->source_identity(path):identify_file(path);
    }
    std::size_t index(std::string_view context)const {
        for(std::size_t i=0;i<profile.size();++i)if(context==profile[i].context)return i;
        throw ScriptError("WorldView source is outside the retained selected slice");
    }
    void current()const {
        // Creator check precedes every mutable field read. A foreign call may
        // only poison the atomic; it never touches vectors or busy state.
        if(creator!=std::this_thread::get_id()){failed.store(true);throw ScriptError("WorldView source foreign thread");}
        require(alive.load()&&!failed.load(),"WorldView source retired or failed");
        if(busy){failed.store(true);throw ScriptError("WorldView source metadata reentered");}
        busy=true;struct Exit {bool& busy;~Exit(){busy=false;}} exit{busy};
        try {
            const auto origin=[&]{if(active)active->require_origin_without_callbacks();};
            origin();if(civis_sources)civis_sources->require_current();
            require(sources.size()==profile.size(),"WorldView retained source profile incomplete");
            require(identify_bytes(declaration_bytes).sha256==declaration_sha&&
                    source_identity(root/declaration_path).sha256==declaration_sha,"WorldView InGame declaration changed");
            origin();
            for(std::size_t i=0;i<profile.size();++i){
                const auto& s=sources[i];const auto& spec=profile[i];
                require(identify_bytes(s.lua_bytes).sha256==spec.lua_sha&&identify_bytes(s.xml_bytes).sha256==spec.xml_sha,
                        "WorldView retained source bytes changed");
                require(identify_bytes(s.lua_handle.read_bytes(budget)).sha256==spec.lua_sha&&
                        identify_bytes(s.xml_handle.read_bytes(budget)).sha256==spec.xml_sha,
                        "WorldView retained VFS source changed");
                origin();
                require(source_identity(root/spec.lua_path).sha256==spec.lua_sha&&
                        source_identity(root/spec.xml_path).sha256==spec.xml_sha,"WorldView physical source changed");
                origin();
            }
        }catch(...){failed.store(true);throw;}
    }
};
#include "bnw_selected_client_context_profile.inc"
class BnwSelectedWorldViewContextOwner::Child final : public ScriptUiChildSource {
    const std::shared_ptr<State> owner_;const std::size_t index_;
    std::shared_ptr<State> owner()const {
        const auto value=owner_;require(static_cast<bool>(value),"WorldView child source expired");
        value->current();return value;
    }
 public:
    Child(const std::shared_ptr<State>& value,std::size_t index):owner_(value),index_(index){}
    std::string_view context()const override {const auto value=owner();return value->profile[index_].context;}
    const assets::AssetHandle& asset()const override {const auto value=owner();return value->sources[index_].lua_handle;}
    void require_current()const override {(void)owner();}
    void require_captured_bytes(std::span<const std::uint8_t> bytes)const override {
        const auto value=owner();
        require(bytes.size()==value->sources[index_].lua_bytes.size()&&identify_bytes(bytes).sha256==value->profile[index_].lua_sha,
                "WorldView child captured Lua bytes differ");if(value->active)value->active->require_origin_without_callbacks();
    }
};
BnwSelectedWorldViewContextOwner::BnwSelectedWorldViewContextOwner(std::shared_ptr<State> state):state_(std::move(state)){}
BnwSelectedWorldViewContextOwner::~BnwSelectedWorldViewContextOwner(){state_->alive.store(false);state_->failed.store(true);}
std::shared_ptr<BnwSelectedWorldViewContextOwner> BnwSelectedWorldViewContextOwner::capture_owned(
    const std::filesystem::path& root,const assets::ResourceMountTable& mounts,const BnwStockLuaAttemptLease& lease,
    std::shared_ptr<const BnwCivisOwnedStockSourceBundle> own){
    const auto serial=lease.operation_serial();
    auto state=std::make_shared<State>(root,lease,std::move(own));
    if(state->civis_sources){
        state->civis_sources->require_current();
        const auto selected_bank=state->civis_sources->bank_->retain_owner();
        require(selected_bank&&selected_bank.get()==state->civis_sources->bank_.get()&&
            !selected_bank.owner_before(state->civis_sources->bank_)&&!state->civis_sources->bank_.owner_before(selected_bank),
            "Lua profile selected startup Bank canonical control differs");
        const auto selected_input=BnwSelectedLanguageInputOwner::capture_from_civis_resources(selected_bank);
        selected_input->require_current();state->civis_sources->require_current();lease.require_operation(serial);
        state->lua_profile_bank=selected_bank;state->lua_profile_selected_input=selected_input;
        state->declaration_bytes=*state->civis_sources->loaded(declaration_path).bytes;
    }else state->declaration_bytes=physical(root/declaration_path);
    lease.require_operation(serial);
    require(identify_bytes(state->declaration_bytes).sha256==declaration_sha,"WorldView selected InGame declaration differs");
    state->declaration=xml(state->declaration_bytes);
    for(const auto& spec:specs){
        // This selected direct request is retained BEFORE AssetKey/lookup.
        // It is the portable loader's explicit request, not root Name metadata.
        auto request=std::shared_ptr<const detail::ContextLoaderRequestCustody>(
            new detail::ContextLoaderRequestCustody(spec.context,spec.stem,{}));
        lease.require_operation(serial);state->loader_requests.push_back(std::move(request));
        lease.require_operation(serial);
        const auto lua=mounts.resolve(assets::AssetKey(std::string(spec.stem)+".lua"));lease.require_operation(serial);
        const auto layout=mounts.resolve(assets::AssetKey(std::string(spec.stem)+".xml"));lease.require_operation(serial);
        const auto* lh=std::get_if<assets::AssetHandle>(&lua.asset);
        const auto* xh=std::get_if<assets::AssetHandle>(&layout.asset);
        require(lh&&xh,"WorldView actual selected VFS source missing or ambiguous");
        state->sources.emplace_back(*lh,*xh);lease.require_operation(serial);
        const auto& source=state->sources.back();
        require(source.layout.name=="Context"&&identify_bytes(source.lua_bytes).sha256==spec.lua_sha&&
                identify_bytes(source.xml_bytes).sha256==spec.xml_sha,"WorldView selected VFS source differs");
    }
    state->capture_client_context_profile_owned(mounts,lease,serial);
    const auto& bulk=direct(state->declaration,"Container","BulkUI");
    const auto& world=direct(bulk,"LuaContext","WorldView");
    require(world.attribute("FileName")==specs[0].stem,"WorldView actual parent source differs");
    const auto& controls=direct(bulk,"Container","WorldViewControls");
    (void)direct(controls,"Container","SelectedUnitContainer");
    require(direct(controls,"LuaContext","UnitFlagManager").attribute("FileName")==specs[1].stem,
            "UnitFlagManager actual sibling membership differs");
    require(direct(state->sources[0].layout,"LuaContext","ActionInfoPanel").attribute("FileName")==specs[2].stem,
            "ActionInfoPanel actual WorldView child membership differs");
    require(direct(state->sources[2].layout,"LuaContext","NotificationPanel").attribute("FileName")==specs[3].stem,
            "NotificationPanel actual ActionInfoPanel child membership differs");
    const auto& notifications=state->sources[3].layout;
    for(const auto* id:{"OuterStack","BigStack","SmallStack"})
        require(unique(notifications,"Stack","ID",id),"NotificationPanel actual authored stack absent");
    require(unique(notifications,"Button","ID","FreePolicyButton")&&
            unique(notifications,"Button","ID","FreeTechButton")&&
            unique(notifications,"Instance","Name","SocialPolicyItem"),
            "NotificationPanel actual authored policy/technology controls or template absent");
    const auto& flags=state->sources[1].layout;
    require(flags.attribute("ColorSet")=="Beige_Black_Alpha","UnitFlagManager actual ColorSet differs");
    for(const auto* id:{"AirCraftFlags","CivilianFlags","MilitaryFlags","GarrisonFlags","CityContainer"})
        (void)direct(flags,"Container",id);
    for(const auto* name:{"NewUnitFlag","CityFlag","AirButton"})
        require(unique(flags,"Instance","Name",name),"UnitFlagManager actual instance template absent");
    require(unique(state->sources[2].layout,"GridButton","ID","EndTurnButton")&&
            unique(state->sources[2].layout,"Label","ID","EndTurnText"),"ActionInfoPanel actual controls absent");
    state->current();lease.require_operation(serial);
    using Canvas=detail::BnwWorldViewCanvasXmlMetadata;
    const std::array<Canvas::Seed,3> canvas_sources{{
        {specs[0].context,specs[0].stem,identify_bytes(state->sources[0].xml_bytes).sha256},
        {specs[2].context,specs[2].stem,identify_bytes(state->sources[2].xml_bytes).sha256},
        {specs[3].context,specs[3].stem,identify_bytes(state->sources[3].xml_bytes).sha256}
    }};
    // SAME real VFS winner/retained declaration capture, no Lua execution or
    // fabricated canvas child attachment. Errors discard the unpublished state.
    state->canvas_xml_metadata=Canvas::capture_owned(state->declaration,state->declaration_bytes,canvas_sources,mounts,lease);
    state->current();lease.require_operation(serial);
    auto owner=std::shared_ptr<BnwSelectedWorldViewContextOwner>(new BnwSelectedWorldViewContextOwner(state));
    state->canonical_owner=owner;
    for(std::size_t i=0;i<state->profile.size();++i)owner->children_.push_back(std::make_shared<Child>(state,i));
    lease.require_operation(serial);return owner;
}
#include "bnw_selected_client_context_source.inc"
#include "bnw_selected_image_color_source.inc"
#include "bnw_selected_button_tooltip_source.inc"
#include "bnw_selected_action_info_locale_source.inc"
#include "bnw_context_lua_profile_config_source_core.inc"
#include "bnw_system_font_language_source_core.inc"
void BnwSelectedWorldViewContextOwner::require_metadata_current()const {state_->current();}
#include "bnw_context_load_name_source_pointer_core.inc"
const BnwWorldViewSourceOperationLoan& BnwSelectedWorldViewContextOwner::current_operation_loan()const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView operation foreign thread");}
    require(s->alive.load()&&!s->failed.load()&&s->active,"WorldView actual native operation absent");
    const auto* loan=s->active;const auto serial=s->scope_serial;
    loan->require_origin_without_callbacks();
    require(s->active==loan&&s->scope_serial==serial,"WorldView active source scope changed");return *loan;
}
void BnwSelectedWorldViewContextOwner::require_sources_current()const {
    const auto& loan=current_operation_loan();const auto serial=loan.operation_serial();
    loan.require_current();state_->current();
    require(&current_operation_loan()==&loan,"WorldView native operation replaced during source callbacks");
    loan.require_operation(serial);
}
std::uint64_t BnwSelectedWorldViewContextOwner::lend_operation_scope(const BnwWorldViewSourceOperationLoan& loan){
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView lender foreign thread");}
    if(!s->alive.load()||s->failed.load()||s->busy||s->active||s->scope_serial==std::numeric_limits<std::uint64_t>::max()){
        s->failed.store(true);throw ScriptError("WorldView lender retired, reentered or exhausted");
    }
    ++s->scope_serial;s->active=&loan;return s->scope_serial;
}
void BnwSelectedWorldViewContextOwner::retire_operation_scope(const BnwWorldViewSourceOperationLoan& loan,std::uint64_t serial)noexcept {
    retire_operation_storage(state_,loan,serial);
}
void BnwSelectedWorldViewContextOwner::retire_operation_storage(const std::shared_ptr<State>& s,const BnwWorldViewSourceOperationLoan& loan,std::uint64_t serial)noexcept {
    if(s->creator!=std::this_thread::get_id())std::terminate();
    if(s->active!=&loan||s->scope_serial!=serial){s->failed.store(true);return;}
    s->active=nullptr;
}
std::weak_ptr<BnwSelectedWorldViewContextOwner> BnwSelectedWorldViewContextOwner::canonical_owner_without_callbacks()const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView canonical owner foreign thread");}
    require(s->alive.load()&&!s->failed.load(),"WorldView canonical owner retired");return s->canonical_owner;
}
void BnwSelectedWorldViewContextOwner::require_world_and_generation_without_callbacks(
 const std::weak_ptr<const LiveSessionState>& world,std::uint64_t generation)const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView generation foreign thread");}
    require(s->alive.load()&&!s->failed.load()&&!world.expired()&&!s->captured_world.expired()&&
      !world.owner_before(s->captured_world)&&!s->captured_world.owner_before(world)&&generation,
      "WorldView initial source world allocation substituted");
    require(s->native_generation==generation,"WorldView captured actual native generation changed");
}
void BnwSelectedWorldViewContextOwner::seal_initial_native_generation_without_callbacks(
 const std::weak_ptr<const LiveSessionState>& world,std::uint64_t generation)const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView generation issuer foreign thread");}
    require(s->alive.load()&&!s->failed.load()&&!s->active&&!s->busy&&!world.expired()&&
      !s->captured_world.expired()&&!world.owner_before(s->captured_world)&&
      !s->captured_world.owner_before(world)&&generation,
      "WorldView genuine first native generation has another source world");
    if(!s->native_generation)s->native_generation=generation;
    require_world_and_generation_without_callbacks(world,generation);
}
void BnwSelectedWorldViewContextOwner::require_operation_origin_without_callbacks(const BnwWorldViewSourceOperationLoan& loan,std::uint64_t serial)const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView origin foreign thread");}
    require(s->alive.load()&&!s->failed.load()&&s->active==&loan&&s->scope_serial==serial&&serial,
            "WorldView actual source scope retired or substituted");
}
const BnwStockLuaAttemptLease& BnwSelectedWorldViewContextOwner::stored_source_lineage_without_callbacks()const {
    const auto s=state_;
    if(s->creator!=std::this_thread::get_id()){s->failed.store(true);throw ScriptError("WorldView lineage foreign thread");}
    require(s->alive.load()&&!s->failed.load(),"WorldView stored lineage retired");return s->lease;
}
void BnwSelectedWorldViewContextOwner::require_source_member_without_callbacks(const ScriptUiChildSource& source)const {
    const auto& loan=current_operation_loan();(void)loan;
    bool found=false;for(const auto& child:children_)if(child.get()==&source)found=true;
    require(found,"WorldView child source allocation substituted");
}
std::shared_ptr<BnwSelectedWorldViewContextOwner::State>
BnwSelectedWorldViewContextOwner::retain_health_source_storage_without_callbacks(
 const BnwSelectedWorldViewContextOwner& owner,
 const std::weak_ptr<const LiveSessionState>& world,std::uint64_t generation){
    const auto saved=owner.state_;
    saved->require_health_storage_without_callbacks(owner,world,generation);
    // The original created callback may still have its render Source active.
    // Retaining its real storage does not lend or bind a health operation.
    return saved;
}
void BnwSelectedWorldViewContextOwner::require_health_source_storage_without_callbacks(
 const std::shared_ptr<State>& saved,const BnwSelectedWorldViewContextOwner& owner,
 const std::weak_ptr<const LiveSessionState>& world,std::uint64_t generation,
 const BnwWorldViewSourceOperationLoan* expected_source,std::uint64_t metadata_serial){
    require(static_cast<bool>(saved),"UnitFlag retained metadata storage absent");
    saved->require_health_storage_without_callbacks(owner,world,generation);
    try {
        require(saved.get()==owner.state_.get()&&
                !saved.owner_before(owner.state_)&&!owner.state_.owner_before(saved),
                "UnitFlag retained metadata storage control substituted");
        if(expected_source){
            // State.current itself uses this origin check while busy. Rejecting
            // busy here would reject a genuine metadata source bookend.
            require(metadata_serial&&saved->active==expected_source&&saved->scope_serial==metadata_serial,
                    "UnitFlag actual active Source metadata scope retired or substituted");
        }else{
            require(!metadata_serial&&!saved->active&&!saved->busy,
                    "UnitFlag metadata prebind has active or reentered Source scope");
        }
    }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<const detail::BnwWorldViewCanvasXmlMetadata>
BnwSelectedWorldViewContextOwner::canvas_xml_metadata_owned(const BnwWorldViewSourceOperationLoan& operation)const {
    const auto saved=state_;const auto* const facade_address=this;
    try{
        if(saved->creator!=std::this_thread::get_id()){saved->failed.store(true);throw ScriptError("Canvas XML getter foreign creator");}
        require(saved->alive.load()&&!saved->failed.load(),"Canvas XML facade retired before identity capture");
        const auto weak_origin=saved->canonical_owner;
        const auto scope_origin=saved->scope_serial;
        const auto pure=[&]{
            if(saved->creator!=std::this_thread::get_id()){saved->failed.store(true);throw ScriptError("Canvas XML getter foreign creator");}
            require(saved->alive.load()&&!saved->failed.load()&&saved->active==&operation&&scope_origin&&saved->scope_serial==scope_origin,
                "Canvas XML source facade/operation retired");
            // Temporary canonical Source facade locks end before any full or
            // asset callback. Saved State cannot hide actual facade retirement.
            {const auto actual=saved->canonical_owner.lock();
             require(actual&&actual.get()==facade_address&&actual->state_.get()==saved.get()&&
                 !weak_origin.owner_before(saved->canonical_owner)&&!saved->canonical_owner.owner_before(weak_origin)&&
                 !weak_origin.owner_before(actual)&&!actual.owner_before(weak_origin),
                 "Canvas XML canonical Source allocation changed");}
            operation.require_origin_without_callbacks();
        };
        pure();
        if(saved->canvas_xml_querying){saved->failed.store(true);saved->lease.invalidate();throw ScriptError("Canvas XML getter reentered");}
        saved->canvas_xml_querying=true;struct End{bool&v;~End(){v=false;}}end{saved->canvas_xml_querying};
        const auto serial=operation.operation_serial();pure();
        const auto metadata=saved->canvas_xml_metadata;require(static_cast<bool>(metadata),"Canvas XML capture unavailable");
        operation.require_operation(serial);pure();saved->current();pure();
        for(const auto& doc:metadata->documents_){
            operation.require_operation(serial);pure();
            const auto bytes=doc.handle.read_bytes(budget);pure();
            operation.require_operation(serial);pure();
            const auto identity=identify_bytes(bytes);
            require(identity.bytes==doc.payload_identity.bytes&&identity.sha256==doc.payload_identity.sha256&&
                identify_bytes(*doc.bytes).sha256==doc.payload_identity.sha256,
                "Canvas XML held payload changed after declaration capture");
        }
        operation.require_operation(serial);pure();
        require(saved->canvas_xml_metadata.get()==metadata.get()&&
                !saved->canvas_xml_metadata.owner_before(metadata)&&!metadata.owner_before(saved->canvas_xml_metadata),
                "Canvas XML immutable storage rebound");
        // Passive immutable source values may outlive this getter. Their private
        // consumers still need a fresh Source/GUI/window/native receiver scope.
        return metadata;
    }catch(...){saved->failed.store(true);saved->lease.invalidate();throw;}
}
#include "bnw_font_resource_scope_core.inc"
detail::ContextLoaderResourceOperand BnwSelectedWorldViewContextOwner::context_loader_resource_operand_owned(
 const BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<const ScriptUiChildSource>& descriptor)const {
 const auto saved=state_;const auto*const facade=this;
 try{
  operation.require_origin_without_callbacks();
  if(saved->creator!=std::this_thread::get_id()){saved->failed.store(true);throw ScriptError("Context loader resource foreign creator");}
  const auto scope=saved->scope_serial;const auto serial=operation.operation_serial();
  const auto pure=[&]{
   require_operation_origin_without_callbacks(operation,scope);
   const auto actual=saved->canonical_owner.lock();
   require(actual&&actual.get()==facade&&actual->state_.get()==saved.get(),"Context loader canonical Source owner substituted");
   operation.require_origin_without_callbacks();
  };
  pure();require(static_cast<bool>(descriptor),"Context loader exact child absent");
  std::size_t index=children_.size();
  for(std::size_t i=0;i<children_.size();++i)if(children_[i].get()==descriptor.get()&&
      !children_[i].owner_before(descriptor)&&!descriptor.owner_before(children_[i]))index=i;
  require(index<saved->profile.size()&&saved->civis_sources&&saved->native_generation,
          "Context loader requires canonical selected Civis bundle/child/generation");
  operation.require_operation(serial);pure();saved->current();pure();
  saved->civis_sources->require_current();pure();operation.require_operation(serial);pure();
  const auto& spec=saved->profile[index];
  require(saved->loader_requests.size()==saved->profile.size()&&saved->loader_requests[index],
          "Context loader original request custody missing");
  const auto request=saved->loader_requests[index];
  require(request->context_==spec.context&&request->original_utf8_==spec.stem,
          "Context loader original request no longer matches actual Source profile");
  const auto& source=saved->sources[index];
  const auto lua_key=source.lua_handle.requested_key().original_name();
  const auto xml_key=source.xml_handle.requested_key().original_name();
  require(assets::AssetKey(lua_key).normalized_name()==assets::AssetKey(std::string(spec.stem)+".lua").normalized_name()&&
          assets::AssetKey(xml_key).normalized_name()==assets::AssetKey(std::string(spec.stem)+".xml").normalized_name(),
          "Context loader source handles have another selected VFS membership");
  auto result=detail::ContextLoaderResourceOperand(saved->civis_sources,descriptor,spec.context,
   lua_key,xml_key,spec.stem,spec.lua_sha,spec.xml_sha,serial,saved->native_generation,request);
  pure();return result;
 }catch(...){saved->failed.store(true);saved->lease.invalidate();throw;}
}
const content::XmlNode& BnwSelectedWorldViewContextOwner::authored_layout(std::string_view context)const {
    require_sources_current();if(context=="InGame")return state_->declaration;
    return state_->sources[state_->index(context)].layout;
}
std::shared_ptr<const ScriptUiChildSource> BnwSelectedWorldViewContextOwner::prepared_source(std::string_view context)const {
    state_->current();return children_[state_->index(context)];
}
std::vector<std::int64_t> BnwSelectedWorldViewContextOwner::native_mouse_callback_events()const {
    require_sources_current();
    // Original registered Mouse protocol: 41D98C PUSH0/eLClick and
    // 41D9B6 PUSH2/eRClick, each actual lua_pushinteger -> lua_setfield.
    // Static byte proof is joined in the source packet, not an input receipt.
    // Original 41D9F8/eMouseEnter5 and 41DA0D/eMouseExit6, same Lua registrar.
    std::vector<std::int64_t> result{0,2,5,6};require_sources_current();return result;
}
void BnwSelectedWorldViewContextOwner::require_current(std::string_view context,const ScriptUiChildSource* source)const {
    require_sources_current();if(context=="InGame"){require(!source||source==children_[4].get(),"InGame actual root source substituted");return;}
    const auto i=state_->index(context);
    if(source)require(source==children_[i].get(),"WorldView child source allocation substituted");
    require_native_activation();
}
std::string BnwSelectedWorldViewContextOwner::selected_context_declared_parent_owned(std::string_view context)const{
    require_sources_current();const auto saved=state_;(void)saved->index(context);
    const auto found=saved->profile_parents.find(context);
    require(found!=saved->profile_parents.end(),"Selected Context genuine parent declaration not captured");
    const auto parent=found->second;require_sources_current();return parent;
}
std::shared_ptr<const ScriptUiChildSource> BnwSelectedWorldViewContextOwner::resolve_child(
    std::string_view parent,const ScriptUiChildSource* source,std::string_view request){
    require_sources_current();const auto pi=state_->index(parent);
    require((parent=="InGame"&&!source)||source==children_.at(pi).get(),
        "Selected nested child actual parent Source substituted");
    const auto layout=state_->sources.at(pi).layout;
    std::shared_ptr<const ScriptUiChildSource> result;std::size_t matches=0;
    const auto walk=[&](auto&& self,const content::XmlNode& node)->void{
        if(node.name=="Instance"||node.name=="InstanceData")return;
        if(node.name=="LuaContext"){
            const auto stem=node.attribute("FileName");auto id=node.attribute("ID");
            if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
            if(request==stem||request==id){
                const auto i=state_->index(id);const auto route=state_->profile_parents.find(id);
                require(route!=state_->profile_parents.end()&&route->second==parent&&state_->profile[i].stem==stem,
                    "Selected nested source needs genuine scoped descriptor/captured parent");
                result=children_.at(i);++matches;
            }return;
        }
        for(const auto& child:node.children)self(self,child);
    };
    walk(walk,layout);require(matches==1&&result,"Selected nested child actual declaration unavailable/ambiguous");
    require_native_activation();return result;
}
 void BnwSelectedWorldViewContextOwner::require_native_activation()const {
    require_sources_current();
}
}
