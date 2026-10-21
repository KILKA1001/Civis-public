#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include <iostream>
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void refused(const std::filesystem::path& root,const std::filesystem::path& path){
    bool no=false;try{(void)civ5::ui::detail::confined_original_stat_path(root,path);}catch(const std::exception&){no=true;}
    need(no,"Unsafe source path accepted before stat");
}
}
int main(){try{
    const std::filesystem::path original=LR"(C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization V)";
    //Pure lexical operations never open any target or outside path.
    for(const auto* bad:{L"",L"..",L"..\\outside",L"Assets/../../outside",L"Assets\\..\\..\\outside",L"Assets\\..\\Resource\\x",
        L"C:\\outside",L"C:outside",L"\\outside",L"\\\\server\\share\\outside",L"Assets/./../outside",L"Assets/x:stream",L"Assets/trailing. /x"})
        refused(original,bad);
    const auto direct=civ5::ui::detail::confined_original_stat_path(original,L"Assets/Gameplay/XML/x.xml");
    need(direct==civ5::ui::detail::confined_original_stat_path(original,L"Assets/./Gameplay/XML/x.xml") &&
         direct==civ5::ui::detail::confined_original_stat_path(original,L"Assets\\.\\Gameplay\\XML\\x.xml"),
         "Safe dot/backslash source lexical normalization differs");
    std::cout<<"PASS original stat lexical confinement: nested traversal/rooted/drive/UNC/ADS rejected before filesystem reads; dot components normalized\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
