#pragma once
#include "civ5/ui/detail/bnw_policy_free_tech_text.hpp"
#include <stdexcept>
namespace civ5::validation {
inline void verify_policy_free_tech_actual_token_grammar(){
 using ui::detail::format_actual_policy_free_tech_text;
 if(format_actual_policy_free_tech_text("You have completed {@1_BldgName}! Choose a free technology...","en_US","Rationalism")!=
  "You have completed Rationalism! Choose a free technology...")throw std::runtime_error("Actual installed EN free-tech expansion failed");
 const auto refused=[](std::string_view text,std::string_view locale,std::string_view argument){
  try{(void)ui::detail::format_actual_policy_free_tech_text(text,locale,argument);}catch(const std::runtime_error&){return;}throw std::runtime_error("Unsupported native free-tech grammar silently accepted");
 };
 refused("Постройка {@1_BldgName[2]} завершена!","ru_RU","Рационализм");
 refused("{@1_BldgName[2]}","en_US","Rationalism");
 refused("{@1_BldgName} {2_Unknown}","en_US","Rationalism");
 refused("{@1_BldgName} {@1_BldgName}","en_US","Rationalism");
 refused("{@1_BldgName}","en_US","{unresolved}");
}
}
