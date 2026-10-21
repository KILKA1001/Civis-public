#pragma once
#include "civ5/graphics/bnw_warrior_team_color.hpp"
#include "civ5/runtime/pregame_roster.hpp"
namespace civ5::graphics {
// Borrow the authoritative roster/rules of the same scene revision. This is a
// fresh query, so Lua PreGame color changes are visible on the next projection.
inline BnwWarriorTeamColorReceipt project_bnw_warrior_team_color_rules(
    const runtime::PregameRoster &roster,const content::RulesDatabase &rules,
    std::int32_t player,std::int32_t unit,std::uint64_t revision) {
    const auto it=roster.slots.find(player);
    if(player<0||unit<0||it==roster.slots.end()||it->second.player_color<0||it->second.player_color_key.empty())
        throw std::invalid_argument("Missing live Warrior selected player color");
    const auto &slot=it->second;
    std::string secondary;
    const auto selected=rules.query_rows({"SELECT ID,Type,SecondaryColor FROM PlayerColors WHERE ID=? OR Type=?",{slot.player_color,slot.player_color_key}},
       [&](const content::RulesRowView &row){
           if(row.integer("ID")!=slot.player_color||row.text("Type")!=slot.player_color_key)
               throw std::invalid_argument("Foreign Warrior PlayerColors key/ID");
           secondary=row.text("SecondaryColor").value_or("");
       },2);
    if(!selected.all_succeeded()||selected.rows!=1||secondary.empty())
        throw std::invalid_argument("Missing/ambiguous current Warrior PlayerColors row");
    WarriorColor color{};
    const auto found=rules.query_rows({"SELECT Red,Green,Blue,Alpha,Red IS NOT NULL AND Green IS NOT NULL AND Blue IS NOT NULL AND Alpha IS NOT NULL AS known FROM Colors WHERE Type=?",{secondary}},
       [&](const content::RulesRowView &row){
           if(!row.integer("known"))throw std::invalid_argument("Incomplete Warrior Colors RGBA");
           color={static_cast<float>(row.real("Red")),static_cast<float>(row.real("Green")),static_cast<float>(row.real("Blue")),static_cast<float>(row.real("Alpha"))};
       },2);
    if(!found.all_succeeded()||found.rows!=1)throw std::invalid_argument("Missing/ambiguous current Warrior Colors row");
    return project_bnw_warrior_team_color({player,unit,revision,slot.player_color_key,secondary,color},player,unit,revision);
}
} // namespace civ5::graphics
