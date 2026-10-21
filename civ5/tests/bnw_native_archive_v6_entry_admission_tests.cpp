#include "bnw_native_archive_v6_entry_admission.hpp"
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Unsupported source branch accepted");}
template<class T>concept PublicAcquire=requires{T::acquire({});};
template<class T>concept PublicEntryAcquire=requires{T::acquire({},{},{});};
// Kept as an independent bitwise oracle for the optimized byte lookup.
std::uint32_t original_crc(std::string_view raw){
    std::uint32_t crc=0xffffffffu;
    for(const char byte:raw){
        auto c=static_cast<unsigned char>(byte);
        if(c>='A'&&c<='Z')c=static_cast<unsigned char>(c+('a'-'A'));
        crc^=c;
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return crc;
}
void crc_differential_cases(){
    for(unsigned a=32;a<=126;++a)for(unsigned b=32;b<=126;++b){
        const char bytes[]={static_cast<char>(a),static_cast<char>(b)};
        const std::string_view name(bytes,2);
        need(bnw_native_admission_ascii_crc(name)==original_crc(name),"Printable pair CRC differs from source bit loop");
    }
    // Exercise high CRC bytes and every supported length, including slash,
    // colon and mixed case. The native hash uses the full raw name.
    std::uint32_t state=0x13579bdfu;
    for(std::size_t length=1;length<=127;++length)for(unsigned sample=0;sample<32;++sample){
        std::string name(length,' ');
        for(auto& c:name){state=state*1664525u+1013904223u;c=static_cast<char>(32+(state%95));}
        need(bnw_native_admission_ascii_crc(name)==original_crc(name),"Full raw name CRC differs from source bit loop");
    }
    for(unsigned byte=0;byte<256;++byte)if(byte<32||byte>126){
        std::string name="before_after";name[6]=static_cast<char>(byte);
        refuses([&]{(void)bnw_native_admission_ascii_crc(name);});
    }
}
}
int main(){try{
    crc_differential_cases();
    static_assert(!PublicAcquire<BnwNativeArchiveEntryAdmissionArena> &&
                  !PublicEntryAcquire<BnwNativeArchiveV6EntryAdmissionOwner> &&
                  !std::is_default_constructible_v<BnwNativeArchiveEntryAdmissionArena> &&
                  !std::is_copy_constructible_v<BnwNativeArchiveV6EntryAdmissionOwner>);

    static_assert(!std::is_default_constructible_v<BnwNativeSharedFirstLineage> &&
                  !std::is_copy_constructible_v<BnwNativeSharedFirstLineage> &&
                  !std::is_default_constructible_v<BnwNativeCategoryNamespaceLineage>);
    static_assert(!std::is_constructible_v<BnwNativeSharedFirstLineage,
                  std::weak_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner>,
                  std::shared_ptr<const civ5::ui::BnwNativeEarlyInputLease>>);
    // Synthetic formula/lifetime tests: none constructs an authentic lineage.
    BnwSharedFirstNamesStorageDiagnostic shared;
    std::map<std::uint32_t,unsigned> default_records,category0_records,category1_records;
    BnwArchiveBasenameCacheStoragePolicy<unsigned> default_cache,category0_cache,category1_cache;
    const auto crc=bnw_native_admission_ascii_crc("FILE.DDS");
    shared.remember(crc,"FILE.DDS");default_records.emplace(crc,10);
    // Common belongs to the SAME default namespace. Mode0 preserves its record.
    shared.remember(crc,"file.dds");default_records.try_emplace(crc,20);
    shared.remember(crc,"File.dds");category0_records.emplace(crc,30);
    shared.remember(crc,"fILE.dds");category1_records.emplace(crc,40);
    need(shared.names().at(crc)=="FILE.DDS" && shared.names().size()==1 &&
         default_records.at(crc)==10 && category0_records.at(crc)==30 && category1_records.at(crc)==40,
         "Shared FIRST changed or namespace records merged");
    need(default_cache.insert_unique("base.fpk",10) && !default_cache.insert_unique("base.fpk",20) &&
         category0_cache.insert_unique("base.fpk",30) && category1_cache.insert_unique("base.fpk",40) &&
         *default_cache.lookup("base.fpk")==10 && *category0_cache.lookup("base.fpk")==30 &&
         *category1_cache.lookup("base.fpk")==40,
         "Namespace basename caches merged across FIRST lineage");
    auto projection=shared.names();projection.emplace(123,"new.dds");
    BnwSharedFirstNamesStorageDiagnostic::require_extension(shared.names(),projection);
    need(!shared.names().contains(123),"Uncommitted staged FIRST leaked");
    auto first_wrong=projection;first_wrong.at(crc)="file.dds";
    refuses([&]{BnwSharedFirstNamesStorageDiagnostic::require_extension(shared.names(),first_wrong);});
    first_wrong=projection;first_wrong.erase(crc);
    refuses([&]{BnwSharedFirstNamesStorageDiagnostic::require_extension(shared.names(),first_wrong);});
    shared.adopt(projection);
    need(shared.names().at(crc)=="FILE.DDS" && shared.names().at(123)=="new.dds",
         "Atomic FIRST extension lost earlier name");
    auto actual=std::make_shared<const unsigned>(1),foreign=std::make_shared<const unsigned>(1);
    std::weak_ptr<const unsigned> issued=actual;
    need(bnw_shared_first_parent_diagnostic(issued,*actual).get()==actual.get(),"SAME parent refused");
    refuses([&]{bnw_shared_first_parent_diagnostic(issued,*foreign);});
    actual.reset();refuses([&]{bnw_shared_first_parent_diagnostic(issued,*foreign);});
    BnwSharedFirstNamesStorageDiagnostic collision_first;
    collision_first.remember(0x6b70aef3u,"explosion200poundwater-002.wav");
    collision_first.remember(0x6b70aef3u,"work_boat_death.dge");
    need(collision_first.names().at(0x6b70aef3u)=="explosion200poundwater-002.wav",
         "Across-namespace CRC collision overwrote native FIRST raw basename");
    NativeResourceRecordIndex loose_namespace;
    const auto collision_crc=NativeResourceRecordIndex::source_hash("work_boat_death.dge");
    need(collision_crc==NativeResourceRecordIndex::source_hash("explosion200poundwater-002.wav"),
         "Cross-namespace collision fixture no longer collides");
    loose_namespace.first_record_names.emplace(collision_crc,"explosion200poundwater-002.wav");
    refuses([&]{loose_namespace.insert("work_boat_death.dge",
        UnreadableLoose{{},"work_boat_death.dge",2,{}},7);}); // Unsupported mode never mutates.
    refuses([&]{loose_namespace.insert("work_boat_death.dge",
        UnreadableLoose{{},"work_boat_death.dge",2,{}},1);}); // No old local record, global FIRST differs.
    need(loose_namespace.records.empty() && loose_namespace.bucket_order.live_nodes()==0 &&
         loose_namespace.first_record_names.at(collision_crc)=="explosion200poundwater-002.wav",
         "Refused global FIRST loose read changed namespace state");

    // Independent zlib CRC32 numeric goldens with final complement inverted;
    // source83B4E0 uses initialFFFFFFFF and returns RAW accumulator.
    need(bnw_native_admission_ascii_crc("file.dds")==0x7b3d8254u &&
         bnw_native_admission_ascii_crc("FILE.DDS")==0x7b3d8254u,"Native C-lower raw CRC golden differs");
    need(bnw_native_admission_ascii_crc("resource/a.dds")==0x7ab81438u &&
         bnw_native_admission_ascii_crc("a.dds")==0x5a2163cbu,"Native entry full name incorrectly became basename");
    refuses([]{bnw_native_admission_ascii_crc("");});
    refuses([]{bnw_native_admission_ascii_crc(std::string(128,'a'));});
    refuses([]{bnw_native_admission_ascii_crc(std::string_view("a\0b",3));});
    refuses([]{bnw_native_admission_ascii_crc(std::string_view("\x80",1));});
    BnwNativeArchiveEntryKernelDiagnostic kernel;
    need(kernel.admit("FILE.DDS",7,13,200)==1,"First source record not admitted");
    const BnwNativeArchiveRecordDiagnostic expected{0x7b3d8254u,13,200,0x00700000u,0x7000u};
    need(kernel.records().at(expected.crc)==expected,"Actual83B32D/330/32A observable fields differ");
    need(kernel.admit("file.dds",8,99,900)==0 && kernel.records().at(expected.crc)==expected,
         "Mode0 preserved record incorrectly replaced or counted");
    need(kernel.first_names().at(expected.crc)=="FILE.DDS","Global FIRST raw name changed on duplicate");
    need(kernel.admit("explosion200poundwater-002.wav",1,100,300)==1 &&
         kernel.admit("work_boat_death.dge",2,10,400)==0,"Native CRC-only collision mode0 differs");
    need(kernel.records().at(0x6b70aef3u).offset==300 &&
         kernel.first_names().at(0x6b70aef3u)=="explosion200poundwater-002.wav","Native collision FIRST/payload differs");
    need(kernel.admit("work_boat_death.dge",3,10,400,2)==1 &&
         kernel.records().at(0x6b70aef3u).offset==400 &&
         kernel.first_names().at(0x6b70aef3u)=="explosion200poundwater-002.wav","Pure mode2 replacement must retain global FIRST");
    // Legitimate CRC0 is not an empty-name/allocator-failure capability.
    need(bnw_native_admission_ascii_crc("zero_479_nu[w")==0 &&
         kernel.admit("zero_479_nu[w",4,1,500)==1 && kernel.records().contains(0) &&
         kernel.first_names().at(0)=="zero_479_nu[w","Supported successful CRC0 admission refused");
    need(kernel.admit("zero_479_nu[w",5,2,600)==0,"CRC0 duplicate counted");
    need(kernel.bucket_order().live_nodes()==kernel.records().size(),"Native selected namespace node count differs");
    refuses([&]{kernel.admit("unknown.dds",4096,1,700);});
    refuses([&]{kernel.admit("unknown.dds",0,1,700,7);});
    BnwNativeArchiveEntryKernelDiagnostic bounded;
    for(std::uint32_t n=0;n<NativeResourceBucketOrder::source_initial_pool;++n)
        need(bounded.admit("slot"+std::to_string(n)+".dds",0,1,100)==1,"Independent finite pool fixture CRC collision");
    const auto first_count=bounded.first_names().size();
    refuses([&]{bounded.admit("beyond_pool.dds",0,1,100);});
    need(bounded.first_names().size()==first_count && bounded.records().size()==0x7000 &&
         bounded.bucket_order().pool_size()==0x7000,"Pool refusal occurred after mutation/growth");
    need(bounded.admit("slot0.dds",0,9,999)==0,"Exhausted pool must still allow mode0 preserve");
    std::vector<civ5::assets::FpkEntry> inputs{{0,"a.dds","a.dds",14,26,3,0,0,42,1,false}};
    bnw_native_v6_entry_inputs(inputs,1,42,100);
    bnw_native_v6_entry_inputs({},0,14,14);
    refuses([&]{bnw_native_v6_entry_inputs(inputs,2,42,100);});
    auto wrong=inputs;wrong[0].padding_length=2;refuses([&]{bnw_native_v6_entry_inputs(wrong,1,42,100);});
    wrong=inputs;wrong[0].metadata_offset=25;refuses([&]{bnw_native_v6_entry_inputs(wrong,1,41,100);});
    wrong=inputs;wrong[0].ordinal=1;refuses([&]{bnw_native_v6_entry_inputs(wrong,1,42,100);});
    wrong=inputs;wrong[0].original_name="dir/a.dds";refuses([&]{bnw_native_v6_entry_inputs(wrong,1,42,100);});
    refuses([&]{bnw_native_v6_entry_inputs(inputs,1,43,100);});
    std::cout<<"PASS source83B2A0 finite mode0/CRC0/collision/FIRST/record fields and native layout inputs; genuine arena/receipt runtime acceptance remains root-owned\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
