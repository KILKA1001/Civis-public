#include "civ5/content/mod_archive.hpp"
#include "zlib.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <utility>

namespace civ5::content {
namespace fs=std::filesystem;
namespace {
using Bytes=std::vector<std::uint8_t>;
void require(bool yes,const char *message){if(!yes)throw std::runtime_error(message);}
std::uint16_t u16(std::span<const std::uint8_t> b,std::size_t at){
    require(at<=b.size()&&b.size()-at>=2,"Truncated ZIP field");
    return std::uint16_t(b[at])|(std::uint16_t(b[at+1])<<8);
}
std::uint32_t u32(std::span<const std::uint8_t> b,std::size_t at){
    return std::uint32_t(u16(b,at))|(std::uint32_t(u16(b,at+2))<<16);
}
std::uint64_t u64(std::span<const std::uint8_t> b,std::size_t at){
    return std::uint64_t(u32(b,at))|(std::uint64_t(u32(b,at+4))<<32);
}
void span_within(std::uint64_t at,std::uint64_t length,std::uint64_t limit){
    require(at<=limit&&length<=limit-at,"ZIP span exceeds archive");
}
Bytes read_at(std::ifstream &input,std::uint64_t at,std::size_t length,std::uint64_t limit){
    span_within(at,length,limit);
    require(at<=std::uint64_t(std::numeric_limits<std::streamoff>::max()),"ZIP seek exceeds host stream");
    Bytes result(length);input.clear();input.seekg(static_cast<std::streamoff>(at));
    require(bool(input),"Cannot seek ZIP archive");
    if(length){input.read(reinterpret_cast<char*>(result.data()),static_cast<std::streamsize>(length));
        require(bool(input),"Cannot read ZIP archive");}
    return result;
}
std::wstring decode_name(std::span<const std::uint8_t> raw,bool utf8){
    require(!raw.empty()&&raw.size()<=65535&&std::find(raw.begin(),raw.end(),0)==raw.end(),
            "Invalid ZIP filename");
    const auto codepage=utf8?CP_UTF8:437u;
    const auto flags=utf8?MB_ERR_INVALID_CHARS:0u;
    const auto count=MultiByteToWideChar(codepage,flags,reinterpret_cast<const char*>(raw.data()),
        static_cast<int>(raw.size()),nullptr,0);
    require(count>0,"Invalid ZIP filename encoding");
    std::wstring result(static_cast<std::size_t>(count),L'\0');
    require(MultiByteToWideChar(codepage,flags,reinterpret_cast<const char*>(raw.data()),
        static_cast<int>(raw.size()),result.data(),count)==count,"Cannot decode ZIP filename");
    return result;
}
struct PathLess {
    bool operator()(const std::wstring &a,const std::wstring &b)const{
        return CompareStringOrdinal(a.data(),static_cast<int>(a.size()),b.data(),
            static_cast<int>(b.size()),TRUE)==CSTR_LESS_THAN;
    }
};
struct ZipEntry {
    ArchiveEntry public_entry;
    Bytes raw_name;
    std::uint64_t local_offset=0,data_offset=0,compressed=0;
    std::uint32_t crc=0;
    std::uint16_t flags=0,method=0;
};
struct WinFile {
    HANDLE handle;
    explicit WinFile(const fs::path &path):handle(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,
        CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)){
        require(handle!=INVALID_HANDLE_VALUE,"Cannot create extracted ZIP file");
    }
    ~WinFile(){CloseHandle(handle);}
    void write(const std::uint8_t *data,std::size_t size){
        while(size){const auto amount=static_cast<DWORD>(std::min<std::size_t>(size,65536));
            DWORD wrote=0;require(WriteFile(handle,data,amount,&wrote,nullptr)&&wrote==amount,
                "ZIP extracted file write failed");data+=wrote;size-=wrote;}
    }
    void flush(){require(FlushFileBuffers(handle),"ZIP extracted file flush failed");}
};
void extract_file(std::ifstream &input,std::uint64_t archive_size,const ZipEntry &entry,const fs::path &path){
    WinFile output(path);
    std::array<std::uint8_t,65536> source{},target{};
    std::uint64_t cursor=entry.data_offset,remaining=entry.compressed,produced=0;
    auto checksum=crc32(0,nullptr,0);
    const auto read_chunk=[&](std::uint8_t *dest,std::size_t size){
        span_within(cursor,size,archive_size);input.clear();input.seekg(static_cast<std::streamoff>(cursor));
        require(bool(input),"Cannot seek ZIP payload");
        input.read(reinterpret_cast<char*>(dest),static_cast<std::streamsize>(size));
        require(bool(input),"Truncated ZIP payload");cursor+=size;remaining-=size;
    };
    if(entry.method==0){
        require(entry.compressed==entry.public_entry.bytes,"Stored ZIP size mismatch");
        while(remaining){const auto n=static_cast<std::size_t>(std::min<std::uint64_t>(remaining,source.size()));
            read_chunk(source.data(),n);checksum=crc32(checksum,source.data(),static_cast<uInt>(n));
            output.write(source.data(),n);produced+=n;}
    }else{
        z_stream stream{};require(inflateInit2(&stream,-MAX_WBITS)==Z_OK,"Cannot initialize ZIP deflate");
        try{
            int result=Z_OK;
            while(result!=Z_STREAM_END){
                if(!stream.avail_in&&remaining){
                    const auto n=static_cast<std::size_t>(std::min<std::uint64_t>(remaining,source.size()));
                    read_chunk(source.data(),n);stream.next_in=source.data();stream.avail_in=static_cast<uInt>(n);
                }
                const auto before=stream.avail_in;
                stream.next_out=target.data();stream.avail_out=static_cast<uInt>(target.size());
                result=inflate(&stream,Z_NO_FLUSH);
                const auto n=target.size()-stream.avail_out;
                require(result==Z_OK||result==Z_STREAM_END,"Invalid ZIP deflate payload");
                require(n<=entry.public_entry.bytes-produced,"ZIP expanded size mismatch");
                if(n){checksum=crc32(checksum,target.data(),static_cast<uInt>(n));
                    output.write(target.data(),n);produced+=n;}
                require(result==Z_STREAM_END||n||stream.avail_in<before,
                    "Truncated ZIP deflate payload");
            }
            require(!remaining&&!stream.avail_in,"Trailing ZIP compressed payload");
        }catch(...){inflateEnd(&stream);throw;}
        inflateEnd(&stream);
    }
    require(produced==entry.public_entry.bytes&&checksum==entry.crc,"ZIP size or CRC mismatch");
    output.flush();
}
}
ArchiveExtraction extract_mod_zip(const fs::path &archive,const fs::path &destination,ArchiveBudget budget){
    static_assert(sizeof(void*)==8&&sizeof(std::size_t)==8&&sizeof(std::streamoff)>=8);
    require(budget.maximum_decoder_memory>=512*1024,"ZIP decoder memory budget too small");
    const auto output=fs::absolute(destination).lexically_normal();
    require_plain_directory(output.parent_path());
    require(!fs::exists(output),"Extraction destination already exists");
    const auto file_size=fs::file_size(archive);
    require(file_size>=22&&file_size<=std::uint64_t(std::numeric_limits<std::streamoff>::max()),
            "Invalid ZIP archive size");
    std::ifstream input(archive,std::ios::binary);require(bool(input),"Cannot open ZIP archive");
    const auto tail_start=file_size-std::min<std::uint64_t>(file_size,65557);
    const auto tail=read_at(input,tail_start,static_cast<std::size_t>(file_size-tail_start),file_size);
    std::optional<std::uint64_t> eocd;
    for(std::size_t i=tail.size()-22;;--i){
        if(u32(tail,i)==0x06054b50&&i+22+u16(tail,i+20)==tail.size()){
            eocd=tail_start+i;break;}
        if(i==0)break;
    }
    require(eocd.has_value(),"ZIP end record missing");
    const auto end=read_at(input,*eocd,22,file_size);
    require(u16(end,4)==0&&u16(end,6)==0&&u16(end,8)==u16(end,10),
            "Multi-disk ZIP is unsupported");
    std::uint64_t count=u16(end,10),central_size=u32(end,12),central_offset=u32(end,16);
    std::uint64_t central_limit=*eocd;
    if(count==UINT16_MAX||central_size==UINT32_MAX||central_offset==UINT32_MAX){
        require(*eocd>=20,"ZIP64 locator missing");
        const auto locator=read_at(input,*eocd-20,20,file_size);
        require(u32(locator,0)==0x07064b50&&u32(locator,4)==0&&u32(locator,16)==1,
                "Invalid ZIP64 locator");
        const auto record_at=u64(locator,8);
        const auto record=read_at(input,record_at,56,file_size);
        require(u32(record,0)==0x06064b50&&u64(record,4)>=44&&
                u32(record,16)==0&&u32(record,20)==0&&u64(record,24)==u64(record,32),
                "Invalid ZIP64 end record");
        span_within(record_at,12,*eocd-20);
        span_within(record_at+12,u64(record,4),*eocd-20);
        count=u64(record,32);central_size=u64(record,40);central_offset=u64(record,48);
        central_limit=record_at;
    }
    span_within(central_offset,central_size,central_limit);
    require(count<=budget.maximum_entries&&count<=SIZE_MAX,"ZIP entry count exceeds budget");
    require(central_offset+central_size==central_limit,"Unexpected data after ZIP directory");
    ArchiveExtraction extraction;extraction.peak_decoder_memory=512*1024;
    std::vector<ZipEntry> entries;entries.reserve(static_cast<std::size_t>(count));
    std::map<std::wstring,bool,PathLess> paths;
    std::set<std::uint64_t> local_offsets;
    bool root_marker_seen=false;
    auto cursor=central_offset;
    for(std::uint64_t i=0;i<count;++i){
        const auto head=read_at(input,cursor,46,central_offset+central_size);
        require(u32(head,0)==0x02014b50,"Invalid ZIP directory entry");
        const auto flags=u16(head,8),method=u16(head,10),name_length=u16(head,28),
            extra_length=u16(head,30),comment_length=u16(head,32);
        require(!(flags&0x2041)&&!u16(head,34)&&(method==0||method==8),
            "Encrypted, split or unsupported ZIP entry");
        const auto variable_size=std::uint64_t(name_length)+extra_length+comment_length;
        span_within(cursor,46+variable_size,central_offset+central_size);
        const auto variable=read_at(input,cursor+46,static_cast<std::size_t>(variable_size),file_size);
        auto raw_name=Bytes(variable.begin(),variable.begin()+name_length);
        std::uint64_t expanded=u32(head,24),compressed=u32(head,20),local=u32(head,42);
        if(expanded==UINT32_MAX||compressed==UINT32_MAX||local==UINT32_MAX){
            bool found=false;std::size_t at=name_length;
            while(at<name_length+extra_length){
                require(name_length+extra_length-at>=4,"Truncated ZIP extra field");
                const auto kind=u16(variable,at),length=u16(variable,at+2);at+=4;
                require(length<=name_length+extra_length-at,"Invalid ZIP extra length");
                if(kind==1){found=true;std::size_t field=at;
                    if(expanded==UINT32_MAX){require(field+8<=at+length,"Missing ZIP64 size");expanded=u64(variable,field);field+=8;}
                    if(compressed==UINT32_MAX){require(field+8<=at+length,"Missing ZIP64 compressed size");compressed=u64(variable,field);field+=8;}
                    if(local==UINT32_MAX){require(field+8<=at+length,"Missing ZIP64 offset");local=u64(variable,field);}
                    break;
                }
                at+=length;
            }
            require(found,"Missing ZIP64 entry extra field");
        }
        const auto attrs=u32(head,38);
        const auto unix_kind=u16(head,4)>>8;
        const auto unix_type=unix_kind==3?((attrs>>16)&0170000):0u;
        require(!(attrs&FILE_ATTRIBUTE_REPARSE_POINT)&&
                (unix_type==0||unix_type==0100000||unix_type==0040000),
                "ZIP contains link or special file");
        const auto trailing=!raw_name.empty()&&(raw_name.back()=='/'||raw_name.back()=='\\');
        const auto directory=trailing||(attrs&FILE_ATTRIBUTE_DIRECTORY)!=0||unix_type==0040000;
        // Some shared-folder ZIPs include a literal "/" directory entry. It
        // denotes the extraction root, never an output path.
        const bool root_marker=raw_name.size()==1&&raw_name.front()=='/';
        if(trailing)raw_name.pop_back();
        const auto path=root_marker?fs::path{}:archive_relative_path(decode_name(raw_name,(flags&0x800)!=0));
        if(root_marker){require(!root_marker_seen,"Duplicate ZIP root marker");root_marker_seen=true;}
        else require(paths.emplace(path.native(),directory).second,"Duplicate or case-alias ZIP path");
        require(!directory||(!expanded&&!compressed&&method==0),"ZIP directory has payload");
        require(expanded<=budget.maximum_expanded_bytes-extraction.expanded_bytes,
                "ZIP expanded size exceeds budget");
        extraction.expanded_bytes+=expanded;
        const auto local_head=read_at(input,local,30,central_offset);
        require(u32(local_head,0)==0x04034b50&&u16(local_head,6)==flags&&
                u16(local_head,8)==method&&u16(local_head,26)==name_length,
                "ZIP local header mismatch");
        const auto local_name=read_at(input,local+30,name_length,central_offset);
        require(local_name==Bytes(variable.begin(),variable.begin()+name_length),
                "ZIP local filename mismatch");
        const auto data_offset=local+30+name_length+u16(local_head,28);
        span_within(data_offset,compressed,central_offset);
        require(local_offsets.insert(local).second,"Duplicate ZIP local header");
        if(root_marker){cursor+=46+variable_size;continue;}
        ZipEntry entry{{path,expanded,directory},std::move(raw_name),local,data_offset,compressed,
            u32(head,16),flags,method};
        extraction.entries.push_back(entry.public_entry);entries.push_back(std::move(entry));
        cursor+=46+variable_size;
    }
    require(cursor==central_offset+central_size,"Trailing ZIP directory bytes");
    for(const auto &entry:entries){auto parent=entry.public_entry.relative.parent_path();
        while(!parent.empty()){const auto found=paths.find(parent.native());
            require(found==paths.end()||found->second,"ZIP file/directory collision");
            parent=parent.parent_path();}
    }
    require(fs::create_directory(output),"Cannot create ZIP extraction directory");
    for(const auto &entry:entries){const auto target=output/entry.public_entry.relative;
        fs::create_directories(entry.public_entry.directory?target:target.parent_path());
        require_plain_directory(entry.public_entry.directory?target:target.parent_path());
        if(!entry.public_entry.directory)extract_file(input,file_size,entry,target);
    }
    return extraction;
}
} // namespace civ5::content
