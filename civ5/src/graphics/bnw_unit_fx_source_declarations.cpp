#include "civ5/graphics/bnw_unit_fx_source_declarations.hpp"
#include <algorithm>
#include <charconv>
#include <functional>
#include <stdexcept>
namespace civ5::graphics {
namespace {
void need(bool v,const char* why){if(!v)throw std::invalid_argument(why);}
std::string_view trim(std::string_view s){while(!s.empty()&&(s.front()==' '||s.front()=='\t'||s.front()=='\r'||s.front()=='\n'))s.remove_prefix(1);while(!s.empty()&&(s.back()==' '||s.back()=='\t'||s.back()=='\r'||s.back()=='\n'))s.remove_suffix(1);return s;}
std::vector<std::int32_t> event_codes(std::string_view s){std::vector<std::int32_t> out;need(s.size()<512,"FX source EC string extent");if(s.empty())return out;for(;;){const auto comma=s.find(',');auto token=trim(s.substr(0,comma));need(!token.empty(),"FX EC empty/non-stock atoi branch");std::int32_t n=0;auto result=std::from_chars(token.data(),token.data()+token.size(),n);need(result.ec==std::errc{}&&result.ptr==token.data()+token.size(),"FX EC unsupported source atoi lexical branch");out.push_back(n);need(out.size()<=128,"FX source EC cardinality");if(comma==s.npos)break;s.remove_prefix(comma+1);}return out;}

}
std::string bnw_unit_fx_native_extension(std::string_view s){need(s.find('\0')==s.npos,"FX filename NUL");const auto dot=s.rfind('.'),slash=s.rfind('\\');if(dot==s.npos||(slash!=s.npos&&slash>=dot))return {};std::string extension(s.substr(dot));for(auto& c:extension){need(static_cast<unsigned char>(c)<128,"FX extension non-ASCII branch");if(c>='A'&&c<='Z')c=char(c-'A'+'a');}return extension;}
std::vector<BnwUnitFxFileDeclaration> read_bnw_unit_fx_declarations(const content::XmlNode& root){need(root.name=="Asset","FX stock Asset root missing");std::vector<BnwUnitFxFileDeclaration> out;unsigned nodes=0;std::function<void(const content::XmlNode&,unsigned)> visit=[&](const auto& node,unsigned depth){need(++nodes<=10000&&depth<=64,"FX source traversal extent");const std::string* file=nullptr,*ec=nullptr;for(const auto& [key,value]:node.attributes){if(key=="file"&&!file)file=&value;if(key=="ec"&&!ec)ec=&value;}if(file){need(!file->empty()&&file->size()<260,"FX source file buffer extent");auto ext=bnw_unit_fx_native_extension(*file);auto kind=(ext==".dds"||ext==".bmp"||ext==".tga")?BnwUnitFxFileClass::texture:(ext==".gr2"||ext==".dge")?BnwUnitFxFileClass::granny:BnwUnitFxFileClass::other;out.push_back({node.name,*file,ec?event_codes(*ec):std::vector<std::int32_t>{},bool(ec),kind});}for(const auto& child:node.children)visit(child,depth+1);};visit(root,0);need(!out.empty(),"FX source selected-file set empty");return out;}
} // namespace civ5::graphics
