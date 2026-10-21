#include "civ5/terrain/scene_command_queue.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace civ5::terrain;
namespace {
using Bytes=std::vector<std::uint8_t>;
std::size_t checks=0,compared=0,delivered=0;
void check(bool ok,const char *message){++checks;if(!ok)throw std::runtime_error(message);}
void put(Bytes &out,std::uint32_t value){
    for(unsigned i=0;i<4;++i)out.push_back(std::uint8_t(value>>(8*i)));
}
void blob(Bytes &out,std::span<const std::uint8_t> value){
    put(out,static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(),value.begin(),value.end());
}
struct Reader {
    std::span<const std::uint8_t> data;
    std::span<const std::uint8_t> take(std::size_t size){
        check(size<=data.size(),"Truncated original scene dispatcher oracle");
        const auto value=data.first(size);data=data.subspan(size);return value;
    }
    std::uint32_t word(){
        const auto bytes=take(4);std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[i])<<(8*i);
        return value;
    }
    Bytes blob(){const auto bytes=take(word());return Bytes(bytes.begin(),bytes.end());}
};
void compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected){
    compared+=expected.size();check(actual.size()==expected.size(),"Scene dispatcher snapshot size");
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if(mismatch.first!=actual.end())
        throw std::runtime_error("Scene dispatcher byte "+std::to_string(mismatch.first-actual.begin()));
    ++checks;
}
void run(const char *path){
    std::ifstream in(path,std::ios::binary);check(bool(in),"Cannot open original scene dispatcher oracle");
    Bytes bytes((std::istreambuf_iterator<char>(in)),{});
    check(bytes.size()>=8&&
        std::string_view(reinterpret_cast<const char *>(bytes.data()),8)=="C5SCD001",
        "Wrong original scene dispatcher oracle");
    Reader oracle{std::span(bytes).subspan(8)};
    const auto cases=oracle.word();check(cases>0&&cases<=20,"Scene dispatcher case count");
    for(unsigned ci=0;ci<cases;++ci){
        const auto body=oracle.blob();Reader scenario{body};
        const auto steps=scenario.word();check(steps>0&&steps<=32,"Scene dispatcher step count");
        SceneCommandQueue queue;
        std::vector<Bytes> events;
        Bytes inject;
        for(const auto [id,owner]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
                {0x1d,0x111d},{0x20,0x1120},{0x21,0x1121}}})
            queue.register_handler(id,[&,id,owner](std::span<const std::uint8_t> payload){
                Bytes event;put(event,id);put(event,owner);
                event.insert(event.end(),payload.begin(),payload.end());
                events.push_back(std::move(event));++delivered;
                if(!inject.empty()){
                    Bytes next;next.swap(inject);queue.enqueue(next);
                }
            });
        for(unsigned si=0;si<steps;++si){
            const auto action=scenario.word();const auto packet=scenario.blob();
            if(action==1)queue.enqueue(packet);
            else if(action==2)inject=packet;
            else if(action==3){check(packet.empty(),"Drain action has a packet");queue.drain();}
            else throw std::runtime_error("Unknown scene dispatcher action");
            Bytes snapshot;put(snapshot,queue.active_bank());
            put(snapshot,static_cast<std::uint32_t>(queue.bank_bytes(0)));
            put(snapshot,static_cast<std::uint32_t>(queue.bank_bytes(1)));
            put(snapshot,static_cast<std::uint32_t>(events.size()));
            for(const auto &event:events)blob(snapshot,event);
            compare(snapshot,scenario.blob());
        }
        check(scenario.data.empty(),"Trailing scene dispatcher case");
    }
    check(oracle.data.empty(),"Trailing scene dispatcher oracle");
    std::cout<<"PASS scene dispatcher cases="<<cases<<" checks="<<checks
             <<" bytes="<<compared<<" delivered="<<delivered<<'\n';
}
void smoke(){
    SceneCommandQueue queue;
    Bytes packet(32);packet[0]=32;packet[4]=0x21;
    unsigned seen=0;
    queue.register_handler(0x21,[&](std::span<const std::uint8_t> payload){
        check(payload.size()==16,"Scene command payload extent");
        check(queue.drain()==0,"Reentrant scene command drain advanced a bank");
        if(++seen==1)queue.enqueue(packet);
    });
    queue.enqueue(packet);
    check(queue.drain()==1&&seen==1&&queue.active_bank()==1&&queue.bank_bytes(1)==32,
        "Scene command enqueued during dispatch ran too early");
    check(queue.drain()==1&&seen==2&&queue.active_bank()==0&&queue.bank_bytes(0)==0,
        "Scene command deferred to next bank was lost");
}
}
int main(int argc,char **argv){
    try{
        check(argc>=1&&argc<=2,"Usage: civ5_scene_command_queue_tests [oracle.bin]");
        smoke();
        if(argc==2)run(argv[1]);
        std::cout<<"PASS scene command queue checks="<<checks<<'\n';return 0;
    }catch(const std::exception &e){
        std::cerr<<"FAIL scene dispatcher: "<<e.what()<<'\n';return 1;
    }
}
