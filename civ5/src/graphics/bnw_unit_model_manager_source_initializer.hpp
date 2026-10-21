#pragma once
#include <array>
#include <cstdint>
#include <map>
namespace civ5::graphics::model_manager_init_detail {
// Source labels are addresses in the pinned x86 file, never callable runtime
// pointers. This private CPU recipe grants no process/cache authority.
struct RegistryFields {
 std::uint32_t source_vtable=0xb71114,constructor_arg=0;
 std::array<std::uint32_t,60> cache_words{};
 std::array<std::uint32_t,7> zero_fields{}; // +48,+54,+70,+74,+16C,+170,+174
 std::uint32_t timer_callback=0,initial_thread=0;
 std::map<std::uint32_t,std::uint32_t> creators;
};
inline RegistryFields constructor_fields(std::uint32_t measured_thread){RegistryFields out;out.initial_thread=measured_thread;return out;}
inline void register_source_creators(RegistryFields& state){
 // 972550 /96D6F0 /9AEB80 /9AF1E0 through96F880, in normal4580F9 order.
 state.creators[0]=0x972470;state.creators[9]=0x96d680;
 state.creators[1]=0x9aeb10;state.creators[8]=0x9af040;
}
struct WrapperFields {
 std::uint32_t source_vtable=0xb70f24,refs=0,name_id=0,registry_identity=0,timestamp_bits=0;
 std::uint8_t marked=0;
 std::uint32_t underlying_resource=0;
};
inline WrapperFields wrapper_fields(std::uint32_t same_registry_identity,std::uint32_t source_flags){
 WrapperFields out;out.registry_identity=same_registry_identity;
 // 96F7D0 applies flags bit8 after972470 creator returns.
 if(source_flags&8)out.marked=1;return out;
}
struct ResourceConstructorFields {
 // The original writes these offsets. Empty-class padding and opaque heap
 // sentinel pointers are deliberately excluded rather than zero-invented.
 std::map<std::uint32_t,std::uint32_t> words;
 std::uint8_t load_byte=0;
};
inline ResourceConstructorFields resource_constructor_fields(std::uint32_t controlled_source_global_flags){
 ResourceConstructorFields out;
 constexpr std::array zero_offsets={4U,8U,12U,16U,20U,24U,32U,36U,40U,44U,52U,56U,60U,68U,72U,76U,84U,88U,92U,100U,104U,108U,120U,128U,132U,136U,144U,148U,152U,164U,172U,176U,180U,184U};
 for(auto offset:zero_offsets)out.words[offset]=0;
 out.words[0]=0xb71548;out.words[28]=0x0fffffff;
 out.words[188]=0x3f800000;out.words[192]=controlled_source_global_flags;
 return out;
}
} // namespace civ5::graphics::model_manager_init_detail
