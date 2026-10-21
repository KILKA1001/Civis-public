#include "bnw_native_config_dictionary.hpp"
#include <bit>
#include <charconv>
#include <limits>
#include <stdexcept>
namespace civ5::ui::detail {
namespace {
void need(bool b,const char* m){if(!b)throw std::invalid_argument(m);}
bool same(std::string_view a,std::string_view b)noexcept{
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i){const auto lower=[](char c){return c>='A'&&c<='Z'?static_cast<char>(c+32):c;};if(lower(a[i])!=lower(b[i]))return false;}return true;
}
std::string_view trim(std::string_view s){const auto a=s.find_first_not_of(" \t\r");return a==s.npos?std::string_view{}:s.substr(a,s.find_last_not_of(" \t\r")-a+1);}
void ascii(std::string_view s){need(s.size()<=1024*1024,"Config finite stream extent unsupported");for(char c:s){const auto u=static_cast<unsigned char>(c);need(u=='\r'||u=='\n'||u=='\t'||(u>=32&&u<127),"Config finite ASCII/C locale branch unavailable");}}
bool digit(char c)noexcept{return c>='0'&&c<='9';}
struct Numeric {std::int32_t integer=0;float numeric=0;};
Numeric number(std::string_view v){
    Numeric out;const auto sign_bits=!v.empty()&&v.front()=='-'?0x80000000U:0U;
    if(!v.empty()&&(v.front()=='+'||v.front()=='-'))v.remove_prefix(1);
    // Do not infer legacy CRT special values, hexadecimal, malformed exponent
    // or partial-dot grammar from a newer host implementation.
    need(!(v.size()>=3&&(same(v.substr(0,3),"inf")||same(v.substr(0,3),"nan"))),"Config legacy CRT special value branch unavailable");
    need(!(v.size()>=2&&v[0]=='0'&&(v[1]=='x'||v[1]=='X')),"Config legacy CRT hexadecimal branch unavailable");
    std::uint64_t integer=0;const auto limit=sign_bits?2147483648ULL:2147483647ULL;
    std::size_t p=0;
    for(;p<v.size()&&digit(v[p]);++p){const auto d=static_cast<std::uint64_t>(v[p]-'0');need(integer<=(limit-d)/10,"Config finite atoi overflow branch unavailable");integer=integer*10+d;}
    out.integer=static_cast<std::int32_t>(sign_bits?-static_cast<std::int64_t>(integer):static_cast<std::int64_t>(integer));
    std::string mantissa(v.substr(0,p));std::int32_t fractional=0;
    if(p<v.size()&&v[p]=='.'){
        ++p;
        while(p<v.size()&&digit(v[p])){mantissa.push_back(v[p++]);++fractional;}
        need(!mantissa.empty(),"Config legacy CRT incomplete decimal branch unavailable");
    }
    // MSVCR90 atoi/atof no-conversion contracts both return positive0. A pure
    // alphabetic/path/quoted/empty value retains its original text and 0 caches.
    if(mantissa.empty())return out;
    std::int32_t exponent=0;
    if(p<v.size()&&(v[p]=='e'||v[p]=='E')){
        ++p;bool negative=false;
        if(p<v.size()&&(v[p]=='+'||v[p]=='-')){negative=v[p]=='-';++p;}
        need(p<v.size()&&digit(v[p]),"Config legacy CRT incomplete exponent branch unavailable");
        while(p<v.size()&&digit(v[p])){const auto d=static_cast<std::int32_t>(v[p++]-'0');need(exponent<=(64-d)/10,"Config finite exponent extent unavailable");exponent=exponent*10+d;}
        if(negative)exponent=-exponent;
    }
    // Numeric suffix is deliberately left unconsumed: both actual CRT imports
    // terminate on the first unrecognized character. Integer and float caches
    // can differ, e.g. 1.25 -> int1, float1.25; 2e1 -> int2, float20.
    const auto first=mantissa.find_first_not_of('0');
    if(first==mantissa.npos){out.numeric=std::bit_cast<float>(sign_bits);return out;}
    mantissa.erase(0,first);std::int32_t scale=exponent-fractional;
    while(mantissa.back()=='0'){mantissa.pop_back();++scale;}
    need(mantissa.size()<=19,"Config exact decimal mantissa extent unavailable");
    std::uint64_t m=0;const auto r=std::from_chars(mantissa.data(),mantissa.data()+mantissa.size(),m);
    need(r.ec==std::errc{}&&r.ptr==mantissa.data()+mantissa.size(),"Config exact mantissa overflow branch unavailable");
    if(scale<0){
        // Decimal denominator5^(-scale) must cancel ENTIRELY. Otherwise the
        // binary float is inexact and needs the unresolved native CRT/FP codec.
        for(std::int32_t i=0;i>scale;--i){need(m%5==0,"Config inexact decimal source atof/FP rounding owner unavailable");m/=5;}
    }else{
        for(std::int32_t i=0;i<scale;++i){need(m<=std::numeric_limits<std::uint64_t>::max()/5,"Config exact numeric magnitude overflow unavailable");m*=5;}
    }
    const auto zeros=std::countr_zero(m);m>>=zeros;scale+=zeros;
    const auto width=std::bit_width(m);need(width<=24,"Config inexact float significand source rounding owner unavailable");
    const auto unbiased=scale+static_cast<std::int32_t>(width)-1;
    need(unbiased<=127,"Config source atof overflow unavailable");
    std::uint32_t bits=0;
    if(unbiased>=-126){
        const auto fraction=static_cast<std::uint32_t>(m)<<(24-static_cast<unsigned>(width));
        bits=(static_cast<std::uint32_t>(unbiased+127)<<23)|(fraction&0x7fffffU);
    }else{
        need(scale>=-149,"Config source atof underflow/inexact subnormal unavailable");
        bits=static_cast<std::uint32_t>(m)<<static_cast<unsigned>(scale+149);
    }
    out.numeric=std::bit_cast<float>(bits|sign_bits);return out;
}
}
void BnwNativeConfigDictionary::load_finite_stream_payload(std::string_view text){
    need(!load_attempted_,"Config finite Load already attempted");
    //831DC5 loaded=1 BEFORE826EF0; source reset/dictionary dirty prefix is
    //retained if a later unsupported parser branch refuses. No false absence.
    load_attempted_=true;++load_generation_;sections_.clear();dirty_=true;
    try{
        ascii(text);Section* current=nullptr;
        for(std::size_t p=0;p<text.size();){
            const auto end=text.find('\n',p);auto line=trim(text.substr(p,end==text.npos?text.npos:end-p));p=end==text.npos?text.size():end+1;
            need(line.size()<1024,"Config source line buffer branch unavailable");
            if(line.empty()||line.front()==';')continue;
            if(line.front()=='['){
                need(line.size()>2&&line.back()==']',"Config finite section syntax unsupported");const auto s=line.substr(1,line.size()-2);
                need(s.find_first_of("[]")==s.npos&&trim(s)==s,"Config section bracket/spacing branch unavailable");
                //827295 creates EACH section. Later827620 chooses FIRST name.
                sections_.push_back({std::string(s),{}});current=&sections_.back();continue;
            }
            if(!current)continue; //8273C1 ignores pre-section entries.
            const auto eq=line.find('=');need(eq!=line.npos&&eq!=0,"Config finite entry syntax unsupported");
            const auto key=trim(line.substr(0,eq)),value=trim(line.substr(eq+1));
            need(!key.empty()&&key.find_first_of("[];")==key.npos,"Config finite key syntax unsupported");
            const auto n=number(value);
            current->entries.push_back({std::string(key),std::string(value),n.integer,n.numeric});
        }
        //827525 selected=NULL then82752B dirty=0 after successful stream EOF.
        dirty_=false;
    }catch(...){failed_=true;throw;}
}
BnwConfigBoolDiagnostic BnwNativeConfigDictionary::get_bool(const BnwConfigBoolOperation& op){
    try{return get_bool_impl(op);}catch(...){failed_=true;throw;}
}
BnwConfigBoolDiagnostic BnwNativeConfigDictionary::get_bool_impl(const BnwConfigBoolOperation& op){
    need(load_attempted_&&!failed_,"Config finite dictionary Load incomplete/failed");
    ascii(op.section);ascii(op.key);need(!op.section.empty()&&!op.key.empty(),"Config selected section/key empty unsupported");
    need(!auto_save_,"Config finite source auto-save effect unavailable");
    BnwConfigBoolDiagnostic out;Section* selected=nullptr;
    for(auto& section:sections_)if(same(section.name,op.section)){selected=&section;break;}
    if(!selected){sections_.push_back({op.section,{}});selected=&sections_.back();dirty_=true;out.section_inserted=true;}
    for(const auto& entry:selected->entries)if(same(entry.key,op.key)){out.found=true;out.value=entry.integer!=0;return out;}
    //827A2E default output,827EB0 mandatory append, dirty=1. Getter returns
    //lookup-false even though insertion succeeds. Defaults are native bool0/1.
    const auto n=static_cast<std::int32_t>(op.default_value);
    selected->entries.push_back({op.key,op.default_value?"1":"0",n,static_cast<float>(n)});
    dirty_=true;out.value=op.default_value;out.inserted=true;return out;
}
BnwConfigDictionaryDiagnostic diagnose_bnw_config_dictionary(std::string_view payload,std::vector<BnwConfigBoolOperation> operations){
    BnwNativeConfigDictionary dictionary;BnwConfigDictionaryDiagnostic out;
    try{dictionary.load_finite_stream_payload(payload);for(const auto& operation:operations)out.getters.push_back(dictionary.get_bool(operation));out.leaf_complete=true;}
    catch(const std::invalid_argument& e){out.refusal=e.what();}
    out.auto_save=dictionary.auto_save_;out.dirty=dictionary.dirty_;out.load_attempted=dictionary.load_attempted_;out.load_generation=dictionary.load_generation_;
    for(const auto& section:dictionary.sections_)for(const auto& entry:section.entries)out.entries.push_back({section.name,entry.key,entry.text,entry.integer,entry.numeric});
    return out;
}
}
