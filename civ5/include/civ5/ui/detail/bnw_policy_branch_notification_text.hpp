#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
namespace civ5::ui::detail {
inline std::string format_actual_branch_ideology_text(std::string_view selected,std::string_view language,bool met,std::string_view civilization,std::string_view branch){
 const auto need=[](bool v){if(!v)throw std::runtime_error("Ideology notification selected argument/grammar unsupported");};
 need(language=="en_US"||language=="ru_RU");
 const auto literal=[](std::string_view v){return !v.empty()&&v.find('\0')==v.npos&&v.find('{')==v.npos&&v.find('}')==v.npos;};
 need(!selected.empty()&&selected.find('\0')==selected.npos&&literal(branch)&&(!met||literal(civilization)));
 std::string result(selected);
 const auto replace=[&](std::string_view token,std::string_view value){const auto at=result.find(token);need(at!=result.npos&&result.find(token,at+token.size())==result.npos);result.replace(at,token.size(),value);};
 if(met){replace("{1_CivName}",civilization);replace("{2_TheirPolicyTree}",branch);}else replace("{1_TheirPolicyTree}",branch);
 need(result.find('{')==result.npos&&result.find('}')==result.npos);return result;
}
}
