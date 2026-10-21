#include "../src/ui/bnw_native_database_validation_settings.hpp"
#include <iostream>
int main(){
    using civ5::ui::database_validation_detail::source_fields;
    auto need=[](bool b){if(!b)throw std::runtime_error("Config source fixture differs");};
    auto zero=source_fields("");need(!zero.enable_tuner&&!zero.validate_database&&zero.tuner_defaulted&&zero.validation_defaulted);
    for(auto value:{"0","1","2","-1","-2147483648","2147483647","+2"}){auto f=source_fields(std::string("[Debugging]\r\nEnableTuner=0\r\nValidateGameDatabase=")+value+"\r\n");need(f.validate_database==(std::string_view(value)!="0")&&!f.validation_defaulted&&!f.tuner_defaulted);}
    auto duplicate=source_fields("[debugging]\nVALIDATEGAMEDATABASE=0\nValidateGameDatabase=1\n[Debugging]\nEnableTuner=1\n");need(!duplicate.validate_database&&!duplicate.validation_defaulted&&duplicate.tuner_defaulted);
    for(auto value:{"2147483648","1junk","","true","+-1"}){bool refused=false;try{(void)source_fields(std::string("[Debugging]\nValidateGameDatabase=")+value);}catch(const std::invalid_argument&){refused=true;}need(refused);}
    bool refused=false;try{(void)source_fields("[ Debugging ]\nValidateGameDatabase=1");}catch(const std::invalid_argument&){refused=true;}need(refused);
    std::cout<<"PASS bounded Config bool/default/FIRST/unsupported branches\n";
}
