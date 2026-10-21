#include "civ5/runtime/bnw_fresh_advisors.hpp"
#include <atomic>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace {
using namespace civ5::runtime;
void need(bool condition,const char* message) {if(!condition) throw std::runtime_error(message);}
template<class F> void refuses(F f,BnwFreshAdvisorError code) {
    try {f();} catch(const BnwFreshAdvisorRefusal& error) {
        need(error.code()==code,"Unexpected Advisor refusal code");return;
    }
    throw std::runtime_error("Expected Advisor refusal");
}
void counsel() {
    detail::BnwAdvisorCounselStorage owned;
    need(owned.size()==50,"Native counsel resize is exactly 50");
    const auto writes=owned.constructor_writes();
    need(writes.size()==100,"Native counsel Reset has two writes per slot");
    for(std::size_t i=0;i<50;++i) {
        const auto row=owned.row(i);
        need(row.advisor==-1 && row.value==-1,"Independent native counsel sentinels");
        need(writes[2*i].field==BnwAdvisorWriteField::counsel_advisor &&
             writes[2*i].slot==i && writes[2*i].value==-1,"Counsel advisor-before-value order");
        need(writes[2*i+1].field==BnwAdvisorWriteField::counsel_value &&
             writes[2*i+1].slot==i && writes[2*i+1].value==-1,"Counsel value write literal");
        refuses([&]{(void)owned.text_byte(i,0);},BnwFreshAdvisorError::field_unwritten);
        refuses([&]{(void)owned.text_byte(i,511);},BnwFreshAdvisorError::field_unwritten);
    }
    refuses([&]{(void)owned.row(50);},BnwFreshAdvisorError::index_unavailable);
    refuses([&]{(void)owned.row(std::numeric_limits<std::size_t>::max());},BnwFreshAdvisorError::index_unavailable);
    refuses([&]{(void)owned.text_byte(0,512);},BnwFreshAdvisorError::index_unavailable);
    std::atomic_bool wrong_thread=false;
    std::thread other([&]{try {(void)owned.size();} catch(const BnwFreshAdvisorRefusal& e) {
        wrong_thread=e.code()==BnwFreshAdvisorError::wrong_thread;
    }});other.join();need(wrong_thread,"Counsel foreign-thread guard");
    owned.reset_detached();need(owned.row(49).value==-1,"Native Reset retains sentinel");
    refuses([&]{(void)owned.text_byte(49,4);},BnwFreshAdvisorError::field_unwritten);
    owned.uninit_detached();need(owned.size()==0,"Counsel Uninit clear logical size");
    owned.reset_detached();need(owned.size()==0,"Reset after clear does not resize again");
    refuses([&]{(void)owned.row(0);},BnwFreshAdvisorError::index_unavailable);
}
void recommender() {
    detail::BnwAdvisorRecommenderStorage owned;
    const auto data=owned.observe();
    const auto writes=owned.constructor_writes();
    need(writes.size()==28,"Member12 plus ResetCity12 plus ResetTechs4");
    for(std::size_t i=0;i<4;++i) {
        need(data.recommended_builds[i].type==0 && data.recommended_builds[i].index==-1 &&
             data.recommended_builds[i].turns==-1,"Native ResetCity independent sentinels");
        need(data.recommended_techs[i]==-1,"Native NO_TECH literal");
        need(data.weighted_sizes[i]==0,"Every native weighted container logical-empty");
        const std::array fields{BnwAdvisorWriteField::member_type,BnwAdvisorWriteField::member_index,
                                BnwAdvisorWriteField::member_turns};
        const std::array reset_fields{BnwAdvisorWriteField::reset_type,BnwAdvisorWriteField::reset_index,
                                      BnwAdvisorWriteField::reset_turns};
        const std::array<std::int32_t,3> reset_values{0,-1,-1};
        for(std::size_t j=0;j<3;++j) {
            need(writes[3*i+j].field==fields[j] && writes[3*i+j].slot==i &&
                 writes[3*i+j].value==0,"CvCityBuildable ctor is 0/0/0, before ResetCity");
            need(writes[12+3*i+j].field==reset_fields[j] && writes[12+3*i+j].slot==i &&
                 writes[12+3*i+j].value==reset_values[j],"ResetCity native field and slot order");
        }
        need(writes[24+i].field==BnwAdvisorWriteField::reset_tech &&
             writes[24+i].slot==i && writes[24+i].value==-1,"ResetTechs follows entire ResetCity");
    }
    owned.uninit_detached(); // Unlike Counsel, this native method is empty.
    auto after=owned.observe();
    need(after.recommended_builds[3].index==-1 && after.recommended_techs[3]==-1,
         "Empty recommender Uninit preserves stored recommendations");
    owned.reset_detached();after=owned.observe();
    need(after.recommended_builds[0].turns==-1 && after.weighted_sizes[2]==0,"Recommender Reset");
    std::atomic_bool wrong_thread=false;
    std::thread other([&]{try {(void)owned.observe();} catch(const BnwFreshAdvisorRefusal& e) {
        wrong_thread=e.code()==BnwFreshAdvisorError::wrong_thread;
    }});other.join();need(wrong_thread,"Recommender foreign-thread guard");
}
static_assert(!std::is_default_constructible_v<BnwFreshAdvisorCounsel>);
static_assert(!std::is_default_constructible_v<BnwFreshAdvisorRecommender>);
static_assert(!std::is_constructible_v<BnwFreshAdvisorCounsel,std::shared_ptr<const BnwCreationConfig>>);
static_assert(!std::is_constructible_v<BnwFreshAdvisorRecommender,std::shared_ptr<const BnwCreationConfig>>);
}
int main() {
    try {
        counsel();recommender();
        std::cout<<"PASS detached native Advisor constructor storage; genuine Game issuance UNOBSERVED\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL Advisor constructor storage: "<<error.what()<<'\n';return 1;
    }
}
