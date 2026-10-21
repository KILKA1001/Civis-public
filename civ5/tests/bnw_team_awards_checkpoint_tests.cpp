#include "civ5/runtime/bnw_team_turn_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <bit>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace civ5;
void need(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
template<class F> void rejects(F &&run) {
    bool caught=false;try{run();}catch(const std::exception &){caught=true;}
    need(caught,"Malformed team-award checkpoint accepted");
}
std::string_view name(const char *p) {return p?std::string_view(p):std::string_view();}
saves::PlotArrayObservation &array(saves::TeamObservation &t,std::string_view id) {
    for(auto &x:t.arrays)if(name(x.name)==id)return x;
    throw std::runtime_error("Test array absent");
}
std::int64_t &points(saves::TeamObservation &t) {
    for(auto &x:t.fields)if(name(x.name)=="victory_points")return x.value;
    throw std::runtime_error("Test points absent");
}
saves::TeamHashedArray &awards(saves::TeamObservation &t) {
    for(auto &x:t.hashes)if(name(x.name)=="small_award_achieved")return x;
    throw std::runtime_error("Test awards absent");
}
saves::SavedSessionSnapshot fixture() {
    saves::SavedSessionSnapshot w;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    w.game_state.layout=bnw;w.teams.emplace();w.teams->layout=bnw;
    w.teams->dimensions.teams=64;w.teams->dimensions.civ_teams=63;
    w.teams->teams.resize(64);
    for(std::size_t i=0;i<64;++i){auto &t=w.teams->teams[i];t.index=i;
        t.fields={{"id",static_cast<std::int64_t>(i)},{"tech_version",1},
                  {"saved_techs",3},{"victory_points",0}};
        t.arrays={{"turns_at_war",1,std::vector<std::int64_t>(64)},
                  {"turns_locked_at_war",1,std::vector<std::int64_t>(64)},
                  {"tech_hashes",1,{11,22,33}},{"no_trade_tech",1,{1,0,1}},
                  {"research_progress",1,{10,20,30}}};
        // Raw full wire span includes zero-hash holes, which have NO values.
        t.hashes={{"small_award_achieved",3,{{0x111u,{0}},{0,{}},{0x222u,{0}}}}};
    }
    return w;
}
void u32(std::vector<std::uint8_t> &b,std::uint32_t n) {
    for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(n>>(8*i)));
}
void i32(std::vector<std::uint8_t> &b,std::int64_t n) {
    u32(b,std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(n)));
}
std::uint32_t read32(const std::vector<std::uint8_t> &b,std::size_t at) {
    std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(b.at(at+i))<<(8*i);
    return n;
}
void set32(std::vector<std::uint8_t> &b,std::size_t at,std::uint32_t n) {
    for(unsigned i=0;i<4;++i)b.at(at+i)=static_cast<std::uint8_t>(n>>(8*i));
}
// Independent literal V1 writer: it does not invoke the candidate V2 encoder.
std::vector<std::uint8_t> legacy_body(saves::SavedSessionSnapshot w) {
    std::vector<std::uint8_t> b;u32(b,64);u32(b,63);
    for(std::size_t i=0;i<64;++i){auto &t=w.teams->teams[i];i32(b,i);
        for(auto n:array(t,"turns_at_war").values)i32(b,n);
        for(auto n:array(t,"turns_locked_at_war").values)i32(b,n);
        auto &hash=array(t,"tech_hashes").values;
        auto &flags=array(t,"no_trade_tech").values;u32(b,hash.size());
        for(std::size_t r=0;r<hash.size();++r){u32(b,hash[r]);b.push_back(flags[r]);}
    }
    return b;
}
saves::CivisSaveSection legacy(const saves::SavedSessionSnapshot &source,
                               const saves::SavedSessionSnapshot &live,
                               const runtime::SessionCheckpointIdentity &id) {
    std::vector<std::uint8_t> b{'C','I','V','B','T','T','0','1'};u32(b,1);
    b.insert(b.end(),id.payload_sha256.begin(),id.payload_sha256.end());
    b.insert(b.end(),id.rules_sha256.begin(),id.rules_sha256.end());
    for(const auto &body:{legacy_body(source),legacy_body(live)}){
        u32(b,body.size());b.insert(b.end(),body.begin(),body.end());}
    return {runtime::bnw_team_turn_checkpoint_section_id,0,std::move(b)};
}
void synthetic() {
    const runtime::SessionCheckpointIdentity id{std::string(64,'a'),std::string(64,'b')};
    const auto source=fixture();auto live=source;
    // Aggregate stores may contain repeated qualifying players/cities; the
    // codec retains the exact total rather than reconstructing/deduplicating it.
    points(live.teams->teams[0])=35;points(live.teams->teams[62])=INT32_MAX;
    awards(live.teams->teams[0]).rows[0].values[0]=1;
    awards(live.teams->teams[0]).rows[2].values[0]=1;
    awards(live.teams->teams[62]).rows[2].values[0]=1;
    array(live.teams->teams[0],"turns_at_war").values[1]=9;
    array(live.teams->teams[0],"no_trade_tech").values[0]=0;
    const auto section=runtime::make_bnw_team_turn_checkpoint(live,source,id);
    need(read32(section.bytes,8)==2,"Maker did not emit internal V2");
    auto restored=source;array(restored.teams->teams[0],"research_progress").values[1]=77;
    runtime::restore_bnw_team_turn_checkpoint(section,restored,source,id);
    need(points(restored.teams->teams[0])==35&&points(restored.teams->teams[62])==INT32_MAX&&
         awards(restored.teams->teams[0]).wire_count==3&&
         awards(restored.teams->teams[0]).rows[0].values[0]==1&&
         awards(restored.teams->teams[0]).rows[1].type_hash==0&&
         awards(restored.teams->teams[0]).rows[1].values.empty()&&
         awards(restored.teams->teams[0]).rows[2].values[0]==1&&
         awards(restored.teams->teams[62]).rows[2].values[0]==1&&
         array(restored.teams->teams[0],"research_progress").values[1]==77,
         "Complete awards/points or foreign ID31 state changed");
    need(runtime::make_bnw_team_turn_checkpoint(restored,source,id).bytes==section.bytes,
         "Complete V2 re-save changed bytes");
    auto atomic_refusal=[&](const saves::CivisSaveSection &bad){auto state=source;
        rejects([&]{runtime::restore_bnw_team_turn_checkpoint(bad,state,source,id);});
        need(runtime::make_bnw_team_turn_checkpoint(state,source,id).bytes==
             runtime::make_bnw_team_turn_checkpoint(source,source,id).bytes,
             "Rejected V2 partially changed ANY owned raw state");};
    auto corrupt=section;corrupt.bytes.pop_back();atomic_refusal(corrupt);
    corrupt=section;corrupt.bytes[12]^=1;atomic_refusal(corrupt);
    corrupt=section;set32(corrupt.bytes,8,3);atomic_refusal(corrupt);
    const std::size_t new_start=148+read32(section.bytes,140);
    constexpr std::size_t team_stride=4+64*4+64*4+4+3*5+4+4+5+4+5;
    constexpr std::size_t first_hash=8+4+64*4+64*4+4+3*5+4+4;
    // Malformed last-team Boolean forces complete decode before first-team stores.
    corrupt=section;corrupt.bytes.at(new_start+63*team_stride+first_hash+4)=2;
    atomic_refusal(corrupt);
    corrupt=section;set32(corrupt.bytes,new_start+first_hash+9,0x111u);atomic_refusal(corrupt);
    corrupt=section;set32(corrupt.bytes,new_start+first_hash,0x333u);atomic_refusal(corrupt);
    corrupt=section;set32(corrupt.bytes,new_start+first_hash-4,UINT32_MAX);atomic_refusal(corrupt);
    // A changed destination baseline also refuses before ANY scalar writes.
    auto wrong=source;points(wrong.teams->teams[63])=1;
    rejects([&]{runtime::restore_bnw_team_turn_checkpoint(section,wrong,source,id);});
    need(points(wrong.teams->teams[63])==1&&points(wrong.teams->teams[0])==0,
         "Baseline refusal partially wrote points");
    auto invalid=live;
    auto rejected_world=[&]{rejects([&]{(void)runtime::make_bnw_team_turn_checkpoint(invalid,source,id);});};
    awards(invalid.teams->teams[0]).wire_count=2;rejected_world();invalid=live;
    awards(invalid.teams->teams[0]).rows[1].values={1};rejected_world();invalid=live;
    awards(invalid.teams->teams[0]).rows[2].type_hash=0x111u;rejected_world();invalid=live;
    awards(invalid.teams->teams[0]).rows[0].values={2};rejected_world();invalid=live;
    awards(invalid.teams->teams[0]).rows[0].type_hash=std::uint64_t(UINT32_MAX)+1;
    rejected_world();invalid=live;
    points(invalid.teams->teams[0])=std::int64_t(INT32_MAX)+1;rejected_world();invalid=live;
    invalid.teams->teams[63].fields.push_back({"victory_points",0});rejected_world();invalid=live;
    invalid.teams->teams[63].hashes.clear();rejected_world();invalid=live;
    std::swap(awards(invalid.teams->teams[0]).rows[0],awards(invalid.teams->teams[0]).rows[2]);
    rejected_world();
    // Plain BNW's actual empty award collection must remain representable.
    auto zero=source;for(auto &t:zero.teams->teams){awards(t).wire_count=0;awards(t).rows.clear();}
    const auto empty=runtime::make_bnw_team_turn_checkpoint(zero,zero,id);auto empty_copy=zero;
    runtime::restore_bnw_team_turn_checkpoint(empty,empty_copy,zero,id);
    need(runtime::make_bnw_team_turn_checkpoint(empty_copy,zero,id).bytes==empty.bytes,
         "Zero-row ordinary BNW V2 failed");
    // Raw codec preserves the signed native i32 domain. Action owners enforce
    // changeVictoryPoints >=0 separately; a codec is not action authority.
    auto signed_live=source;points(signed_live.teams->teams[1])=INT32_MIN;
    const auto signed_section=runtime::make_bnw_team_turn_checkpoint(signed_live,source,id);
    auto signed_copy=source;runtime::restore_bnw_team_turn_checkpoint(signed_section,signed_copy,source,id);
    need(points(signed_copy.teams->teams[1])==INT32_MIN,"Signed i32 wire changed");
    // Legacy V1 restores precisely its original fields, even with unrelated
    // current points/award values and when legacy fixtures have no new fields.
    const auto v1=legacy(source,live,id);auto legacy_copy=source;
    points(legacy_copy.teams->teams[0])=123;
    awards(legacy_copy.teams->teams[0]).rows[2].values[0]=1;
    runtime::restore_bnw_team_turn_checkpoint(v1,legacy_copy,source,id);
    need(points(legacy_copy.teams->teams[0])==123&&
         awards(legacy_copy.teams->teams[0]).rows[2].values[0]==1&&
         array(legacy_copy.teams->teams[0],"turns_at_war").values[1]==9,
         "V1 changed fields outside its original ownership");
    auto minimal=source;for(auto &t:minimal.teams->teams){t.fields.pop_back();t.hashes.clear();}
    auto minimal_live=minimal;array(minimal_live.teams->teams[0],"turns_at_war").values[1]=9;
    auto minimal_copy=minimal;
    runtime::restore_bnw_team_turn_checkpoint(legacy(minimal,minimal_live,id),minimal_copy,minimal,id);
    need(array(minimal_copy.teams->teams[0],"turns_at_war").values[1]==9,
         "V1 incorrectly required new award fields");
    rejects([&]{(void)runtime::make_bnw_team_turn_checkpoint(minimal,minimal,id);});
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto boot=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
    need(boot.preview.host!=nullptr,"Selected actual BNW Host absent");
    const auto &host=*boot.preview.host;const auto &source=host.session().source().snapshot;
    auto live=host.session().state().world;
    auto &p=points(live.teams->teams.at(0));p=p==INT32_MAX?p-1:p+1;
    auto &raw=awards(live.teams->teams.at(0));
    for(auto &row:raw.rows)if(row.type_hash){row.values.at(0)=row.values.at(0)?0:1;break;}
    const auto section=runtime::make_bnw_team_turn_checkpoint(live,source,host.identity());
    auto restored=source;runtime::restore_bnw_team_turn_checkpoint(section,restored,source,host.identity());
    need(runtime::make_bnw_team_turn_checkpoint(restored,source,host.identity()).bytes==section.bytes,
         "Actual selected BNW V2 points/award round trip changed");
    std::cout<<"selected BNW teams="<<source.teams->teams.size()
             <<" complete_team_v2_bytes="<<section.bytes.size()<<'\n';
}
}
int main(int argc,char **argv) {
    try{synthetic();if(argc==3)selected(argv[1],argv[2]);return 0;}
    catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
