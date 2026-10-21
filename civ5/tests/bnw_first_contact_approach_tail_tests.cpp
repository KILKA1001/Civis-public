#include "civ5/runtime/bnw_first_contact_approach_tail.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
using namespace civ5::runtime;
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
int main() {
    try {
        BnwApproachTailInput input;
        input.weights={500,40,390,30,20,10,400};
        input.target_human=true;input.human_war_modifier=75;
        auto tail=quote_bnw_first_contact_approach_tail(input);
        check(tail.gate==BnwApproachTailGate::quoted && tail.approach==6 &&
              tail.weights[0]==375 && tail.weights[2]==292 &&
              tail.war_face==-1 && !tail.war_face_write_required &&
              !tail.can_commit(),"Human handicap did not modify WAR and DECEPTIVE before sorting");
        input.target_human=false;
        tail=quote_bnw_first_contact_approach_tail(input);
        check(tail.approach==0 && tail.war_face==1 &&
              tail.war_face_write_required,"WAR with NEUTRAL runner-up did not stage neutral face");
        input.weights[1]=450;
        check(quote_bnw_first_contact_approach_tail(input).war_face==0,
              "HOSTILE runner-up did not produce hostile WarFace");
        input.weights[1]=40;input.weights[2]=450;
        check(quote_bnw_first_contact_approach_tail(input).war_face==2,
              "DECEPTIVE runner-up did not produce friendly WarFace");
        input.denounced_target=true;
        check(quote_bnw_first_contact_approach_tail(input).war_face==1,
              "Denouncement did not neutralize friendly WarFace");
        input.existing_war_face=0;
        tail=quote_bnw_first_contact_approach_tail(input);
        check(tail.war_face==0 && !tail.war_face_write_required,
              "Approach tail replaced an already chosen WarFace");
        input.existing_war_face=-1;input.denounced_target=false;
        input.weights[1]=450;
        check(quote_bnw_first_contact_approach_tail(input).gate==
                  BnwApproachTailGate::native_sort_tie_unverified,
              "Different second-place WarFaces silently resolved a native sort tie");
        input.weights[1]=40;input.weights[5]=450;
        tail=quote_bnw_first_contact_approach_tail(input);
        check(tail.gate==BnwApproachTailGate::quoted && tail.war_face==2,
              "Observable-equivalent second-place tie failed to compute WarFace");
        input.weights[2]=500;
        check(quote_bnw_first_contact_approach_tail(input).gate==
                  BnwApproachTailGate::native_sort_tie_unverified,
              "Tied best approaches silently invented a native sort order");
        input.weights={INT32_MAX,1,2,3,4,5,6};
        input.target_human=true;input.human_war_modifier=101;
        check(quote_bnw_first_contact_approach_tail(input).gate==
                  BnwApproachTailGate::native_arithmetic_unavailable,
              "Native int handicap multiplication overflow was accepted");
        input.weights={-101,1,-199,3,4,5,6};input.human_war_modifier=50;
        tail=quote_bnw_first_contact_approach_tail(input);
        check(tail.weights[0]==-50 && tail.weights[2]==-99,
              "Negative human handicap division failed to truncate toward zero");
        input.existing_war_face=3;
        check(quote_bnw_first_contact_approach_tail(input).gate==
                  BnwApproachTailGate::input_unavailable,
              "Invalid native WarFace was accepted");
        std::cout<<"PASS BNW first-contact approach tail\n";
    } catch(const std::exception &error) {
        std::cerr<<"FAIL BNW first-contact approach tail: "<<error.what()<<'\n';return 1;
    }
}