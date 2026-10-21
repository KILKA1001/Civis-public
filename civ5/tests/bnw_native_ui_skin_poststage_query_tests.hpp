#pragma once
#include "../src/ui/bnw_native_ui_skin_poststage_instance.hpp"
#include <thread>
namespace civ5::ui::poststage_query_authored_tests {
inline void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
inline void ordinary_data_operations(){
    using namespace native_ui_skin_query_detail;
    //These are genuine ordinary data operations, never synthetic sealed leases.
    std::uint32_t count=0;
    for(const auto& text:{std::string(),std::string("\0tail",5),std::string("a\0tail",6)}){
        const auto result=ordinary_null_query(text,0);
        require(result==(text.empty()||text.front()=='\0'?0U:1U),"Native C-prefix result differs");
        count=completed_entry_count(count); //after normal operation, includingEAX0
    }
    require(count==3,"Empty/NUL ordinary completion must still count");
    require(ordinary_null_query("x",0x80)==1 && ordinary_null_query("",0x80)==0,"NativeFile80 null route differs");
    require(completed_entry_count(0xffffffffU)==0 && !available(0x7fffffffU,1,0) &&
            available(0xffffffffU,2,0) && !available(0,0,0),"Native DWORD wrap/signed availability differs");
    bool rejected=false;try{(void)ordinary_null_query("x",8);}catch(const std::runtime_error&){rejected=true;}
    require(rejected,"Unknown filter entered output-materialization branch");
    auto before_failure=count;
    try { (void)ordinary_null_query("x",8); count=completed_entry_count(count); }
    catch(const std::runtime_error&) {}
    require(count==before_failure,"Aborted operation was counted before normal completion");
    static_assert(!std::is_default_constructible_v<detail::BnwNativeUiSkinPoststageQueryLease>);
    static_assert(!std::is_default_constructible_v<detail::BnwNativeUiSkinPoststageInstance>);
}
inline void actual_unsealed_owner(const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& owner){
    const auto refuses=[&]{bool failed=false;try{owner->require_native_package_category_sources_complete();}
        catch(const std::exception&){failed=true;}require(failed,"Unsealed real Package minted completed poststage state");};
    refuses(); //actual ownedPackage, no fixture row/token/friend issuer
    bool foreign=false;std::thread thread([&]{try{owner->require_native_package_category_sources_complete();}
        catch(const std::exception&){foreign=true;}});thread.join();
    require(foreign,"Foreign thread entered real poststage owner");
}
}
