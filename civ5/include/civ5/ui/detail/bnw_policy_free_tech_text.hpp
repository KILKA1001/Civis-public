#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
namespace civ5::ui::detail {
// This is the reached installed EN token grammar only. RU [2] needs an actual
// grammatical-form owner, so it fails before chooseTech option/count effects.
inline std::string format_actual_policy_free_tech_text(std::string_view selected_template,std::string_view language,std::string_view selected_argument){
 const auto require=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
 require(language=="en_US","Free tech selected locale requires genuine native grammatical-form formatter");
 require(!selected_template.empty()&&!selected_argument.empty()&&selected_template.find('\0')==selected_template.npos&&selected_argument.find('\0')==selected_argument.npos&&
  selected_argument.find('{')==selected_argument.npos&&selected_argument.find('}')==selected_argument.npos,"Free tech actual localized argument malformed/unresolved");
 constexpr std::string_view token="{@1_BldgName}";const auto at=selected_template.find(token);
 require(at!=selected_template.npos&&selected_template.find(token,at+token.size())==selected_template.npos,"Free tech actual selected argument token unsupported");
 auto output=std::string(selected_template);output.replace(at,token.size(),selected_argument);
 require(output.find('{')==output.npos&&output.find('}')==output.npos,"Free tech actual selected formatter left unsupported grammar");return output;
}
}
