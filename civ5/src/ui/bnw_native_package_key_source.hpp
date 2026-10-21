#pragma once
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

// Private arithmetic/XML source kernel, not an owner or caller flag authority.
// Production entry is only the SAME retained descriptor in PreSkin::create.
namespace civ5::ui::native_package_key_detail {
using Bytes = std::vector<std::uint8_t>;
using Digest = std::array<std::uint8_t,16>;
inline void need(bool value,const char* why) { if(!value) throw std::runtime_error(why); }
inline int nibble(unsigned char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return c-'a'+10;
    if(c>='A'&&c<='F') return c-'A'+10;
    return -1;
}
// 891460: odd LAST nibble is high, low=0. A failed pair is not appended;
// complete preceding pairs remain, including when appending to nonempty output.
inline bool append_hex(std::string_view text,Bytes& out) {
    for(std::size_t i=0;i<text.size();i+=2) {
        const int high=nibble(static_cast<unsigned char>(text[i]));
        const int low=i+1<text.size()?nibble(static_cast<unsigned char>(text[i+1])):0;
        if(high<0||low<0) return false;
        out.push_back(static_cast<std::uint8_t>((high<<4)+low));
    }
    return true;
}
// 891400 mode0: exact borrowed FString length; no trim/case/delimiter/NUL.
inline bool append_text(std::string_view text,Bytes& out) {
    out.insert(out.end(),text.begin(),text.end()); return true;
}
inline bool guid_number(std::string_view text,std::size_t& cursor,unsigned digits,std::uint32_t& value) {
    while(cursor<text.size()&&(text[cursor]=='{'||text[cursor]=='-'||text[cursor]==' ')) ++cursor;
    if(cursor==text.size()) return false;
    value=0;
    for(unsigned i=0;i<digits && cursor<text.size();++i,++cursor) {
        const int hex=nibble(static_cast<unsigned char>(text[cursor]));
        if(hex<0) return false;
        value=(value<<4)+static_cast<std::uint32_t>(hex);
    }
    return true;
}
// 6D7530 ->4BD4D0: DWORD/WORD/WORD/eight BYTE fields. Short EOF is allowed
// by a numeric wrapper; only the last field can end short and complete the GUID.
// Local partial output is discarded on failure, unlike append_hex's prefix.
inline bool append_guid(std::string_view text,Bytes& out) {
    std::size_t cursor=0;Digest local{};std::size_t at=0;
    for(unsigned field=0;field<11;++field) {
        const unsigned bytes=field==0?4:field<3?2:1;
        std::uint32_t value=0;
        if(!guid_number(text,cursor,2*bytes,value)) return false;
        for(unsigned i=0;i<bytes;++i) local[at++]=static_cast<std::uint8_t>(value>>(8*i));
    }
    while(cursor<text.size()&&text[cursor]=='}') ++cursor;
    // Native actual null optional cursor has no required EOF validation.
    out.insert(out.end(),local.begin(),local.end());return true;
}
inline constexpr Digest interleave_operand{
    0x1f,0x33,0x93,0xfb,0x35,0x0f,0x42,0xc7,0xbd,0x50,0xbe,0x7a,0xa5,0xc2,0x61,0x81};
inline std::size_t interleaved_size(std::size_t count) {
    need(count<=std::numeric_limits<std::uint32_t>::max()/2,
         "Native Key DWORD interleave overflow requires exact overflow owner");
    return count+std::max(count,std::size_t{16});
}
inline Bytes interleave(std::span<const std::uint8_t> input) {
    const auto count=interleaved_size(input.size());
    need(input.size()<=64ULL*1024*1024,"Native Key retained XML source budget exceeded");
    Bytes out;out.reserve(count);
    for(std::size_t i=0;i<input.size();++i) {out.push_back(input[i]);out.push_back(interleave_operand[i%16]);}
    if(input.size()<16) out.insert(out.end(),interleave_operand.begin()+input.size(),interleave_operand.end());
    need(out.size()==count,"Native Key interleave count disagrees with DWORD algorithm");return out;
}
// 83F820/83F850/83F8F0/83F9C0/840060: RFC1321, exact uint32 wrapping,
// little-endian words, bit length and output. No OS/native process/hash provider.
inline Digest md5(std::span<const std::uint8_t> input) {
    static_assert(std::numeric_limits<std::uint32_t>::digits==32);
    need(input.size()<=128ULL*1024*1024,"Native Key digest supported retained-input budget exceeded");
    constexpr std::array<std::uint32_t,64> constants{
        0xd76aa478U,0xe8c7b756U,0x242070dbU,0xc1bdceeeU,0xf57c0fafU,0x4787c62aU,0xa8304613U,0xfd469501U,
        0x698098d8U,0x8b44f7afU,0xffff5bb1U,0x895cd7beU,0x6b901122U,0xfd987193U,0xa679438eU,0x49b40821U,
        0xf61e2562U,0xc040b340U,0x265e5a51U,0xe9b6c7aaU,0xd62f105dU,0x02441453U,0xd8a1e681U,0xe7d3fbc8U,
        0x21e1cde6U,0xc33707d6U,0xf4d50d87U,0x455a14edU,0xa9e3e905U,0xfcefa3f8U,0x676f02d9U,0x8d2a4c8aU,
        0xfffa3942U,0x8771f681U,0x6d9d6122U,0xfde5380cU,0xa4beea44U,0x4bdecfa9U,0xf6bb4b60U,0xbebfbc70U,
        0x289b7ec6U,0xeaa127faU,0xd4ef3085U,0x04881d05U,0xd9d4d039U,0xe6db99e5U,0x1fa27cf8U,0xc4ac5665U,
        0xf4292244U,0x432aff97U,0xab9423a7U,0xfc93a039U,0x655b59c3U,0x8f0ccc92U,0xffeff47dU,0x85845dd1U,
        0x6fa87e4fU,0xfe2ce6e0U,0xa3014314U,0x4e0811a1U,0xf7537e82U,0xbd3af235U,0x2ad7d2bbU,0xeb86d391U};
    constexpr std::array<unsigned,16> shifts{7,12,17,22,5,9,14,20,4,11,16,23,6,10,15,21};
    std::array<std::uint32_t,4> state{0x67452301U,0xefcdab89U,0x98badcfeU,0x10325476U};
    const auto block=[&](const std::uint8_t* bytes) {
        std::array<std::uint32_t,16> words{};
        for(unsigned w=0;w<16;++w) for(unsigned j=0;j<4;++j) words[w]|=std::uint32_t(bytes[4*w+j])<<(8*j);
        auto a=state[0],b=state[1],c=state[2],d=state[3];
        for(unsigned i=0;i<64;++i) {
            std::uint32_t f=0;unsigned word=0;
            if(i<16) {f=(b&c)|(~b&d);word=i;}
            else if(i<32) {f=(d&b)|(~d&c);word=(5*i+1)%16;}
            else if(i<48) {f=b^c^d;word=(3*i+5)%16;}
            else {f=c^(b|~d);word=(7*i)%16;}
            const auto previous_d=d;d=c;c=b;
            b+=std::rotl(std::uint32_t(a+f+constants[i]+words[word]),int(shifts[4*(i/16)+i%4]));a=previous_d;
        }
        state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;
    };
    const auto complete=input.size()/64;
    for(std::size_t i=0;i<complete;++i) block(input.data()+64*i);
    std::array<std::uint8_t,128> tail{};
    const auto remaining=input.size()%64;
    if(remaining) std::copy_n(input.data()+64*complete,remaining,tail.begin());
    tail[remaining]=0x80;
    const auto padded=remaining<56?64u:128u;
    const auto bits=std::uint64_t(input.size())*8;
    for(unsigned i=0;i<8;++i) tail[padded-8+i]=static_cast<std::uint8_t>(bits>>(8*i));
    block(tail.data());if(padded==128) block(tail.data()+64);
    Digest out{};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j) out[4*i+j]=static_cast<std::uint8_t>(state[i]>>(8*j));
    return out;
}
inline std::string ascii_fold(std::string_view text) {
    std::string out(text);
    for(auto& c:out) {
        need(static_cast<unsigned char>(c)>=32&&static_cast<unsigned char>(c)<127,
             "Native Key selector/type CP_ACP or XML text conversion requires source owner");
        if(c>='A'&&c<='Z') c=char(c+('a'-'A'));
    }
    return out;
}
inline bool native_record_equal(std::string_view a,std::string_view b) {
    const auto folded_a=ascii_fold(a),folded_b=ascii_fold(b);
    if(a==b) return true;
    if(folded_a!=folded_b) return false;
    // _stricmp is imported MSVCR90 current-locale code. Exact equal bytes are
    // reflexive; ordinary ASCII pairs are invariant. I/i needs the real native
    // C-locale producer, which this parent does not retain. Never guess it.
    for(std::size_t i=0;i<a.size();++i)
        need(a[i]==b[i]||folded_a[i]!='i',"Native Key I/i _stricmp needs actual retained C-locale input");
    return true;
}
inline void literal_scalar(const content::XmlNode& node) {
    need(node.children.empty(),"Native Key mixed/nested XML text requires native text producer");
    (void)ascii_fold(node.text);
    need(!node.text.empty(),"Native empty XML text outcome remains unsupported (codec empty is separately proven)");
    need(node.text.front()!=' '&&node.text.back()!=' ',"Native Key XML whitespace normalization requires source owner");
}
inline const content::XmlNode* first(const content::XmlNode& root,std::string_view selector) {
    const content::XmlNode* found=nullptr;
    for(const auto& node:root.children) {
        if(ascii_fold(node.name)==ascii_fold(selector)&&node.name!=selector)
            throw std::runtime_error("Native XML selector case matching unproved for colliding spelling");
        if(node.name==selector&&!found) found=&node;
    }
    return found;
}
struct Selector {std::string name;unsigned type;bool required;};
inline std::vector<Selector> selectors(const content::XmlNode& root) {
    std::vector<Selector> out{{"GUID",3,true},{"SteamApp",0,true},{"Version",0,true}};
    if(const auto* tags=first(root,"PTags")) for(const auto& tag:tags->children) {
        need(tag.name=="Tag","Native PTags non-Tag/case traversal requires source owner");literal_scalar(tag);
        if(std::any_of(out.begin(),out.end(),[&](const auto& prior){return native_record_equal(prior.name,tag.text);} )) continue;
        need(std::all_of(tag.text.begin(),tag.text.end(),[](unsigned char c){return
            (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';}),
            "Native Key XPath/namespace selector requires actual XML selector producer");
        unsigned type=2;bool type_seen=false;
        for(const auto& [name,value]:tag.attributes) {
            need(name=="Type"&&!type_seen,"Native PTags unsupported/repeated attribute source");type_seen=true;
            if(native_record_equal(value,"Text")) type=0;else if(native_record_equal(value,"Hex")) type=1;
            else if(native_record_equal(value,"GUID")) type=3;
            else need(native_record_equal(value,"File"),"Native Key unrecognized PTags Type requires source owner");
        }
        need(type!=2,"Native Key File/checksum/current C-locale source producer remains mandatory");
        out.push_back({tag.text,type,false});
    }
    return out;
}
inline bool descriptor_bytes(const content::XmlNode& root,Bytes& out) {
    const auto records=selectors(root);bool bl=true;
    for(const auto& record:records) {
        bool processed=false;
        (void)first(root,record.name); // Refuse unproved XML selector case collisions.
        for(const auto& node:root.children) if(node.name==record.name) {
            literal_scalar(node);
            need(node.attributes.empty(),"Native Key scalar attribute text-source shape unsupported");
            if(record.type==0) bl=append_text(node.text,out);
            else if(record.type==1) bl=append_hex(node.text,out);
            else if(record.type==3) bl=append_guid(node.text,out);
            else throw std::runtime_error("Native Key unsupported selector record type");
            if(bl) processed=true;
            if(!bl) break; // Native queries next sibling before testing BL; query has no byte append.
        }
        if(record.required&&!processed) return false;
        // A later successful selector overwrites BL; do NOT accumulate all-fields AND.
    }
    return bl;
}
enum class Comparison {not_attempted,helpers_rejected,mismatched,matched};
// File-only lexical witness: maps source elements in order to the SAME parsed
// tree, then proves each consumed scalar is literal text without comment/PI
// concatenation. It cannot issue a descriptor/lease or interpret new XML values.
inline void require_literal_source(const content::XmlNode& root,std::string_view source,
                                   const std::vector<const content::XmlNode*>& consumed) {
    struct Frame {const content::XmlNode* node;std::size_t child=0;std::string text;bool markup=false;};
    std::vector<Frame> stack;bool root_seen=false;
    const auto consumed_node=[&](const auto* node){return std::find(consumed.begin(),consumed.end(),node)!=consumed.end();};
    const auto finish=[&] {
        auto& frame=stack.back();need(frame.child==frame.node->children.size(),"Native Key source/tree child order differs");
        if(consumed_node(frame.node)) need(!frame.markup&&frame.node->children.empty()&&frame.text==frame.node->text,
            "Native Key scalar lacks exact literal SAME XML byte witness");
        stack.pop_back();
    };
    for(std::size_t cursor=0;cursor<source.size();) {
        if(source[cursor]!='<') {
            const auto next=source.find('<',cursor);const auto end=next==std::string_view::npos?source.size():next;
            if(!stack.empty()) stack.back().text.append(source.substr(cursor,end-cursor));cursor=end;continue;
        }
        if(source.substr(cursor,4)=="<!--") {
            const auto end=source.find("-->",cursor+4);need(end!=std::string_view::npos,"Native Key unterminated XML comment");
            if(!stack.empty()) stack.back().markup=true;cursor=end+3;continue;
        }
        if(source.substr(cursor,2)=="<?") {
            const auto end=source.find("?>",cursor+2);need(end!=std::string_view::npos,"Native Key unterminated XML PI");
            if(!stack.empty()) stack.back().markup=true;cursor=end+2;continue;
        }
        need(source.substr(cursor,2)!="<!","Native Key XML declaration text requires native source producer");
        char quote=0;std::size_t end=cursor+1;
        for(;end<source.size();++end) {
            const char c=source[end];if(quote) {if(c==quote)quote=0;}
            else if(c=='\''||c=='"') quote=c;else if(c=='>')break;
        }
        need(end<source.size()&&!quote,"Native Key unterminated XML token");
        auto token=source.substr(cursor+1,end-cursor-1);const bool closing=token.starts_with('/');
        if(closing) token.remove_prefix(1);
        const auto name_end=token.find_first_of(" \t\r\n/");const auto name=token.substr(0,name_end);
        need(!name.empty(),"Native Key empty XML source token");
        if(closing) {
            need(!stack.empty()&&stack.back().node->name==name,"Native Key XML source/tree close differs");finish();
        } else {
            const content::XmlNode* node=nullptr;
            if(stack.empty()) {need(!root_seen&&root.name==name,"Native Key source root differs");node=&root;root_seen=true;}
            else {
                auto& parent=stack.back();need(parent.child<parent.node->children.size(),"Native Key extra source XML element");
                node=&parent.node->children[parent.child++];need(node->name==name,"Native Key source/tree name order differs");
                parent.markup=true;
            }
            stack.push_back({node});if(token.ends_with('/')) finish();
        }
        cursor=end+1;
    }
    need(root_seen&&stack.empty(),"Native Key incomplete source XML witness");
}
inline Comparison compare_retained_xml(const content::XmlNode& root,std::string_view original_xml) {
    need(root.name=="Civ5Package","Native Key requires original DLC descriptor");
    need(original_xml.size()<=64ULL*1024*1024,"Native Key original XML budget exceeded");
    // Canonical simple ASCII operands avoid currently unproved 8352A0 entity,
    // CDATA and empty-text outcomes. This witness is read from SAME pinned file
    // by PreSkin, never a caller-provided owner input. Pure test calls grant no authority.
    need(original_xml.find('&')==std::string_view::npos &&
         original_xml.find("<![CDATA[")==std::string_view::npos &&
         original_xml.find("<!DOCTYPE")==std::string_view::npos,
         "Native Key XML entity/CDATA/DTD conversion requires native source producer");
    const auto* key=first(root,"Key");if(!key) return Comparison::not_attempted;
    literal_scalar(*key);need(key->attributes.empty(),"Native Key scalar attributes require XML source producer");
    std::vector<const content::XmlNode*> consumed{key};
    const auto records=selectors(root);
    for(const auto& record:records) for(const auto& node:root.children) if(node.name==record.name) consumed.push_back(&node);
    if(const auto* tags=first(root,"PTags")) for(const auto& tag:tags->children) consumed.push_back(&tag);
    require_literal_source(root,original_xml,consumed);
    Bytes plain,authored;
    if(!descriptor_bytes(root,plain)||!append_hex(key->text,authored)) return Comparison::helpers_rejected;
    const auto transformed=interleave(plain);const auto computed=md5(transformed);
    return authored.size()==computed.size()&&std::equal(computed.begin(),computed.end(),authored.begin())?
        Comparison::matched:Comparison::mismatched;
}
}
