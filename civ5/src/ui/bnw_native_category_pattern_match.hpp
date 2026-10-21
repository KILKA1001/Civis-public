#pragma once
#include <algorithm>
#include <clocale>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
// Private pure source predicates; no Package/namespace/stat/FindA issuer API.
namespace civ5::ui::native_category_patterns_detail {
inline void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}
inline void ascii_input(std::string_view s) {
    need(s.size()<=static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
         std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32&&c<127;}),
         "Native category matcher requires printable ASCII NUL-free C-string scope");
}
// Successful ASCII/C CRT port81D1C0: last-star retry, '?' and native DOS
// empty-tail dot/star stripping. No slash normalization or regex conversion.
inline bool match_one_ascii(std::string_view text,std::string_view pattern,bool case_sensitive=false) {
    ascii_input(text);ascii_input(pattern);
    const auto* locale=std::setlocale(LC_CTYPE,nullptr);
    need(locale&&std::string_view(locale)=="C","Native category matcher requires current C CRT scope");
    const auto at=[](std::string_view s,std::size_t n)->unsigned char{return n<s.size()?static_cast<unsigned char>(s[n]):0;};
    const auto upper=[](unsigned char c){return c>='a'&&c<='z'?static_cast<unsigned char>(c-32):c;};
    const auto equal=[&](unsigned char a,unsigned char b){return case_sensitive?a==b:upper(a)==upper(b);};
    std::size_t t=0,p=0,retry_text=0,retry_pattern=0;
    //81D1F1: before first '*' mismatch can only be saved by '?'.
    while(t<text.size()&&at(pattern,p)!='*') {
        if(!equal(at(pattern,p),at(text,t))&&at(pattern,p)!='?')return false;
        ++t;++p;
    }
    //81D266: '*' remembers pattern-after-star and text+1 for next failure.
    while(t<text.size()) {
        if(at(pattern,p)=='*') {
            do{++p;if(p==pattern.size())return true;}while(at(pattern,p)=='*');
            retry_pattern=p;retry_text=t+1;
        } else if(equal(at(pattern,p),at(text,t))||at(pattern,p)=='?') {++p;++t;}
        else {
            need(retry_text!=0,"Native wildcard retry lacks reached star");
            p=retry_pattern;t=retry_text++;
        }
    }
    //81D231..258: after text NUL, strip each '.' and any following '*' run.
    // Native accepts 'a' against 'a.' and 'a.*', unlike regex/glob libraries.
    while(at(pattern,p)=='.') {++p;while(at(pattern,p)=='*')++p;}
    //81D2E3: then skip remaining '*' and require pattern NUL.
    while(at(pattern,p)=='*')++p;
    return p==pattern.size();
}
//81D100/81D5C0/81E370: strpbrk with literal ';', no whitespace trim.
// Empty tokens and a final empty token are actual patterns, not discarded.
inline bool match_list_ascii(std::string_view text,std::string_view patterns,bool case_sensitive=false) {
    ascii_input(text);ascii_input(patterns);
    std::size_t start=0;
    for(;;) {
        const auto end=patterns.find(';',start);
        if(match_one_ascii(text,patterns.substr(start,end==std::string_view::npos?patterns.size()-start:end-start),case_sensitive))return true;
        if(end==std::string_view::npos)return false;
        start=end+1;
    }
}
//5F69CC base list;5F6ACF exact format ';%s\*'. Rows MUST be the genuine
// retained native provider arguments, not guessed canonical path prefixes.
inline std::string directory_exclusion_list_ascii(std::span<const std::string> actual_provider_rows) {
    std::string out="*\\Sounds\\Speech\\*;*\\Scenarios\\*";
    for(const auto& row:actual_provider_rows) {ascii_input(row);out+=';';out+=row;out+="\\*";}
    return out;
}
inline bool directory_excluded_ascii(std::string_view actual_source_query,std::span<const std::string> actual_provider_rows) {
    return match_list_ascii(actual_source_query,directory_exclusion_list_ascii(actual_provider_rows),false);
}
//82A7A0 dispatch only. NOT a loaded/admitted File or a namespace completion.
// '/'→'\', then ONLY '*' in final basename chooses the FindA glob branch.
// A '?' without '*' is sent directly to82AA80; do not enumerate it here.
inline bool explicit_file_uses_find_a_ascii(std::string_view actual_request) {
    ascii_input(actual_request);
    const auto slash=actual_request.find_last_of("/\\");
    return actual_request.substr(slash==std::string_view::npos?0:slash+1).find('*')!=std::string_view::npos;
}
inline bool explicit_find_a_attributes_selected(unsigned attributes)noexcept {
    return (attributes&0x16u)==0; // native Hidden/System/Directory skip
}
}
