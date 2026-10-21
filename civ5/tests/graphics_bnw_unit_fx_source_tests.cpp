#include "civ5/graphics/bnw_unit_fx_source_declarations.hpp"
#include <fstream>
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <iostream>
using namespace civ5;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Fixture {
 std::vector<std::uint8_t> bytes;std::size_t at=0;
 unsigned word(){check(at+4<=bytes.size(),"FX fixture extent");unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(bytes[at++])<<(i*8);return n;}
 std::string text(){auto n=word();check(n<=65536&&at+n<=bytes.size(),"FX fixture string extent");std::string s(reinterpret_cast<const char*>(bytes.data()+at),n);at+=n;return s;}
 content::XmlNode tree(unsigned depth=0){check(depth<=64,"FX fixture depth");content::XmlNode node;node.name=text();auto n=word();check(n<=128,"FX fixture attribute count");for(unsigned i=0;i<n;++i){auto k=text(),v=text();node.attributes.emplace_back(std::move(k),std::move(v));}n=word();check(n<=10000,"FX fixture child count");for(unsigned i=0;i<n;++i)node.children.push_back(tree(depth+1));return node;}
};
int main(int argc,char**argv){try{check(argc==2,"Usage: FxSource source fixture");std::ifstream f(argv[1],std::ios::binary);check(bool(f),"FX source fixture missing");Fixture fixture;fixture.bytes.assign(std::istreambuf_iterator<char>(f),{});check(fixture.bytes.size()>=12&&std::string(reinterpret_cast<const char*>(fixture.bytes.data()),8)=="BNWFXS01","FX fixture magic");fixture.at=8;check(fixture.word()==3,"FX stock model cardinality");unsigned files=0,ec=0;
 for(unsigned i=0;i<3;++i){auto model=fixture.text();auto xml=fixture.tree();auto declarations=graphics::read_bnw_unit_fx_declarations(xml);auto count=fixture.word();check(count==63&&declarations.size()==count,"Original native file-attribute traversal cardinality");unsigned idle=0,mesh=0;for(const auto& d:declarations){auto file=fixture.text();auto has=fixture.word(),n=fixture.word();std::vector<std::int32_t> expected;for(unsigned j=0;j<n;++j)expected.push_back(std::bit_cast<std::int32_t>(fixture.word()));check(file==d.file&&bool(has)==d.has_event_attribute&&expected==d.event_codes,"Original recursive/EC source declaration mismatch");++files;if(has)++ec;if(std::find(d.event_codes.begin(),d.event_codes.end(),1000)!=d.event_codes.end()){check(d.file=="Warrior_IdleA.gr2","Stock event1000 source differs");++idle;}if(!has&&graphics::bnw_unit_fx_native_extension(d.file)==".gr2")++mesh;}check(idle==1&&mesh==1,"Stock typed Warrior projections not unique");std::cout<<"ACTUAL_FX "<<model<<" files="<<count<<'\n';}
 check(fixture.at==fixture.bytes.size(),"FX trailing fixture bytes");unsigned negative=0;auto refuse=[&](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"FX unsupported declaration accepted");++negative;};
 content::XmlNode nested{"Asset",{}, {},{{"Container",{}, {},{{"NotMesh",{},{{"file","inner.GR2"},{"ec","1000"}}, {}}}}}};auto d=graphics::read_bnw_unit_fx_declarations(nested);check(d.size()==1&&d[0].extension_class==graphics::BnwUnitFxFileClass::granny,"Recursive file lookup depends on node names");
 check(graphics::bnw_unit_fx_native_extension("folder.with.dot\\model").empty()&&graphics::bnw_unit_fx_native_extension("folder\\model.GR2")==".gr2","Native last-dot/backslash extension branch");
 refuse([&]{content::XmlNode wrong;graphics::read_bnw_unit_fx_declarations(wrong);});refuse([&]{content::XmlNode empty{"Asset",{}, {},{}};graphics::read_bnw_unit_fx_declarations(empty);});refuse([&]{auto bad=nested;bad.children[0].children[0].attributes[1].second="1000,";graphics::read_bnw_unit_fx_declarations(bad);});refuse([&]{graphics::bnw_unit_fx_native_extension(std::string("x\0.gr2",6));});
 std::cout<<"PASS "<<files<<" original source file traversal + "<<ec<<" original source EC declarations, "<<negative<<" refusals; no native loader/production chain receipt\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}