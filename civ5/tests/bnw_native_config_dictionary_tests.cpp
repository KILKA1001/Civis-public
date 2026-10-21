#include "../src/ui/bnw_native_config_dictionary.hpp"
#include <bit>
#include <cfenv>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using namespace civ5::ui::detail;
static_assert(!std::is_default_constructible_v<BnwNativeConfigDictionary>);
static_assert(!std::is_copy_constructible_v<BnwNativeConfigDictionary>);
static_assert(!std::is_move_constructible_v<BnwNativeConfigDictionary>);
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<BnwConfigBoolOperation> validation(){return {{"Debugging","EnableTuner",false},{"Debugging","ValidateGameDatabase",false}};}
}
int main(){
    // Source827A00 returns lookup-false on first default insertion; next access
    // must find SAME mutation even when a DIFFERENT default is supplied.
    auto missing=diagnose_bnw_config_dictionary("",{{"Debugging","EnableTuner",false},{"Debugging","ValidateGameDatabase",false},{"debugging","validategamedatabase",true}});
    require(missing.leaf_complete&&!missing.auto_save&&missing.dirty&&missing.load_generation==1,"constructor/load/default state");
    require(missing.entries.size()==2&&missing.getters.size()==3,"default append count");
    require(missing.getters[0].section_inserted&&missing.getters[0].inserted&&!missing.getters[0].found,"missing section/default source order");
    require(!missing.getters[1].section_inserted&&missing.getters[1].inserted&&!missing.getters[1].found,"second key same section");
    require(missing.getters[2].found&&!missing.getters[2].inserted&&!missing.getters[2].value,"missing insertion is retained, not replayed");
    require(missing.entries[0].key=="EnableTuner"&&missing.entries[1].key=="ValidateGameDatabase","actual normal source getters order");
    for(const auto* value:{"0","1","2","-1","-2147483648","+2"}){
        auto quoted=diagnose_bnw_config_dictionary(std::string("[Debugging]\r\nEnableTuner=0\r\nValidateGameDatabase=")+value+"\r\n",validation());
        require(quoted.leaf_complete&&!quoted.dirty&&quoted.getters[1].found,"source loaded key must remain clean");
        require(quoted.getters[1].value==(std::string_view(value)!="0"),"original827A00 nonzero source fixture");
        require(quoted.entries.size()==2,"present key must not append");
    }
    auto first=diagnose_bnw_config_dictionary("; source comment\npresection=not-parsed\n[debugging]\nVALIDATEGAMEDATABASE=0\nValidateGameDatabase=1\n[Debugging]\nEnableTuner=1\n",validation());
    require(first.leaf_complete&&first.entries.size()==4&&first.dirty,"FIRST section retains source entries and default insertion");
    require(first.getters[0].inserted&&!first.getters[0].value&&!first.getters[1].value&&first.getters[1].found,"FIRST key/section source semantics");
    auto distinct=diagnose_bnw_config_dictionary("[DEBUG]\nValidateGameDatabase=1\n[Debugging]\nValidateGameDatabase=0\n",validation());
    require(distinct.leaf_complete&&!distinct.getters[1].value&&distinct.getters[1].found,"DEBUG must not alias Debugging");
    auto true_default=diagnose_bnw_config_dictionary("",{{"Debugging","ValidateGameDatabase",true},{"Debugging","ValidateGameDatabase",false}});
    require(true_default.getters[0].value&&!true_default.getters[0].found&&true_default.getters[1].found&&true_default.getters[1].value,"true default output and mandatory dictionary mutation");
    require(std::bit_cast<std::uint32_t>(true_default.entries[0].numeric)==0x3f800000U,"default insertion native float mirror");
    auto exact=diagnose_bnw_config_dictionary("[Debugging]\nValidateGameDatabase=16777216\n",validation());
    require(exact.leaf_complete&&std::bit_cast<std::uint32_t>(exact.entries[0].numeric)==0x4b800000U,"exact finite atoi/atof mirror");
    for(const auto* value:{"2147483648","16777217","2147483647","0.1","1e-3","1e+","0x1","nan","Infinity","1e100"}){
        auto bad=diagnose_bnw_config_dictionary(std::string("[Debugging]\nEnableTuner=1\nValidateGameDatabase=")+value,validation());
        require(!bad.leaf_complete&&!bad.refusal.empty()&&bad.load_attempted&&bad.dirty&&bad.getters.empty(),"unsupported load cannot publish bools");
        require(bad.entries.size()==1&&bad.entries[0].key=="EnableTuner","source load prefix retained on refusal");
    }
    struct PairCase {const char* text;std::int32_t integer;std::uint32_t bits;};
    // Independent exact encodings, paired with distinct source atoi prefix.
    const PairCase cases[]={
        {"",0,0},{"true",0,0},{"None",0,0},{"C:\\local\\resource",0,0},
        {"\"en_US\"",0,0},{"+-1",0,0},{"  +12suffix  ",12,0x41400000U},
        {"-12 ; comment",-12,0xc1400000U},{"1junk",1,0x3f800000U},
        {"0.5",0,0x3f000000U},{".5suffix",0,0x3f000000U},
        {"-2.5",-2,0xc0200000U},{"1.25",1,0x3fa00000U},
        {"5e-1",5,0x3f000000U},{"2e1suffix",2,0x41a00000U},
        {"25e-1",25,0x40200000U},{"1e0",1,0x3f800000U},
        {"-0",0,0x80000000U},{"-0.000",0,0x80000000U},
        {"0.000000059604644775390625",0,0x33800000U}
    };
    const auto prior=std::fegetround();
    for(const auto rounding:{FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO}){
        require(std::fesetround(rounding)==0,"host fenv test setup");
        for(const auto& c:cases){
            auto quoted=diagnose_bnw_config_dictionary(std::string("[Debugging]\nEnableTuner=0\nValidateGameDatabase=")+c.text+"\n",validation());
            require(quoted.leaf_complete&&!quoted.dirty&&quoted.entries.size()==2,"additional finite CRT branch");
            require(quoted.entries[1].integer==c.integer&&std::bit_cast<std::uint32_t>(quoted.entries[1].numeric)==c.bits,"paired int/float golden must ignore host rounding");
            require(quoted.getters[1].found&&quoted.getters[1].value==(c.integer!=0),"bool consumes int cache, not atof float cache");
        }
    }
    require(std::fesetround(prior)==0,"host fenv restore");
    for(const auto* payload:{"[ Debugging ]\nValidateGameDatabase=1","[Debugging\nValidateGameDatabase=1","[Debugging]\nnoequals"}){
        auto bad=diagnose_bnw_config_dictionary(payload,validation());require(!bad.leaf_complete&&!bad.refusal.empty(),"unsupported grammar refused");
    }
    auto nul=diagnose_bnw_config_dictionary(std::string_view("\0",1),validation());require(!nul.leaf_complete,"NUL source stream refused");
    auto wide=diagnose_bnw_config_dictionary(std::string(1024,'x')+"\n",validation());require(!wide.leaf_complete,"native line buffer frontier");
    std::cout<<"PASS Config dictionary source FIRST/default/dirty/order/paired-CRT/exact-float/refusal leaves; native routing unowned\n";
}
