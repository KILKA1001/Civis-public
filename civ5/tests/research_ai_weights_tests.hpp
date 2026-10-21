#pragma once
void research_ai_weights_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    using content::TechnologyProfile;
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Technologies(ID,Type);"
        "INSERT INTO Technologies VALUES(0,'TECH_A'),(1,'TECH_B'),(5,'TECH_F');");
    const auto types=database.cache_info_types();
    for(bool vp:{false,true}) {
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
        content::TechnologyRules technologies;technologies.profile=profile;technologies.collection.slots=6;
        ResearchContext setup;setup.profile=profile;setup.players[9]={};
        saves::SavedSessionSnapshot snapshot{};snapshot.all_players.emplace().layout=layout;
        saves::SavedPlayerObservation saved{};saved.saved_player_id=9;saved.knowledge.layout=layout;
        if(vp){saves::PlayerTraitsField trait{};trait.name="m_bPermanentYieldsDecreaseEveryEra";
            trait.values={1};saved.traits.layout=layout;saved.traits.fields.push_back(trait);}
        saves::PlayerKnowledgeBlock techs{};techs.name="player_techs";
        if(vp) {
            saves::PlotArrayObservation vector{};vector.name="tech_ai_weights";vector.columns=2;
            vector.values={5,10,-1,20,0,0,1,30};techs.arrays.push_back(vector);
        } else {
            saves::TeamHashedArray vector{};vector.name="tech_ai_weights";vector.wire_count=6;
            vector.rows={{0,{17}},{content::saved_info_type_hash("TECH_A"),{10}},
                {UINT32_MAX,{77}},{content::saved_info_type_hash("TECH_B"),{20}},
                {content::saved_info_type_hash("TECH_A"),{0}},
                {content::saved_info_type_hash("TECH_F"),{50}}};
            techs.hashes.push_back(vector);
        }
        saved.knowledge.blocks.push_back(techs);snapshot.all_players->players.push_back(saved);
        const auto restored=restore_research_ai_weights(snapshot,setup,technologies,types);
        const auto &weights=restored.players.at(9);
        check(restored.profile==profile&&weights.weight_slots==(vp?4U:6U),"TechAI vector size must follow its GameCore wire format");
        if(vp) {
            check(weights.weight_at(0)==10&&weights.weight_at(1)==20&&weights.weight_at(2)==0&&
                weights.weight_at(3)==30&&!weights.weight_at(4),"VP TechAI weights use vector positions");
            const auto *elements=weights.profile_extensions.find<VpResearchAiVector>();
            check(elements&&elements->saved_elements.at(0)==5&&elements->saved_elements.at(1)==-1&&
                elements->saved_elements.at(3)==1&&elements->permanent_yields_decrease_every_era&&
                weights.unresolved_rows==0,
                "VP-only element IDs must stay behind the profile boundary");
        } else {
            check(weights.weight_at(0)==0&&weights.weight_at(1)==20&&weights.weight_at(5)==50&&
                weights.unresolved_rows==1&&!weights.weight_at(6),
                "BNW hashed weights must preserve zero holes, duplicates and unresolved hashes");
            check(!weights.profile_extensions.find<VpResearchAiVector>(),"BNW must not require VP TechAI state");
            technologies.collection.slots=5000000001ULL;
            const auto wide=restore_research_ai_weights(snapshot,setup,technologies,types);
            check(wide.players.at(9).weight_slots==5000000001ULL&&
                wide.players.at(9).weight_at(5000000000ULL)==0,
                "Sparse host TechAI weights must not allocate through a 32-bit technology ceiling");
            technologies.collection.slots=6;
            snapshot.all_players->players[0].knowledge.blocks[0].hashes[0].wire_count=-7;
            snapshot.all_players->players[0].knowledge.blocks[0].hashes[0].rows.clear();
            const auto negative=restore_research_ai_weights(snapshot,setup,technologies,types);
            check(negative.players.at(9).weight_at(5)==0,
                "Negative BNW saved count must retain the source's empty vector initialization");
            snapshot.all_players->players[0]=saved;
        }
        snapshot.all_players->players.push_back(saved);
        rejects([&]{restore_research_ai_weights(snapshot,setup,technologies,types);});
        snapshot.all_players->players.pop_back();
        if(vp) {
            snapshot.all_players->players[0].knowledge.blocks[0].arrays[0].columns=1;
            rejects([&]{restore_research_ai_weights(snapshot,setup,technologies,types);});
        } else {
            snapshot.all_players->players[0].knowledge.blocks[0].hashes[0].rows.pop_back();
            rejects([&]{restore_research_ai_weights(snapshot,setup,technologies,types);});
        }
    }
}
