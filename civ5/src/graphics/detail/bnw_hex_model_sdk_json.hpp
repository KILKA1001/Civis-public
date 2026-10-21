#pragma once
// Private SDK export reader. No Source/model authority is issued by parsing.
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::graphics::detail::hex_sdk {
[[noreturn]] inline void bad(){throw std::runtime_error("HexModel pinned SDK document is malformed or unsupported");}
struct Value final {
 enum class Kind{null_value,boolean,number,string,array,object}; Kind kind=Kind::null_value;
 bool boolean=false;double number=0;std::string string;std::vector<Value> array;std::map<std::string,Value> object;
 const Value& at(const char* key)const{if(kind!=Kind::object)bad();const auto i=object.find(key);if(i==object.end())bad();return i->second;}
 const std::vector<Value>& elements()const{if(kind!=Kind::array)bad();return array;}
 const std::string& text()const{if(kind!=Kind::string)bad();return string;}
 double real()const{if(kind!=Kind::number||!std::isfinite(number))bad();return number;}
 std::int64_t integer()const{const auto x=real();if(std::trunc(x)!=x||x<-2147483648.0||x>4294967295.0)bad();return static_cast<std::int64_t>(x);}
 bool flag()const{if(kind!=Kind::boolean)bad();return boolean;}
};
class Reader final {
 std::string_view source_;std::size_t cursor_=0,nodes_=0;
 void whitespace(){while(cursor_<source_.size()&&(source_[cursor_]==' '||source_[cursor_]=='\r'||source_[cursor_]=='\n'||source_[cursor_]=='\t'))++cursor_;}
 bool eat(char c){whitespace();if(cursor_<source_.size()&&source_[cursor_]==c){++cursor_;return true;}return false;}
 void need(char c){if(!eat(c))bad();}
 unsigned hex4(){unsigned v=0;for(unsigned i=0;i<4;++i){if(cursor_==source_.size())bad();const auto c=source_[cursor_++];v<<=4;if(c>='0'&&c<='9')v|=c-'0';else if(c>='a'&&c<='f')v|=c-'a'+10;else if(c>='A'&&c<='F')v|=c-'A'+10;else bad();}return v;}
 static void utf8(std::string& s,unsigned c){if(c<=0x7f)s+=static_cast<char>(c);else if(c<=0x7ff){s+=static_cast<char>(0xc0|(c>>6));s+=static_cast<char>(0x80|(c&63));}else if(c<=0xffff){s+=static_cast<char>(0xe0|(c>>12));s+=static_cast<char>(0x80|((c>>6)&63));s+=static_cast<char>(0x80|(c&63));}else{s+=static_cast<char>(0xf0|(c>>18));s+=static_cast<char>(0x80|((c>>12)&63));s+=static_cast<char>(0x80|((c>>6)&63));s+=static_cast<char>(0x80|(c&63));}}
 std::string string(){need('"');std::string s;while(cursor_<source_.size()){const auto c=static_cast<unsigned char>(source_[cursor_++]);if(c=='"')return s;if(c<32)bad();if(c!='\\'){s+=static_cast<char>(c);continue;}if(cursor_==source_.size())bad();switch(source_[cursor_++]){case '"':s+='"';break;case '\\':s+='\\';break;case '/':s+='/';break;case 'b':s+='\b';break;case 'f':s+='\f';break;case 'n':s+='\n';break;case 'r':s+='\r';break;case 't':s+='\t';break;case 'u':{auto v=hex4();if(v>=0xd800&&v<=0xdbff){if(cursor_+2>source_.size()||source_[cursor_++]!='\\'||source_[cursor_++]!='u')bad();const auto low=hex4();if(low<0xdc00||low>0xdfff)bad();v=0x10000+((v-0xd800)<<10)+(low-0xdc00);}else if(v>=0xdc00&&v<=0xdfff)bad();utf8(s,v);break;}default:bad();}if(s.size()>1048576)bad();}bad();}
 Value value(unsigned depth){if(depth>48||++nodes_>4000000)bad();whitespace();if(cursor_==source_.size())bad();Value v;const char c=source_[cursor_];
  if(c=='{'){++cursor_;v.kind=Value::Kind::object;if(eat('}'))return v;do{const auto key=string();need(':');if(!v.object.emplace(key,value(depth+1)).second)bad();}while(eat(','));need('}');return v;}
  if(c=='['){++cursor_;v.kind=Value::Kind::array;if(eat(']'))return v;do{v.array.push_back(value(depth+1));}while(eat(','));need(']');return v;}
  if(c=='"'){v.kind=Value::Kind::string;v.string=string();return v;}
  if(source_.substr(cursor_,4)=="null"){cursor_+=4;return v;}
  if(source_.substr(cursor_,4)=="true"){cursor_+=4;v.kind=Value::Kind::boolean;v.boolean=true;return v;}
  if(source_.substr(cursor_,5)=="false"){cursor_+=5;v.kind=Value::Kind::boolean;return v;}
  const auto begin=cursor_;if(c=='-')++cursor_;if(cursor_==source_.size())bad();if(source_[cursor_]=='0')++cursor_;else{if(source_[cursor_]<'1'||source_[cursor_]>'9')bad();while(cursor_<source_.size()&&source_[cursor_]>='0'&&source_[cursor_]<='9')++cursor_;}
  if(cursor_<source_.size()&&source_[cursor_]=='.'){++cursor_;const auto start=cursor_;while(cursor_<source_.size()&&source_[cursor_]>='0'&&source_[cursor_]<='9')++cursor_;if(start==cursor_)bad();}
  if(cursor_<source_.size()&&(source_[cursor_]=='e'||source_[cursor_]=='E')){++cursor_;if(cursor_<source_.size()&&(source_[cursor_]=='+'||source_[cursor_]=='-'))++cursor_;const auto start=cursor_;while(cursor_<source_.size()&&source_[cursor_]>='0'&&source_[cursor_]<='9')++cursor_;if(start==cursor_)bad();}
  const auto result=std::from_chars(source_.data()+begin,source_.data()+cursor_,v.number);if(result.ec!=std::errc{}||result.ptr!=source_.data()+cursor_||!std::isfinite(v.number))bad();v.kind=Value::Kind::number;return v;
 }
 public:explicit Reader(std::string_view s):source_(s){if(s.empty()||s.size()>64ULL*1024*1024)bad();}
 Value read(){auto v=value(0);whitespace();if(cursor_!=source_.size())bad();return v;}
};
inline std::vector<float> floats(const Value& v,std::size_t count){const auto& a=v.elements();if(a.size()!=count)bad();std::vector<float> out;out.reserve(count);for(const auto& x:a){const auto n=x.real();if(std::abs(n)>std::numeric_limits<float>::max())bad();out.push_back(static_cast<float>(n));}return out;}
inline std::vector<std::uint8_t> base64(std::string_view s){if(s.empty()||s.size()%4||s.size()>90ULL*1024*1024)bad();const auto digit=[](char c)->unsigned{if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='+')return 62;if(c=='/')return 63;bad();};std::vector<std::uint8_t> out;out.reserve(s.size()/4*3);for(std::size_t i=0;i<s.size();i+=4){const auto a=digit(s[i]),b=digit(s[i+1]);const bool p2=s[i+2]=='=',p3=s[i+3]=='=';if((p2&&!p3)||((p2||p3)&&i+4!=s.size()))bad();const auto c=p2?0:digit(s[i+2]),d=p3?0:digit(s[i+3]);if((p2&&(b&15))||(!p2&&p3&&(c&3)))bad();out.push_back(static_cast<std::uint8_t>((a<<2)|(b>>4)));if(!p2)out.push_back(static_cast<std::uint8_t>((b<<4)|(c>>2)));if(!p3)out.push_back(static_cast<std::uint8_t>((c<<6)|d));}return out;}
}
