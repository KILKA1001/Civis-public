#pragma once
#include "civ5/graphics/gl_bnw_font_shader.hpp"
#include <filesystem>
#include <cstdint>
namespace civ5::validation {
struct OriginalFontShaderPrograms {
 std::vector<std::uint32_t>vs,ps;
 std::string exe_sha256,vs_sha256,ps_sha256;
};
// Reads original executable bytes once, pins the whole file and exact embedded
// programs, and never instruments/recompiles/modifies original shader tokens.
OriginalFontShaderPrograms read_original_font_shader_programs(const std::filesystem::path&);
void validate_original_font_shader_programs(const OriginalFontShaderPrograms&);
struct GeneratedFontShaderPlanes {
 graphics::GlBnwSystemFontAtlasFrame pair;
 assets::Bytes green,shadow;
 std::string green_sha256,shadow_sha256;
};
enum class FontShaderOracleStorage {rgba32f,rgba8};
struct FontShaderOracleImage {
 FontShaderOracleStorage storage=FontShaderOracleStorage::rgba8;
 int width=0,height=0;
 // Canonical top-down RGBA; native rowPitch/BGRA conversion is recorded.
 std::vector<float>rgba32f;
 assets::Bytes rgba8;
 std::uint32_t native_format=0;
 std::int32_t native_row_pitch=0;
 bool original_shader_fixture_executed=false;
 bool direct_original_vs_outputs_observed=false;
 bool native_label_initialized=false,native_game_capture=false;
 std::uint32_t actual_vertex_stride=36;
 bool actual_color_ubyte4n=false;
};
struct FontShaderOracleDeviceReceipt {
 std::string adapter_description;
 std::uint32_t vendor=0,device=0,vs_version=0,ps_version=0;
 bool hal=false,l8_point_wrap=false,rgba8_blend8=false,float_rt=false;
 bool float_post_shader_blend=false;
};
class Dx9FontShaderOracle : public std::enable_shared_from_this<Dx9FontShaderOracle> {
 public:
 static std::shared_ptr<Dx9FontShaderOracle>create(
  graphics::GlContextNativeIdentity,int width,int height,
  const OriginalFontShaderPrograms&,const GeneratedFontShaderPlanes&,
  std::function<void()>validate_same_source);
 ~Dx9FontShaderOracle();
 FontShaderOracleImage draw(const graphics::BnwFontShaderBatch&,
  FontShaderOracleStorage,bool source_blend8);
 FontShaderOracleImage draw_packed(const graphics::BnwFontShaderPackedBatch&,
  FontShaderOracleStorage,bool source_blend8);
 const FontShaderOracleDeviceReceipt&receipt()const noexcept;
 private:
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit Dx9FontShaderOracle(std::unique_ptr<Impl>);
};
}
