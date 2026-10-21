#include "../src/content/bnw_info_type_cache_source.hpp"
#include "civ5/runtime/crc32_byte.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <thread>
using civ5::content::RulesDatabase;
using civ5::content::saved_info_type_hash;
using namespace civ5::content::info_type_cache_detail;
namespace {
int checks = 0;
void need(bool value, const char* why) { ++checks; if (!value) throw std::runtime_error(why); }
template<class F> void refuse(F&& action) {
    bool failed=false; try { action(); } catch(const std::exception&) { failed=true; }
    need(failed,"Expected refusal");
}
std::shared_ptr<RulesDatabase> sql(std::string_view source) {
    auto database=std::make_shared<RulesDatabase>(); database->apply_sql(std::string(source)); return database;
}
void crc_compatibility() {
    const auto bit_step=[](std::uint32_t state,unsigned char byte){
        state^=byte;
        for(unsigned bit=0;bit<8;++bit)state=(state>>1)^((state&1)?0xedb88320u:0u);
        return state;
    };
    // All byte values and 32 independent accumulator bits, including the
    // high bits that are shifted outside the table index.
    for(unsigned bit=0;bit<32;++bit)for(unsigned byte=0;byte<256;++byte){
        const auto state=(std::uint32_t{1}<<bit)^0xa5c35a3cu;
        need(civ5::runtime::crc32_update_byte(state,static_cast<unsigned char>(byte))==
             bit_step(state,static_cast<unsigned char>(byte)),"CRC byte recurrence changed");
    }
    need(saved_info_type_hash("")==0xffffffffu,"Empty type hash changed");
    // The save-format hash is case-sensitive and accepts every nonzero byte;
    // it must not inherit the admission hash's ASCII/case/length restrictions.
    for(unsigned a=1;a<256;++a)for(unsigned b=1;b<256;++b){
        const char bytes[]={static_cast<char>(a),static_cast<char>(b)};
        const auto expected=bit_step(bit_step(0xffffffffu,static_cast<unsigned char>(a)),static_cast<unsigned char>(b));
        need(saved_info_type_hash(std::string_view(bytes,2))==expected,"Saved type byte pair hash changed");
    }
    std::string long_name;std::uint32_t expected=0xffffffffu;
    for(unsigned n=0;n<1024;++n){
        const auto byte=static_cast<unsigned char>(1+n%255);
        long_name.push_back(static_cast<char>(byte));expected=bit_step(expected,byte);
    }
    need(saved_info_type_hash(long_name)==expected,"Long saved type hash changed");
    for(const auto position:{std::size_t{0},std::size_t{512},std::size_t{1023}}){
        auto with_nul=long_name;with_nul[position]='\0';
        refuse([&]{(void)saved_info_type_hash(with_nul);});
    }
}
void scalar_order_reset() {
    auto database=sql("CREATE TABLE ZFirst(ID,Type);"
        "INSERT INTO ZFirst VALUES(4294967301,'DUP'),(-7,'NEGATIVE'),(99,NULL),"
        "(NULL,'NULL_ID'),('42tail','TEXT_ID'),(10.75,'REAL_ID'),(5,''),"
        "(6,'Case'),(7,'CASE'),(8,'TYPE_4978D320478D8056'),"
        "(77,'prefix'||char(0)||'suffix'),(-9223372036854775808,'MIN64'),"
        "(X'34327461696C','BLOB_ID'),(90,X'424C4F425F54595045');"
        "CREATE TABLE ASecond(ID,Type); INSERT INTO ASecond VALUES(12,'DUP'),(11,'DUP'),"
        "(9223372036854775807,'WIDE'),(13,'TYPE_5348478E677AAFE1'),(-1,'NONE');"
        "CREATE TABLE Empty(ID,Type); CREATE TABLE NoId(Type); INSERT INTO NoId VALUES('IGNORE');"
        "CREATE TABLE NoType(ID); INSERT INTO NoType VALUES(123);"
        "CREATE TABLE Lowercase(id,type); INSERT INTO Lowercase VALUES(123,'LOWER');"
        "CREATE VIEW ViewOnly AS SELECT 200 AS ID,'VIEW' AS Type;"
        "CREATE TEMP TABLE TempOnly(ID,Type); INSERT INTO TempOnly VALUES(300,'TEMP');");
    LeafState state; reset_and_enumerate(database,state);
    const auto& report=state.report();
    need(report.reset_performed && report.inventory_exhausted && report.tables.size()==6,"Main inventory filtering");
    need(report.tables[0].name=="ZFirst" && report.tables[1].name=="ASecond","Inventory is not alphabetical");
    need(report.tables[0].sql=="select * from ZFirst ","Native SelectAll formatter");
    need(state.writes()[0].id==5 && state.lookup("DUP")==11,"Low32 int and last visited overwrite");
    need(state.writes()[0].type=="DUP" && state.writes()[1].type=="NEGATIVE","Source row order retained");
    need(state.lookup("WIDE")==-1 && state.names().contains("WIDE"),"Stored ID-1 is distinct from missing");
    need(state.lookup("MIN64")==0 && state.lookup("NEGATIVE")==-7,"Native signed32 lowbits");
    need(state.lookup("NULL_ID")==0 && state.lookup("TEXT_ID")==42 && state.lookup("REAL_ID")==10,"SQLite ID conversions");
    need(state.lookup("BLOB_ID")==42 && state.lookup("BLOB_TYPE")==90,"SQLite BLOB conversion not Lua nil");
    need(state.lookup("")==5 && state.lookup("Case")==6 && state.lookup("CASE")==7 && state.lookup("case")==-1,"Exact case and empty strings");
    need(state.lookup("LOWER")==-1 && state.lookup("VIEW")==-1 && state.lookup("TEMP")==-1 && state.lookup("IGNORE")==-1,"Ineligible/temp/view omitted");
    need(!report.tables[2].checked_columns && report.tables[2].rows==0,"Empty table is not column-checked");
    need(report.tables[3].checked_columns && !report.tables[3].eligible && report.tables[3].rows==1,"Missing ID first row breaks");
    need(report.tables[4].checked_columns && !report.tables[4].eligible,"Missing Type first row breaks");
    need(state.lookup(nullptr)==-1 && state.lookup_hash(0)==-1 && state.lookup("UNKNOWN")==-1,"Source failed lookups return-1");
    need(state.lookup(std::string_view("prefix\0different",16))==77 && state.lookup_hash(saved_info_type_hash("prefix"))==77,"C-string prefix");
    constexpr auto a="TYPE_4978D320478D8056",b="TYPE_5348478E677AAFE1";
    need(saved_info_type_hash(a)==0x2b0c0da6 && saved_info_type_hash(b)==0x2b0c0da6,"Authentic CRC collision");
    need(state.lookup(a)==8 && state.lookup(b)==13 && state.lookup_hash(0x2b0c0da6)==13,"Independent string/hash overwrite");
    need(state.hashes().size()+1==state.names().size() && report.null_types==1,"NULL is skipped and collision replaces hash");
    const auto generic=database->cache_info_types();
    need(generic.find("WIDE")->id==INT64_MAX && generic.find("MIN64")->id==INT64_MIN,"Generic wide diagnostic intentionally separate");
    for(const auto& [name,entry]:state.names()) {
        const auto* existing=generic.find(name);
        need(existing && existing->table==entry.table && existing->hash==entry.hash,"Reuse existing catalog/hash behavior");
    }
    database->apply_sql("DELETE FROM ZFirst; DELETE FROM ASecond; INSERT INTO ZFirst VALUES(4294967301,'DUP');");
    need(state.lookup("DUP")==11,"Stored source registry is not final SQL lookup");
    reset_and_enumerate(database,state);
    need(state.lookup("DUP")==5 && state.lookup("WIDE")==-1 && !state.names().contains("WIDE"),"Actual reset removes prior maps");
    need(state.lookup_hash(0x2b0c0da6)==-1 && state.writes().size()==1,"Reset clears hash and source writes");
}
void table_failures() {
    auto database=sql("CREATE TABLE Broken(ID,Type); INSERT INTO Broken VALUES(1,'WRONG');"
        "CREATE TEMP VIEW Broken AS SELECT * FROM Absent;"
        "CREATE TABLE Partial(ID,Type); CREATE TEMP VIEW Partial AS "
        "SELECT 31 AS ID,'PREFIX' AS Type UNION ALL SELECT abs(-9223372036854775808),'ERROR';"
        "CREATE TABLE Good(ID,Type); INSERT INTO Good VALUES(55,'GOOD');"
        "CREATE TABLE \"Table With Spaces\"(ID,Type); INSERT INTO \"Table With Spaces\" VALUES(66,'SPACES');"
        "CREATE TABLE \"Quote'Table\"(ID,Type); INSERT INTO \"Quote'Table\" VALUES(67,'QUOTED');");
    LeafState state; reset_and_enumerate(database,state);
    const auto& report=state.report();
    need(report.inventory_exhausted && report.tables.size()==5,"Failure does not stop inventory");
    need(report.tables[0].failure==TableVisit::Failure::prepare && state.lookup("WRONG")==-1,"Failed table preparation skipped");
    need(report.tables[1].failure==TableVisit::Failure::step && !report.tables[1].error.empty(),"Actual SQLite step error observed");
    need(state.lookup("PREFIX")==31 && state.lookup("ERROR")==-1 && state.lookup("GOOD")==55,"Prefix retained and later tables visited");
    need(report.tables[3].failure==TableVisit::Failure::prepare && state.lookup("SPACES")==-1,"Raw native table name not host identifier quote");
    need(report.tables[4].sql=="select * from Quote''Table " && report.tables[4].failure==TableVisit::Failure::prepare,"Native percent-q doubles apostrophe without surrounding quotes");
    database->apply_sql("DROP VIEW temp.Broken; CREATE TEMP TABLE Broken(ID,Type); INSERT INTO temp.Broken VALUES(3,'SHADOW');");
    reset_and_enumerate(database,state);
    need(state.lookup("SHADOW")==3 && state.lookup("GOOD")==55,"Native ordinary temporary-object resolution");
}
void query_buffer() {
    const std::string fits(1009,'T'), truncates=fits+"X", too_long(1600,'Q');
    auto database=sql("CREATE TABLE "+fits+"(ID,Type); INSERT INTO "+fits+" VALUES(71,'FITS');"
        "CREATE TABLE "+truncates+"(ID,Type); INSERT INTO "+truncates+" VALUES(72,'TRUNCATED');"
        "CREATE TABLE "+too_long+"(ID,Type); INSERT INTO "+too_long+" VALUES(73,'TOO_LONG');");
    LeafState state; reset_and_enumerate(database,state);
    need(state.report().tables[0].sql.size()==1023 && state.lookup("FITS")==71,"Exact 0x400 snprintf byte boundary");
    need(state.report().tables[1].sql==state.report().tables[0].sql && state.writes().size()==2,"Truncated name resolves same prefix table");
    need(state.writes()[1].table==truncates && state.writes()[1].type=="FITS" && state.lookup("TRUNCATED")==-1,"Retain native truncation alias behavior");
    need(state.report().tables[2].sql.size()==1023 && state.report().tables[2].failure==TableVisit::Failure::prepare && state.lookup("TOO_LONG")==-1,"Long SQL failure differs from widened generic cache");
}
void interrupted_prefix() {
    auto database=sql("CREATE TABLE A(ID,Type); INSERT INTO A VALUES(1,'ONE'),(2,'TWO'),(3,'THREE'); CREATE TABLE B(ID,Type); INSERT INTO B VALUES(4,'FOUR');");
    LeafState state; reset_and_enumerate(database,state);
    refuse([&]{reset_and_enumerate(database,state,{10,2});});
    need(state.lookup("ONE")==1 && state.lookup("TWO")==2 && state.lookup("THREE")==-1 && state.lookup("FOUR")==-1,"Budget interruption retains new source prefix");
    need(state.writes().size()==2 && state.report().rows==2 && !state.report().inventory_exhausted,"No false enumeration completion");
    refuse([&]{reset_and_enumerate(database,state,{0,10});});
    need(state.names().empty() && state.hashes().empty() && state.report().reset_performed,"Reset survives refusal before first table");
    reset_and_enumerate(database,state);
    refuse([&]{reset_and_enumerate({},state);});
    need(state.lookup("FOUR")==4 && state.writes().size()==4,"Null owner refusal precedes source reset");
    database->apply_sql("DROP TABLE A; DROP TABLE B;"); reset_and_enumerate(database,state);
    need(state.report().reset_performed && state.report().inventory_exhausted && state.names().empty(),"Empty inventory still resets maps");
}
void nul_prefixes() {
    auto database=sql("CREATE TABLE Keys(ID,Type); INSERT INTO Keys VALUES"
        "(1,'A'||char(0)||'B'),(2,'A'||char(0)||'C'),(3,char(0)||'tail'),(4,NULL);");
    LeafState state; reset_and_enumerate(database,state);
    need(state.lookup("A")==2 && state.names().size()==2,"Embedded NUL Type uses C-string prefix and last overwrite");
    need(state.lookup_hash(saved_info_type_hash("A"))==2,"Hash uses the same Type prefix as name map");
    need(state.lookup("")==3 && state.lookup_hash(0xffffffffu)==3,"Leading NUL is a stored empty key, not NULL skip or refusal");
    need(state.writes().size()==3 && state.report().null_types==1 && state.writes()[0].type=="A" && state.writes()[2].type.empty(),"Only null pointer skips source insertion");
    need(state.lookup(std::string_view("A\0ignored",9))==2,"Length-bearing lookup also stops at embedded NUL");
}
void without_rowid() {
    auto database=sql("CREATE TABLE Ordered(K TEXT PRIMARY KEY,ID,Type) WITHOUT ROWID; INSERT INTO Ordered VALUES('z',-5,'DUP'),('a',9,'DUP');");
    LeafState state; reset_and_enumerate(database,state);
    need(state.writes().size()==2 && state.writes()[0].id==9 && state.writes()[1].id==-5 && state.lookup("DUP")==-5,"No invented ID/rowid/insertion sort");
}
void ownership_currentness() {
    std::weak_ptr<RulesDatabase> weak;
    {
        LeafState state; auto database=sql("CREATE TABLE A(ID,Type); INSERT INTO A VALUES(8,'OWNED');"); weak=database;
        database->seal_immutable_preview_fingerprints(); reset_and_enumerate(database,state); database.reset();
        need(!weak.expired() && state.lookup("OWNED")==8,"Actual SQL facade retained beyond caller reset");
    }
    need(weak.expired(),"Leaf destruction releases actual facade");
    auto database=sql("CREATE TABLE A(ID,Type); INSERT INTO A VALUES(9,'SEALED');");
    database->seal_immutable_preview_fingerprints(); LeafState state; reset_and_enumerate(database,state);
    refuse([&]{database->apply_sql("UPDATE A SET ID=10");});
    refuse([&]{(void)state.lookup("SEALED");});
    refuse([&]{reset_and_enumerate(database,state);});
    LeafState foreign; auto mutable_db=sql("CREATE TABLE A(ID,Type); INSERT INTO A VALUES(11,'THREAD');"); reset_and_enumerate(mutable_db,foreign);
    bool refused=false; std::thread thread([&]{try{(void)foreign.lookup("THREAD");}catch(const std::exception&){refused=true;}});thread.join();
    need(refused,"Thread-first refusal"); refuse([&]{(void)foreign.names();});
}
}
int main() {
    static_assert(!std::is_copy_constructible_v<LeafState> && !std::is_move_constructible_v<LeafState>);
    try { crc_compatibility();scalar_order_reset();table_failures();query_buffer();interrupted_prefix();nul_prefixes();without_rowid();ownership_currentness();
        std::cout<<"BNW info-type isolated source leaf PASS "<<checks<<" checks; no GC stage/readiness receipt\n";return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
