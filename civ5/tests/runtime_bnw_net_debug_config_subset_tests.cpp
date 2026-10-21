#include "civ5/runtime/detail/bnw_net_debug_config_subset.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using civ5::runtime::detail::net_debug_explicit_boolean;
void expect(bool yes){if(!yes)throw std::runtime_error("Net debug explicit input test failed");}
void rejects(const std::string& input){bool denied=false;try{(void)net_debug_explicit_boolean(input);}catch(const std::invalid_argument&){denied=true;}expect(denied);}
int main(){
 expect(!net_debug_explicit_boolean(""));
 expect(!net_debug_explicit_boolean("[DEBUG]\nOther=0\n"));
 expect(!net_debug_explicit_boolean("[User Settings]\nLanguage=en_US\nLoggingEnabled=0\n"));
 auto disabled=net_debug_explicit_boolean("[DEBUG]\r\nLoggingEnabled = 0\r\n");expect(disabled.has_value()&&!(*disabled));
 auto enabled=net_debug_explicit_boolean("[debug]\nLOGGINGENABLED=1\n");expect(enabled.has_value()&&*enabled);
 rejects("[DEBUG]\nLoggingEnabled=0\nLoggingEnabled=1\n");
 rejects("[DEBUG]\nLoggingEnabled=0\n[debug]\nLoggingEnabled=1\n");
 rejects("[DEBUG]\nLoggingEnabled=false\n");rejects("[DEBUG]\nLoggingEnabled=2\n");
 rejects("[DEBUG]\nLoggingEnabled=0 ; inline ambiguity\n");
 auto nul=std::string("[DEBUG]\nLoggingEnabled=0");nul.push_back('\0');rejects(nul);
 rejects("[DEBUG\nLoggingEnabled=0\n");
 // Real override0 and original key-missing remain separate observations.
 auto original=net_debug_explicit_boolean("[User Settings]\nLanguage=en_US\n");
 auto override=net_debug_explicit_boolean("[User Settings]\nLanguage=en_US\n[DEBUG]\nLoggingEnabled=0\n");
 expect(!original&&override.has_value()&&!(*override));
 std::cout<<"PASS explicit config input facts; no manager/disabled/delivery proof\n";
}
