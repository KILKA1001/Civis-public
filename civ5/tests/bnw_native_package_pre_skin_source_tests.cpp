#include "civ5/ui/bnw_native_package_pre_skin_owner.hpp"
#include <stdexcept>
#include <type_traits>
#include "../src/ui/bnw_native_package_name_source.hpp"
using civ5::ui::detail::native_mode7_replaces;
static_assert(!std::is_copy_constructible_v<civ5::ui::BnwNativePackagePreSkinOwner>);
// Source regressions for890EC0 and private descriptor retention. No native
// language/Package/category/mount capability is issued by these fixtures.
namespace {
using civ5::content::XmlNode;
using namespace civ5::ui::native_package_name_detail;
XmlNode package_name_fixture(std::vector<XmlNode> values) {
    XmlNode package; package.name="Civ5Package";
    XmlNode name; name.name="Name"; name.children=std::move(values);
    package.children.push_back(std::move(name)); return package;
}
XmlNode name_value(std::string language,std::string text) {
    XmlNode value; value.name="Value"; value.text=std::move(text);
    value.attributes.push_back({"language",std::move(language)}); return value;
}
void name_test_need(bool value,const char* why) {
    if (!value) throw std::runtime_error(why);
}
template<class F> void name_test_refuses(F&& f) {
    bool refused=false;
    try { f(); } catch (const std::runtime_error&) { refused=true; }
    name_test_need(refused,"Native name unsupported input did not refuse");
}
template<class T> concept HasPublicDescriptorIssuer = requires(const T& value) {
    value.retained_descriptor_for_pre_skin(std::string{});
};
static_assert(!HasPublicDescriptorIssuer<civ5::ui::BnwNativePackageDiscoveryOwner>);
void native_package_name_source_tests() {
    auto single=package_name_fixture({name_value("en_US","Actual English name")});
    name_test_need(language_independent_native_name(single)=="Actual English name",
                   "Single native fallback has no language dependency");
    auto invariant=package_name_fixture({name_value("english","Same"),
                                        name_value("fr_FR","Same"),name_value("EN_us","Same")});
    name_test_need(language_independent_native_name(invariant)=="Same",
                   "All native language branches should prove one raw value");
    auto varied=package_name_fixture({name_value("english","Fallback first"),
                                     name_value("en_US","US override"),name_value("fr_FR","French")});
    name_test_need(name_branch_diagnostic(varied,std::nullopt)=="Fallback first",
                   "Native retains FIRST matching fallback");
    name_test_need(name_branch_diagnostic(varied,std::string_view("EN_US"))=="US override",
                   "Current language must override an earlier fallback");
    name_test_need(name_branch_diagnostic(varied,std::string_view("fr_fr"))=="French",
                   "Native _stricmp current-language branch");
    name_test_refuses([&]{(void)language_independent_native_name(varied);});
    auto repeated=package_name_fixture({name_value("en_US","First"),name_value("EN_US","Last")});
    name_test_need(name_branch_diagnostic(repeated,std::string_view("en_us"))=="Last",
                   "Repeated exact current language is processed in authored order");
    name_test_need(name_branch_diagnostic(repeated,std::nullopt)=="First",
                   "Fallback selection cannot be overwritten by another fallback");
    name_test_refuses([&]{(void)language_independent_native_name(repeated);});
    auto nofallback=package_name_fixture({name_value("fr_FR","French")});
    name_test_refuses([&]{(void)language_independent_native_name(nofallback);});
    name_test_need(name_branch_diagnostic(single,std::string_view(""))=="Actual English name",
                   "Empty current language uses the same real fallback branch");
    auto empty_language=package_name_fixture({name_value("en_US","Good"),name_value("","Other")});
    name_test_refuses([&]{(void)language_independent_native_name(empty_language);});
    auto empty_value=package_name_fixture({name_value("en_US","")});
    name_test_refuses([&]{(void)language_independent_native_name(empty_value);});
    auto missing_attribute=package_name_fixture({name_value("en_US","Good")});
    auto ignored=name_value("fr_FR","Ignored"); ignored.attributes.clear();
    missing_attribute.children.front().children.insert(missing_attribute.children.front().children.begin(),ignored);
    name_test_need(language_independent_native_name(missing_attribute)=="Good",
                   "Missing language attribute must not fabricate a native match");
    auto whitespace=package_name_fixture({name_value("en_US"," padded ")});
    name_test_refuses([&]{(void)language_independent_native_name(whitespace);});
    auto nested=single; nested.children.front().children.front().children.push_back(XmlNode{});
    name_test_refuses([&]{(void)language_independent_native_name(nested);});
    auto nul=single; nul.children.front().children.front().text=std::string("A\0B",3);
    name_test_refuses([&]{(void)language_independent_native_name(nul);});
    auto duplicate=single; duplicate.children.push_back(duplicate.children.front());
    name_test_refuses([&]{(void)language_independent_native_name(duplicate);});
    XmlNode absent; absent.name="Civ5Package";
    name_test_need(language_independent_native_name(absent).empty(),
                   "Missing Name preserves genuine fresh constructor empty field");
    auto direct=single; direct.children.front().children.clear(); direct.children.front().text="Direct";
    name_test_need(language_independent_native_name(direct)=="Direct","Native enclosing-text branch");
    direct.children.front().text.clear();
    name_test_need(language_independent_native_name(direct).empty(),
                   "Empty enclosing Name retains fresh empty constructor output");
    // Canonical descriptor, no copy or original executable invocation. Future
    // actual execution must fail if this genuine input is missing or changed.
    const auto actual=civ5::content::read_manifest(
        "C:/CrazyProject/civ5-assets/Assets/DLC/Expansion2/Expansion2.Civ5Pkg");
    name_test_need(language_independent_native_name(actual.document)=="Expansion - Brave New World",
                   "Actual BNW Name source differs from pinned language-invariant schema");
}
}

#include "../src/ui/bnw_native_package_key_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
// Authored Source tests only. RFC/native codec synthetics are separate from the
// real pinned canonical descriptor positive below; no owner is fabricated.
namespace key_test {
using namespace civ5::ui::native_package_key_detail;
void check(bool value) {if(!value) throw std::runtime_error("Private package Key Source test failed");}
template<class F> void refuses(F action) {bool rejected=false;try{action();}catch(const std::runtime_error&){rejected=true;}check(rejected);}
std::string hex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:bytes){out.push_back(digits[value>>4]);out.push_back(digits[value&15]);}return out;
}
void arithmetic_and_codecs() {
    constexpr std::array<std::pair<std::string_view,std::string_view>,7> rfc{{
        {"","d41d8cd98f00b204e9800998ecf8427e"},{"a","0cc175b9c0f1b6a831c399e269772661"},
        {"abc","900150983cd24fb0d6963f7d28e17f72"},{"message digest","f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz","c3fcd3d76192e4007dfb496cca67e13b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789","d174ab98d277d9f5a5611c2c9f419d9f"},
        {"12345678901234567890123456789012345678901234567890123456789012345678901234567890","57edf4a22be3c955ac49da2e2107b67a"}}};
    for(const auto& [text,expected]:rfc) {Bytes bytes;append_text(text,bytes);check(hex(md5(bytes))==expected);}
    Bytes bytes{0x55};check(append_hex("",bytes)&&bytes==Bytes{0x55});
    bytes.clear();check(append_hex("Af3",bytes)&&bytes==Bytes({0xaf,0x30}));
    bytes.clear();check(!append_hex("12X",bytes)&&bytes==Bytes{0x12});
    bytes.clear();check(!append_hex("A!",bytes)&&bytes.empty());
    bytes.clear();check(!append_hex("0x12",bytes)&&bytes.empty());
    bytes.clear();check(append_text(std::string_view(" a\n\0",4),bytes)&&bytes==Bytes({32,97,10,0}));
    check(append_text("",bytes)&&bytes.size()==4);
    constexpr std::string_view guid="{6DA07636-4123-4018-B643-6575B4EC336B}";
    const Bytes expected{0x36,0x76,0xa0,0x6d,0x23,0x41,0x18,0x40,0xb6,0x43,0x65,0x75,0xb4,0xec,0x33,0x6b};
    bytes.clear();check(append_guid(guid,bytes)&&bytes==expected);
    bytes.clear();check(append_guid("{{ --6DA07636-- --4123 --4018--B6-43-65-75-B4-EC-33-6B}}tail",bytes)&&bytes==expected);
    bytes.clear();check(append_guid("00000000-0000-0000-00-00-00-00-00-00-00-F",bytes)&&bytes.size()==16&&bytes.back()==0x0f);
    bytes={0x55};check(!append_guid("{\t6DA07636-4123-4018-B643-6575B4EC336B}",bytes)&&bytes==Bytes{0x55});
    check(!append_guid("",bytes)&&bytes==Bytes{0x55});
    check(!append_guid("6DA07636-4123-4018-B643-6575B4EC336G",bytes)&&bytes==Bytes{0x55});
    for(std::size_t count:{0u,1u,15u,16u,17u}) {
        Bytes input(count,0x44);const auto out=interleave(input);check(out.size()==count+std::max(count,std::size_t{16}));
        for(std::size_t i=0;i<count;++i) check(out[2*i]==0x44&&out[2*i+1]==interleave_operand[i%16]);
        if(count<16) for(std::size_t i=count;i<16;++i) check(out[count+i]==interleave_operand[i]);
    }
    refuses([]{(void)interleaved_size(std::size_t(std::numeric_limits<std::uint32_t>::max()/2)+1);});
}
void selector_and_bl_synthetics() {
    const auto parse=[](std::string text){return civ5::content::read_xml_metadata(text);};
    const std::string start="<Civ5Package><GUID>{6DA07636-4123-4018-B643-6575B4EC336B}</GUID>";
    const std::string end="<SteamApp>235580</SteamApp><Version>1</Version>";
    auto root=parse(start+end+"<PTags><Tag Type='File'>vErSiOn</Tag><Tag Type='tExT'>Later</Tag><Tag Type='File'>LATER</Tag></PTags></Civ5Package>");
    const auto records=selectors(root);check(records.size()==4);
    check(records[0].name=="GUID"&&records[0].type==3&&records[0].required);
    check(records[1].name=="SteamApp"&&records[1].type==0&&records[1].required);
    check(records[2].name=="Version"&&records[2].type==0&&records[2].required);
    check(records[3].name=="Later"&&records[3].type==0&&!records[3].required);
    check(native_record_equal("Version","vErSiOn"));
    refuses([]{(void)native_record_equal("GUID","guid");});
    Bytes ordinary;check(descriptor_bytes(parse(start+end+"</Civ5Package>"),ordinary)&&ordinary.size()==23);
    // First GUID processed true; second fails and stops GUID siblings. Later
    // SteamApp/Version successes overwrite BL and preserve the first GUID bytes.
    Bytes duplicate;check(descriptor_bytes(parse(start+"<GUID>bad</GUID><GUID>{00000000-0000-0000-0000-000000000000}</GUID>"+end+"</Civ5Package>"),duplicate));
    check(duplicate==ordinary);
    Bytes missing;check(!descriptor_bytes(parse(start+"<Version>1</Version></Civ5Package>"),missing));
    Bytes failed;check(!descriptor_bytes(parse(start+end+"<PTags><Tag Type='Hex'>Bad</Tag><Tag Type='Text'>Absent</Tag></PTags><Bad>X</Bad></Civ5Package>"),failed));
    Bytes restored;check(descriptor_bytes(parse(start+end+"<PTags><Tag Type='Hex'>Bad</Tag><Tag Type='Text'>Later</Tag></PTags><Bad>X</Bad><Later>ok</Later></Civ5Package>"),restored));
    check(restored.size()==25&&restored[23]=='o'&&restored[24]=='k');
    refuses([&]{(void)selectors(parse(start+end+"<PTags><Tag>FilePath</Tag></PTags></Civ5Package>"));});
    refuses([&]{Bytes out;(void)descriptor_bytes(parse(start+"<guid>bad</guid>"+end+"</Civ5Package>"),out);});
    refuses([&]{Bytes out;(void)descriptor_bytes(parse(start+"<SteamApp></SteamApp><Version>1</Version></Civ5Package>"),out);});
}
void canonical_source_and_mutations(const std::filesystem::path& assets) {
    const auto path=assets/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
    const auto before=civ5::runtime::identify_file(path);
    check(before.bytes==22555&&before.sha256=="c5886615f8c15f1f78c10f61ee8c708c12834756604c361a4273a1afb86281c7");
    std::ifstream input(path,std::ios::binary);check(static_cast<bool>(input));
    const std::string original((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
    const auto manifest=civ5::content::read_manifest(path);
    const auto after=civ5::runtime::identify_file(path);check(before.bytes==after.bytes&&before.sha256==after.sha256);
    Bytes plain;check(descriptor_bytes(manifest.document,plain)&&plain.size()==23&&interleave(plain).size()==46);
    // Genuine pinned canonical SOURCE arithmetic positive; not an Original EXE
    // run or a fabricated owner/flag receipt. Expected Key comes from real XML.
    check(compare_retained_xml(manifest.document,original)==Comparison::matched);
    const auto replace=[](std::string source,std::string_view from,std::string_view to) {
        const auto pos=source.find(from);check(pos!=std::string::npos);source.replace(pos,from.size(),to);return source;
    };
    const auto compare=[](const std::string& raw){return compare_retained_xml(civ5::content::read_xml_metadata(raw),raw);};
    const auto changed=replace(original,"<SteamApp>235580</SteamApp>","<SteamApp>235581</SteamApp>");
    check(compare(changed)==Comparison::mismatched);
    // A borrowed stale parsed tree plus changed authentic raw bytes must refuse.
    refuses([&]{(void)compare_retained_xml(manifest.document,changed);});
    const auto* key=first(manifest.document,"Key");check(key);
    const auto key_element="<Key>"+key->text+"</Key>";
    check(compare(replace(original,key_element,""))==Comparison::not_attempted);
    check(compare(replace(original,key_element,"<Key>12X</Key>"))==Comparison::helpers_rejected);
    check(compare(replace(original,key_element,"<Key>F</Key>"))==Comparison::mismatched);
    refuses([&]{(void)compare(replace(original,key_element,"<Key></Key>"));});
    refuses([&]{(void)compare(replace(original,"<Version>1</Version>","<Version>&#49;</Version>"));});
    refuses([&]{(void)compare(replace(original,"<Version>1</Version>","<Version>1<!--not literal--></Version>"));});
    refuses([&]{(void)compare(replace(original,"<Version>1</Version>","<Version>1\n</Version>"));});
    refuses([&]{(void)compare(replace(original,"<Tag>Version</Tag>","<Tag Type='File'>Other</Tag>"));});
    const auto final=civ5::runtime::identify_file(path);check(final.bytes==before.bytes&&final.sha256==before.sha256);
}
}

static_assert(!std::is_invocable_v<decltype(&civ5::ui::BnwNativePackagePreSkinOwner::create),civ5::content::XmlNode>);
int main(int argc,char** argv) {
    native_package_name_source_tests();
    //Native83B130: old loose always replaces; absent cache/archive replaces;
    //otherwise signed high _stat64i32 st_mtime then unsigned low, equality replaces.
    static_assert(native_mode7_replaces(false,true,true,0,~0ULL));
    static_assert(native_mode7_replaces(true,false,true,0,~0ULL));
    static_assert(native_mode7_replaces(true,true,false,0,~0ULL));
    static_assert(!native_mode7_replaces(true,true,true,1,2));
    static_assert(native_mode7_replaces(true,true,true,2,2));
    static_assert(native_mode7_replaces(true,true,true,0x100000000ULL,0xffffffffULL));
    static_assert(!native_mode7_replaces(true,true,true,0x8000000000000000ULL,0x7fffffff00000000ULL));
    static_assert(native_mode7_replaces(true,true,true,0x00000000ffffffffULL,0x0000000080000000ULL));
    static_assert(!native_mode7_replaces(true,true,true,0x0000000080000000ULL,0x00000000ffffffffULL));
    key_test::arithmetic_and_codecs();
    key_test::selector_and_bl_synthetics();
    key_test::check(argc==2);
    const auto canonical=std::filesystem::canonical(argv[1]);
    key_test::check(canonical==std::filesystem::canonical("C:/CrazyProject/civ5-assets"));
    key_test::canonical_source_and_mutations(canonical);
    std::cout<<"PASS: private Key RFC1321/codecs/selector BL synthetics and pinned canonical descriptor SOURCE arithmetic\n";

}
