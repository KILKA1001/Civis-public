#include "civ5/ui/detail/bnw_native_resource_bucket_order.hpp"
#include <fstream>
#include <iostream>
#include <array>
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::uint32_t read(std::istream& in){
    std::array<unsigned char,4> b{};in.read(reinterpret_cast<char*>(b.data()),b.size());
    need(static_cast<bool>(in),"Truncated original table fixture");
    return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
}
}
int main(int argc,char** argv){try{
    need(argc==2,"Pinned original bucket CPU fixture required");std::ifstream input(argv[1],std::ios::binary);
    std::array<char,8> magic{};input.read(magic.data(),magic.size());
    need(std::string(magic.data(),magic.size())=="BNWBKT01","Original bucket fixture schema differs");
    need(read(input)==3,"Bounded original cases differ");std::size_t steps=0;
    using Buckets=civ5::ui::detail::NativeResourceBucketOrder;
    for(unsigned c=0;c<3;++c){
        const auto config=read(input);auto table=Buckets::diagnostic_pool(config);const auto count=read(input);
        need(count<=100,"Unbounded original fixture");
        for(unsigned i=0;i<count;++i){
            const auto op=read(input),key=read(input),value=read(input),expected=read(input),pool=read(input),counter=read(input),n=read(input);
            need(op==1||op==2,"Unowned table fixture operation");need(n<=100,"Unbounded table fixture nodes");
            std::vector<Buckets::Node> order;for(unsigned j=0;j<n;++j){const auto k=read(input),v=read(input);order.push_back({k,v});}
            const auto actual=op==1?table.insert(key,value):table.erase_first(key);
            need(actual==expected,"Original table insert/delete return differs");
            need(table.pool_size()==pool && table.source_count_field()==counter,"Original table growth/count field differs");
            need(table.live_nodes()==n && table.ordered_nodes()==order,"Original82B1E0 bucket/link iteration differs");++steps;
        }
    }
    need(input.peek()==std::char_traits<char>::eof(),"Trailing original bucket fixture bytes");
    need(steps==99,"Original bounded transition count differs");
    std::cout<<"PASS original native bucket allocation/tail insertion/delete/growth/link order: "<<steps<<" steps; actual namespace readiness remains pending\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
