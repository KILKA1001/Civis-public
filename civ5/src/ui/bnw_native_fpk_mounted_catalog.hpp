#pragma once
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalColorRegistryBank;
// One actual physical FPK6 mount, owned by SAME ResourceManager. No public
// issuer: the bank must bind actual Paks query/enumeration, mount index, path,
// immutable file identity, Package/Skin/slot/world/window/epoch/loan to Current.
// This is not a complete manager-wide lookup/namespace/priority producer.
class BnwNativeFpkMountedCatalog final {
    friend class BnwNativeGlobalColorRegistryBank;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeMountedStyleReader;
    using Current=std::function<void(const std::filesystem::path&,const runtime::FileIdentity&)>;
    struct Entry {std::uint32_t hash,length,offset,packed_flags;};
    struct State {
        //83C7B0 actual constructor writes byte+280=1 before83C910.
        //No preference toggle/warm archive mutation is issued by this prefix.
        const std::uint8_t preference280=1;
        Current current;
        const std::filesystem::path path;
        const runtime::FileIdentity expected;
        const std::thread::id thread=std::this_thread::get_id();
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::mutex archive_cursor;
        std::map<std::uint32_t,std::shared_ptr<const Entry>> entries;
        std::uint32_t archive_size=0;
#if defined(_WIN32)
        HANDLE handle=INVALID_HANDLE_VALUE;
        BY_HANDLE_FILE_INFORMATION identity{};
#endif
        State(Current guard,std::filesystem::path selected,runtime::FileIdentity pin)
            :current(std::move(guard)),path(std::move(selected)),expected(std::move(pin)){}
        ~State(){
#if defined(_WIN32)
            if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);
#endif
        }
        static void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
        struct Scope {
            std::shared_ptr<State> state;
            explicit Scope(std::shared_ptr<State> s):state(std::move(s)) {
                need(state->thread==std::this_thread::get_id(),"FPK source foreign thread");
                need(!state->closed.load()&&!state->failed.load(),"FPK source retired");
                if(state->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("FPK source reentry");}
            }
            ~Scope(){state->busy.store(false);}
            Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
        };
        void check_source() {
            need(!closed.load()&&!failed.load(),"FPK owner retired before callback");
            const auto guard=current;need(bool(guard),"FPK actual mount/source issuer absent");
            guard(path,expected);
            need(!closed.load()&&!failed.load(),"FPK owner lost during callback");
#if defined(_WIN32)
            if(handle!=INVALID_HANDLE_VALUE) {
                BY_HANDLE_FILE_INFORMATION now{};
                need(GetFileInformationByHandle(handle,&now)!=0,"FPK retained handle identity unavailable");
                need(now.dwVolumeSerialNumber==identity.dwVolumeSerialNumber&&
                     now.nFileIndexHigh==identity.nFileIndexHigh&&now.nFileIndexLow==identity.nFileIndexLow&&
                     now.nFileSizeHigh==identity.nFileSizeHigh&&now.nFileSizeLow==identity.nFileSizeLow&&
                     now.ftLastWriteTime.dwHighDateTime==identity.ftLastWriteTime.dwHighDateTime&&
                     now.ftLastWriteTime.dwLowDateTime==identity.ftLastWriteTime.dwLowDateTime,
                     "FPK retained source handle changed");
            }
#endif
        }
        std::uint32_t physical_read(std::uint32_t at,std::span<std::uint8_t> out) {
#if defined(_WIN32)
            need(handle!=INVALID_HANDLE_VALUE,"FPK physical owner not open");
            need(out.size()<=UINT32_MAX&&at<=archive_size&&out.size()<=archive_size-at,"FPK read extent unsupported");
            std::lock_guard lock(archive_cursor);LARGE_INTEGER position{};position.QuadPart=at;
            need(SetFilePointerEx(handle,position,nullptr,FILE_BEGIN)!=0,"FPK native seek failed");
            DWORD got=0;need(ReadFile(handle,out.data(),static_cast<DWORD>(out.size()),&got,nullptr)!=0,"FPK native read failed");
            return got; //83CD20 underlying83C9xx/829320 actual partial read count.
#else
            (void)at;(void)out;throw std::runtime_error("FPK Windows file owner unavailable");
#endif
        }
        void exact(std::uint32_t at,std::span<std::uint8_t> out) {
            need(physical_read(at,out)==out.size(),"FPK truncated source refuses catalog publication");
        }
        std::uint32_t word(std::uint32_t& at,bool big=false) {
            std::array<std::uint8_t,4> b{};exact(at,b);at+=4;
            if(big)return std::uint32_t(b[3])|(std::uint32_t(b[2])<<8)|(std::uint32_t(b[1])<<16)|(std::uint32_t(b[0])<<24);
            return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
        }
        void verify_opened_bytes() {
#if defined(_WIN32)
            // Port source admission, not Original hashing/refcount behaviour.
            // Hash THIS retained handle incrementally; no archive copy or
            // second path-based read substitutes bytes of the opened object.
            struct Algorithm{BCRYPT_ALG_HANDLE h=nullptr;~Algorithm(){if(h)BCryptCloseAlgorithmProvider(h,0);}} algorithm;
            auto ok=[](NTSTATUS s){need(s>=0,"FPK handle source digest failed");};
            ok(BCryptOpenAlgorithmProvider(&algorithm.h,BCRYPT_SHA256_ALGORITHM,nullptr,0));
            DWORD object_size=0,returned=0;
            ok(BCryptGetProperty(algorithm.h,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&object_size),sizeof(object_size),&returned,0));
            std::vector<UCHAR> object(object_size);
            // The CNG hash object buffer must outlive BCryptDestroyHash.
            struct Hash{BCRYPT_HASH_HANDLE h=nullptr;~Hash(){if(h)BCryptDestroyHash(h);}} hash;
            ok(BCryptCreateHash(algorithm.h,&hash.h,object.data(),object_size,nullptr,0,0));
            std::array<std::uint8_t,65536> buffer{};std::uint32_t at=0;
            while(at<archive_size) {
                const auto count=std::min<std::uint32_t>(buffer.size(),archive_size-at);
                exact(at,std::span(buffer).first(count));ok(BCryptHashData(hash.h,buffer.data(),count,0));at+=count;
            }
            std::array<UCHAR,32> digest{};ok(BCryptFinishHash(hash.h,digest.data(),digest.size(),0));
            constexpr char hex[]="0123456789abcdef";std::string actual;
            for(auto byte:digest){actual+=hex[byte>>4];actual+=hex[byte&15];}
            need(expected.bytes==archive_size&&expected.sha256==actual,"FPK opened bytes differ from actual source pin");
#else
            throw std::runtime_error("FPK Windows source digest unavailable");
#endif
        }
    };
    struct Stream {
        std::weak_ptr<State> owner;
        std::shared_ptr<const Entry> entry;
        std::uint32_t position=0; //5A4310+234=0; SAME mount Entry at+23C.
    };
    std::shared_ptr<State> state_;
    static std::uint32_t name_hash(std::string name) {
#if defined(_WIN32)
        //83B4E0 native CRT lowercase and CA7810 CRC, initialFFFFFFFF/no xor.
        //CA7810 is runtime-generated, not the allzero initial PE DATA:
        //9B8060 loads EAX=EDB88320 then838BB0 publishes all256 words
        //using eight SHR1/conditionalXOR steps. This loop is that recurrence.
        ::_strlwr(name.data());std::uint32_t hash=0xffffffffu;
        for(unsigned char c:name) {
            hash^=c;for(unsigned bit=0;bit!=8;++bit)hash=(hash>>1)^((hash&1)?0xedb88320u:0u);
        }
        return hash;
#else
        (void)name;throw std::runtime_error("FPK native Windows CRT hash unavailable");
#endif
    }
    static void parse_catalog(std::shared_ptr<State> state,std::uint32_t mount_index,std::uint32_t native_catalog_argument) {
        std::uint32_t at=0;
        const auto version=state->word(at);
        //83C9D0 raw83CA0C CMP[+250],6/JNZ83CD01. Its FPK7 block is
        //unreachable through this entry; do not promote it into acceptance.
        State::need(version==6,"FPK format unsupported by actual83C9D0 entry");
        State::need(state->word(at)==0x5f4b5046u,"FPK6 native header magic mismatch");
        std::array<std::uint8_t,2> flags{};state->exact(at,flags);at+=2;
        State::need(flags[0]==0,"FPK6 native header zero byte mismatch");
        const auto count=state->word(at); //Count precedes native endian switch.
        State::need(count<=(state->archive_size-at)/20u,"FPK6 truncated record count");
        for(std::uint32_t n=0;n<count;++n) {
            const auto length=state->word(at,flags[1]!=0);
            //Native name buffer is128 bytes, zeroed before read. Unterminated
            //or overflowing native buffers are refused, not assigned a name.
            State::need(length<=128u,"FPK6 native fixed name buffer extent unsupported");
            std::array<std::uint8_t,129> name{};state->exact(at,std::span(name).first(length));
            State::need(length<128u||std::find(name.begin(),name.begin()+128,0)!=name.begin()+128,"FPK6 native name unterminated");
            const auto padded=(length+3u)&~3u;
            State::need(at<=state->archive_size&&padded+8u<=state->archive_size-at,"FPK6 name/timestamp extent truncated");
            at+=padded+8u;
            const auto size=state->word(at,flags[1]!=0),offset=state->word(at,flags[1]!=0);
            State::need(offset<=state->archive_size&&size<=state->archive_size-offset,"FPK6 entry outside SAME source archive");
            const auto key=name_hash(std::string(reinterpret_cast<const char*>(name.data())));
            auto entry=std::make_shared<Entry>(Entry{key,size,offset,mount_index<<20});
            //83B2A0 ->83B130 flags2 iff native load argument !=0. Within
            //this freshly owned one-mount catalog flags0 retains first hash,
            //flags2 replaces it. Manager-wide timestamp/preference is absent.
            if(native_catalog_argument!=0)state->entries[key]=std::move(entry);
            else state->entries.try_emplace(key,std::move(entry));
        }
    }
    explicit BnwNativeFpkMountedCatalog(Current current,std::filesystem::path actual_pak_path,
            runtime::FileIdentity source_pin,std::uint32_t actual_mount_index,std::uint32_t native_catalog_argument)
        :state_(std::make_shared<State>(std::move(current),std::move(actual_pak_path),std::move(source_pin))) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(actual_mount_index<0x1000u,"FPK genuine mount index unsupported");
            State::need(state->expected.bytes<=UINT32_MAX&&!state->expected.sha256.empty(),"FPK actual DWORD source pin absent");
            state->check_source();
#if defined(_WIN32)
            //Actual FPK catalog loader uses physical base stream; this is NOT
            //a direct-open replacement for Style's820410 namespace lookup.
            state->handle=CreateFileW(state->path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0x08000080u,nullptr);
            State::need(state->handle!=INVALID_HANDLE_VALUE,"FPK actual physical source cannot open");
            State::need(GetFileInformationByHandle(state->handle,&state->identity)!=0,"FPK opened identity absent");
            State::need(state->identity.nFileSizeHigh==0,"FPK native DWORD size domain unsupported");
            state->archive_size=state->identity.nFileSizeLow;
#else
            throw std::runtime_error("FPK native Windows mount unavailable");
#endif
            state->check_source();state->verify_opened_bytes();state->check_source();
            parse_catalog(state,actual_mount_index,native_catalog_argument);state->check_source();
        } catch(...){state->failed.store(true);throw;}
    }
    std::shared_ptr<Stream> lookup_same_mount(const char* actual_query) {
        auto state=state_;State::Scope scope(state);
        try {
            //82AD00 refuses NULL/empty input before basename/hash lookup.
            //Snapshot before source callbacks; only THIS mount can report miss.
            std::string name=actual_query?actual_query:"";
            state->check_source();
            if(name.empty()){state->check_source();return {};}
            std::replace(name.begin(),name.end(),'/','\\'); //83AE20
            auto split=name.find_last_of('\\');if(split==std::string::npos)split=name.find(':');
            if(split!=std::string::npos)name.erase(0,split+1); //81DA10/81D3B0
            const auto found=state->entries.find(name_hash(std::move(name)));
            std::shared_ptr<Stream> result;
            if(found!=state->entries.end()) {
                State::need(found->second->offset!=0,"FPK entry requires unimplemented physical fallback");
                result=std::make_shared<Stream>(Stream{state,found->second,0});
            }
            state->check_source();return result; //One complete admitted mount ONLY.
        } catch(...){state->failed.store(true);throw;}
    }
    std::uint32_t read_same_mount(std::shared_ptr<Stream> borrowed,
            std::shared_ptr<std::vector<std::uint8_t>> output,std::uint32_t requested) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(borrowed&&output,"FPK retained stream/output absent");
            const auto entry=borrowed->entry;
            State::need(borrowed->owner.lock()==state&&entry,"FPK stream foreign/expired mount");
            const auto found=state->entries.find(entry->hash);
            State::need(found!=state->entries.end()&&found->second==entry,"FPK entry identity replaced");
            auto position=borrowed->position;state->check_source();
            State::need(position<=entry->length,"FPK stream position outside native bounded domain");
            const auto count=requested==0?entry->length:requested; //83CD20 zero means full entry.
            State::need(count<=UINT32_MAX-position,"FPK native DWORD read wrap unsupported");
            const auto clipped=std::min(count,entry->length-position);
            State::need(clipped<=output->size(),"FPK actual output buffer extent insufficient");
            const auto got=clipped==0?0u:state->physical_read(entry->offset+position,std::span(*output).first(clipped));
            state->check_source();borrowed->position=position+got;return got;
        } catch(...){state->failed.store(true);throw;}
    }
    BnwNativeFpkMountedCatalog(const BnwNativeFpkMountedCatalog&)=delete;
    BnwNativeFpkMountedCatalog&operator=(const BnwNativeFpkMountedCatalog&)=delete;
public:
    ~BnwNativeFpkMountedCatalog(){state_->closed.store(true);}
};
} //namespace civ5::ui::detail
