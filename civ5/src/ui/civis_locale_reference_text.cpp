#include "civ5/ui/civis_locale_reference_text.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <algorithm>
#include <vector>

namespace civ5::ui {
namespace {
struct Expansion {
    const std::function<std::string(std::string_view)> &resolve;
    std::vector<std::string> path;
    std::size_t visits=0,bytes=0;
    static constexpr std::size_t max_depth=32,max_visits=4096,max_bytes=1024*1024;
    void append(std::string &output,std::string_view text){
        if(text.size()>max_bytes-bytes)throw runtime::ScriptError("Civis localized reference expansion exceeds its byte budget");
        bytes+=text.size();output.append(text);
    }
    std::string expand(std::string_view key){
        if(path.size()>=max_depth||++visits>max_visits)
            throw runtime::ScriptError("Civis localized reference expansion exceeds its depth or visit budget");
        if(std::find(path.begin(),path.end(),key)!=path.end())
            throw runtime::ScriptError("Civilopedia localized reference cycle: "+std::string(key));
        path.emplace_back(key);const auto source=resolve(key);std::string result;
        for(std::size_t position=0;position<source.size();){
            const auto opening=source.find('{',position);
            if(opening==source.npos){append(result,std::string_view(source).substr(position));break;}
            append(result,std::string_view(source).substr(position,opening-position));
            const auto closing=source.find('}',opening+1);
            if(closing==source.npos)throw runtime::ScriptError("Civilopedia localized reference braces are malformed: "+std::string(key));
            const auto token=std::string_view(source).substr(opening+1,closing-opening-1);
            if(token.starts_with("TXT_KEY_")){
                if(token.size()==8||!std::all_of(token.begin(),token.end(),[](unsigned char ch){
                       return (ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='_';}))
                    throw runtime::ScriptError("Civilopedia localized reference modifier is unsupported: "+std::string(token));
                const auto nested=expand(token);append(result,nested);
            }else append(result,std::string_view(source).substr(opening,closing-opening+1));
            position=closing+1;
        }
        path.pop_back();return result;
    }
};
}
std::string expand_civis_locale_references(std::string_view key,
    const std::function<std::string(std::string_view)> &selected_source){
    if(!selected_source||key.empty()||key.find('\0')!=key.npos)
        throw runtime::ScriptError("Civis localized reference expansion requires a valid key and selected source resolver");
    Expansion expansion{selected_source,{}};return expansion.expand(key);
}
} // namespace civ5::ui
