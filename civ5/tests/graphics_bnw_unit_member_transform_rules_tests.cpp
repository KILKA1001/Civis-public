#include "civ5/graphics/bnw_unit_member_transform_rules.hpp"
#include <iostream>
#include <limits>
int main(int argc,char **argv) {
    try {
        using namespace civ5;using namespace graphics;
        const auto check=[](bool b){if(!b)throw std::runtime_error("Member transform rules check failed");};
        check(compile_bnw_unit_member_transform_rules({}, {}, 2)==BnwUnitMemberTransformRules{2,0});
        check(compile_bnw_unit_member_transform_rules(0, {}, .5f)==BnwUnitMemberTransformRules{.5f,0});
        check(compile_bnw_unit_member_transform_rules(-1, -4, 2)==BnwUnitMemberTransformRules{2,-4});
        check(compile_bnw_unit_member_transform_rules(.14f, 3, 2)==BnwUnitMemberTransformRules{.28f,3});
        unsigned rejected=0;const auto reject=[&](auto f){try{f();}catch(const std::exception &){++rejected;}};
        reject([&]{compile_bnw_unit_member_transform_rules({}, {}, 0);});
        reject([&]{compile_bnw_unit_member_transform_rules({}, {}, -1);});
        reject([&]{compile_bnw_unit_member_transform_rules(std::numeric_limits<float>::quiet_NaN(), {}, 1);});
        reject([&]{compile_bnw_unit_member_transform_rules({}, std::numeric_limits<float>::infinity(), 1);});
        reject([&]{compile_bnw_unit_member_transform_rules(2, {}, std::numeric_limits<float>::max());});
        if(argc!=2)throw std::runtime_error("Usage: member_transform_rules_tests ASSET_ROOT");
        assets::LooseSource loose(argv[1]);const auto found=loose.find(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));
        check(std::holds_alternative<assets::AssetHandle>(found));const auto &handle=std::get<assets::AssetHandle>(found);
        std::string sha;
        for(const auto *type:{"ART_DEF_UNIT_MEMBER_WARRIOR_V2","ART_DEF_UNIT_MEMBER_WARRIOR","ART_DEF_UNIT_MEMBER_WARRIOR_V4"}) {
            const auto source=read_bnw_unit_member_transform_rules(handle,type,1);
            check(source.rules==BnwUnitMemberTransformRules{.14f,0});
            if(sha.empty())sha=source.xml_sha256;else check(sha==source.xml_sha256);
            check(!source.type.empty() && !source.xml_sha256.empty());
        }
        reject([&]{read_bnw_unit_member_transform_rules(handle,"MISSING_MEMBER",1);});
        reject([&]{read_bnw_unit_member_transform_rules(handle,"",1);});
        check(rejected==7);
        std::cout<<"PASS compiled member Scale/ZOffset: native nullable defaults, positive fallback, global scale, 3 actual Warrior art types; XML "<<sha<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
