#include "civ5/graphics/detail/bnw_original_vertex_declaration_gl_schema.hpp"
#include <array>
#include <stdexcept>
namespace civ5::graphics::detail {
BnwOriginalVertexDeclarationGlSchema::Schema
BnwOriginalVertexDeclarationGlSchema::project(
 std::span<const ui::detail::BnwOriginalVertexDeclarationElement> elements){
 if(elements.size()<2||elements.size()>10)
  throw std::runtime_error("Original declaration requires one through nine actual attributes and end");
 const auto&end=elements.back();
 if(end.stream!=255||end.offset||end.type!=17||end.method||end.usage||end.usage_index)
  throw std::runtime_error("Original declaration end8B differs");
 constexpr std::array<std::array<std::uint8_t,2>,14> usages{{
  {0,0},{3,0},{5,0},{5,1},{5,2},{5,3},{10,0},{10,1},{1,0},{1,1},{2,0},{2,1},{6,0},{7,0}}};
 Schema out{{},0};out.attributes.reserve(elements.size()-1);
 for(std::size_t n=0;n+1<elements.size();++n){
  const auto&e=elements[n];
  if(e.stream!=0||e.method!=0)throw std::runtime_error("Original declaration stream/method unsupported");
  bool known_usage=false;
  for(const auto&u:usages)if(u[0]==e.usage&&u[1]==e.usage_index)known_usage=true;
  if(!known_usage)throw std::runtime_error("Original declaration actual usage/index unsupported");
  Attribute a{0,0,GL_FALSE,e.offset,e.type,e.usage,e.usage_index,0};
  switch(e.type){
   case 0:case 1:case 2:case 3:
    a.components=e.type+1;a.scalar_type=GL_FLOAT;a.storage_bytes=4*a.components;break;
   case 15:a.components=2;a.scalar_type=GL_HALF_FLOAT;a.storage_bytes=4;break;
   case 16:a.components=4;a.scalar_type=GL_HALF_FLOAT;a.storage_bytes=8;break;
   case 5:a.components=4;a.scalar_type=GL_UNSIGNED_BYTE;a.storage_bytes=4;break;
   case 8:a.components=4;a.scalar_type=GL_UNSIGNED_BYTE;a.normalized=GL_TRUE;a.storage_bytes=4;break;
   default:throw std::runtime_error("Original declaration actual type unsupported");
  }
  for(const auto&old:out.attributes){
   if(old.usage==a.usage&&old.usage_index==a.usage_index)
    throw std::runtime_error("Original declaration duplicate semantic");
   if(a.relative_offset<old.relative_offset+old.storage_bytes&&old.relative_offset<a.relative_offset+a.storage_bytes)
    throw std::runtime_error("Original declaration actual byte ranges overlap");
  }
  out.stride+=a.storage_bytes;out.attributes.push_back(a);
 }
 for(const auto&a:out.attributes)if(a.relative_offset+a.storage_bytes>out.stride)
  throw std::runtime_error("Original declaration extent exceeds actual summed stride");
 return out;
}
}
