#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
namespace civ5::graphics::model_prerequisite_detail {
// Private controlled CPU recipes; no native namespace/allocator authority.
struct CallbackLabels {std::uint32_t log=0x96cbe0,log_user=0,reader=0x96d250,allocate=0x96cd40,release=0x96cf00;bool initialized=true;};
inline CallbackLabels callback_labels(){return {};}
struct AllocationRoute {bool civ_aligned=false;std::uint32_t bytes=0,alignment=0;};
inline AllocationRoute allocation_route(std::uint32_t alignment,std::uint32_t bytes){return {alignment!=0,bytes,alignment};}
inline std::optional<unsigned> log_level(std::uint32_t type,std::uint32_t code){if(type!=3)return 2;if(code==45)return 1;return std::nullopt;}
inline bool gsd_match(const std::string& name){
 for(unsigned char c:name)if(c==0||c>127)throw std::invalid_argument("Private GSD source recipe ASCII scope");
 if(name.size()<4)return false;auto suffix=name.substr(name.size()-4);
 for(auto& c:suffix)if(c>='a'&&c<='z')c=char(c-'a'+'A');return suffix==".GSD";
}
struct NamespaceFixture {bool enabled=true;std::vector<std::vector<std::uint32_t>> buckets;};
inline std::vector<std::string> enumerate_source_names(const std::optional<NamespaceFixture>& current,const std::vector<NamespaceFixture>& stack,const std::map<std::uint32_t,std::string>& first_global_names){
 std::vector<std::string> out;if(!current||!current->enabled)return out;
 const auto append=[&](const NamespaceFixture& ns){if(!ns.enabled)return;for(const auto& bucket:ns.buckets)for(auto key:bucket){auto hit=first_global_names.find(key);if(hit!=first_global_names.end()&&gsd_match(hit->second))out.push_back(hit->second);}};
 for(auto i=stack.rbegin();i!=stack.rend();++i)append(*i);append(*current);return out;
}
struct TypeStringFixture {std::uint32_t modeled_crc=0;std::string bytes;};
struct DescriptorFixture {bool file_loaded=true,descriptor_available=true;std::vector<TypeStringFixture> types;};
struct DescriptorLifetimeFields {std::map<std::uint32_t,std::string> owned_names;std::uint32_t comparison_label=0;unsigned files_freed=0,files_without_descriptor=0;};
inline DescriptorLifetimeFields register_descriptor_names(const std::vector<DescriptorFixture>& files,std::uint32_t controlled_previous_comparison){
 DescriptorLifetimeFields out;out.comparison_label=controlled_previous_comparison;
 for(const auto& file:files){if(!file.file_loaded)continue;if(!file.descriptor_available){++out.files_without_descriptor;continue;}
  for(const auto& type:file.types)out.owned_names.try_emplace(type.modeled_crc,type.bytes);
  out.comparison_label=0x96c970;++out.files_freed;
 }return out;
}
} // namespace civ5::graphics::model_prerequisite_detail
