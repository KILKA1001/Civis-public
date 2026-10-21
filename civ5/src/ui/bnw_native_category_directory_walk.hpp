#pragma once
#include "bnw_native_loose_walk.hpp"
#include "bnw_native_category_pattern_match.hpp"

// Private traversal data only: no Package, mount, stat, record or completion
// issuer. CategorySource supplies the authentic retained request/provider rows
// and validates its actual namespace/lease channel before/after each recursion.
namespace civ5::ui::native_category_directory_detail {
inline std::string normalized_query_ascii(std::string_view request) {
    native_category_patterns_detail::ascii_input(request);
    native_loose_detail::need(!request.empty(),"Native category Directory request is empty");
    std::string query(request);
    std::replace(query.begin(),query.end(),'/','\\');
    if(query.back()!='\\')query+='\\'; //4DE480(0,1): preserve repeated separators
    return query;
}
struct DirectoryWalk : native_loose_detail::LooseWalk {
    // Native query retains incoming relative/absolute form, independently of
    // the confined absolute filesystem path used for actual OS enumeration.
    std::string original_request;
    std::vector<std::string> provider_rows;
    bool operator==(const DirectoryWalk&)const=default;
};
template<class Enumeration,class Current>
std::vector<native_loose_detail::LooseEntry> capture_entries(
    const std::string& io_root,const std::string& original_request,
    std::span<const std::string> provider_rows,Enumeration& enumeration,Current&& current) {
    const auto root_query=normalized_query_ascii(original_request);
    std::vector<native_loose_detail::LooseEntry> entries;
    const auto visit=[&](auto&& self,const std::string& relative)->void {
        current();
        const auto source_query=relative.empty()?root_query:root_query+relative+'\\';
        //83A827/831 before FindA. Match the SAME whole native query, never
        // io_root, a basename, or a post-capture reconstructed path prefix.
        if(native_category_patterns_detail::directory_excluded_ascii(source_query,provider_rows)) {
            current();return;
        }
        const auto search=io_root+'\\'+relative+(relative.empty()?"":"\\")+"*.*";
        {
            auto find=enumeration.open(search);
            native_loose_detail::need(find.valid(),"Native category directory enumeration failed");
            do {
                const auto& data=find.data;
                const std::string name=data.cFileName;
                const auto attributes=data.dwFileAttributes;
                if(name=="." || name==".." || (attributes&6)!=0)continue;
                native_loose_detail::need(std::all_of(name.begin(),name.end(),[](unsigned char c){return c<128;}),
                    "Native category directory non-ASCII record requires CRT source owner");
                native_loose_detail::need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),
                    "Native category directory reparse source requires target owner");
                const auto path=relative.empty()?name:relative+'\\'+name;
                const auto bytes=(std::uint64_t(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
                const auto written=(std::uint64_t(data.ftLastWriteTime.dwHighDateTime)<<32)|data.ftLastWriteTime.dwLowDateTime;
                entries.push_back({std::filesystem::path(native_loose_detail::acp_wide(path)),name,
                    attributes,bytes,written,path});
                if(attributes&FILE_ATTRIBUTE_DIRECTORY)self(self,path);
            }while(find.next());
            native_loose_detail::need(find.error()==ERROR_NO_MORE_FILES,
                "Native category directory traversal incomplete");
        } //Close this actual cursor before the final currentness callback.
        current();
    };
    visit(visit,{});return entries;
}
template<class Current>
DirectoryWalk capture(const std::filesystem::path& io_root,const std::string& original_request,
    std::span<const std::string> provider_rows,Current&& current) {
    current();
    DirectoryWalk out;
    out.root=io_root;out.base_filters=false;out.exists=std::filesystem::exists(io_root);
    out.code_page=GetACP();out.original_request=original_request;
    out.provider_rows.assign(provider_rows.begin(),provider_rows.end());
    (void)normalized_query_ascii(original_request);
    if(out.exists) {
        native_loose_detail::need(std::filesystem::is_directory(io_root),
            "Native category directory source is not a directory");
        native_loose_detail::NativeEnumeration enumeration;
        out.entries=capture_entries(native_loose_detail::acp_path(io_root),original_request,
            provider_rows,enumeration,current);
    }
    native_loose_detail::need(GetACP()==out.code_page,"Native category directory CP_ACP changed");
    current();return out;
}
template<class Current>
bool matches(const DirectoryWalk& expected,std::span<const std::string> actual_provider_rows,
    Current&& current) {
    native_loose_detail::need(GetACP()==expected.code_page,"Native category directory CP_ACP changed");
    native_loose_detail::need(std::equal(expected.provider_rows.begin(),expected.provider_rows.end(),
        actual_provider_rows.begin(),actual_provider_rows.end()),"Native category directory provider rows changed");
    return capture(expected.root,expected.original_request,actual_provider_rows,current)==expected;
}
}
