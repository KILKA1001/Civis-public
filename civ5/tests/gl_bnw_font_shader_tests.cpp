#include "civ5/graphics/gl_bnw_font_shader.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <algorithm>
#include <cmath>
#include <thread>
#include "civ5/ui/bnw_text_locale_adapter.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
using namespace civ5;
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool blocked=false;try{f();}catch(const std::exception&){blocked=true;}check(blocked,"Native font atlas unexpectedly accepted missing/stale producer");}
int main(int argc,char**argv){
 check(argc==4,"Actual Gandhi/rules/shared assets required");const std::filesystem::path root(argv[3]);auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);if(!selected.preview){
  std::cerr<<"Selected BNW bootstrap failed";
  if(selected.preview.blocker)std::cerr<<" stage="<<static_cast<int>(selected.preview.blocker->stage)<<" reason="<<selected.preview.blocker->reason;
  else std::cerr<<" without host or blocker";
  std::cerr<<std::endl;return 1;
 }auto&live=*selected.preview.host;
 auto context=std::make_shared<graphics::GlContext>(800,600,false);const auto window_handle=reinterpret_cast<HWND>(context->native_window_handle());std::size_t queries=0,trigger=0;
 std::uint64_t client_revision=4;auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window_handle),"gandhi-text-policy",[&]{++queries;if(trigger&&queries>=trigger){++client_revision;trigger=0;}return client_revision;});
 auto loose=std::make_shared<assets::LooseResourceIndex>(root);loose->add_file("Assets/UI/Styles.xml");for(const auto font:{"TwCenMT16","TwCenMT18","TwCenMT20"})loose->add_file(std::string("Assets/UI/Fonts/Tw Cent MT/")+font+".ggxml");for(const auto prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU","Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/ru_RU","Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/ru_RU"})loose->scan({root/prefix,{},true});assets::ResourceMountTable resources(assets::resource_lookup(loose),assets::resource_lookup_all(loose),assets::resource_keys(loose));const assets::AssetEnvironment environment(root);std::uint64_t mount=1;for(const auto key:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){auto opened=environment.open_fpk_archive(key);auto*archive=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);check(archive,"Shared source FPK missing");resources.mount({mount++,0,0,assets::resource_lookup(*archive),true,true,{},{}});}
 auto shell=ui::BnwCivilopediaScrollBarOwner::create(root,resources,*client.current(),runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256);auto tree=ui::BnwCivilopediaRightTreeOwner::create(root,resources,shell->render_snapshot());const auto metrics=tree->snapshot().label_metrics;check(metrics&&metrics->snapshot().staged_metrics.size()==19,"Selected metric producer missing");const auto*a=tree->control(*client.current(),0,"ArticleID");const auto index=static_cast<std::size_t>(a-tree->snapshot().controls.data());
 const auto sources=ui::inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Actual locale source unavailable");ui::CivilopediaLayoutOwnerKey identity;identity.frame_number=9;identity.owner_token="actual-gandhi-retained-locale";identity.player_id=live.active_player_id();identity.game_turn=live.session().state().setup.game_turn;identity.payload_sha256=live.identity().payload_sha256;identity.rules_sha256=live.identity().rules_sha256;identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;

 auto locale=ui::SelectedCivilopediaLocale::open(root,resources,sources,identity,"ru_RU");auto text_owner=ui::CivisLocaleTextOwner::open(locale,identity,"ru_RU");auto policy=ui::BnwClientTextPolicyOwner::create(live.database(),client,ui::bnw_text_policy_selected_locale(locale,text_owner,identity),ui::BnwClientTextPolicy::selected_language,metrics);auto cpu=ui::BnwSystemFontCpuOwner::create(reinterpret_cast<std::uintptr_t>(window_handle),policy,resources,policy->declared_metric(index));auto cache=ui::BnwSystemFontCacheOwner::create(cpu);auto atlas=graphics::GlBnwSystemFontAtlas::create(context,cache);

 check(runtime::identify_file(root/"CivilizationV.exe").sha256==graphics::bnw_font_shader_original_exe_sha256,"Original FontShader binary source pin differs");
 // Existing source-bound factory above is fixture setup only; it does not
 // create a production Label/default-font/packed-color capability.
 check(cache->source_cpu_owner().get()==cpu.get()&&atlas->source_cache_owner().get()==cache.get(),"No-current source identities differ");
 const auto native_identity=context->native_identity();check(native_identity.owner_thread&&native_identity.window==reinterpret_cast<std::uintptr_t>(window_handle)&&native_identity.context==reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())&&native_identity.dc==reinterpret_cast<std::uintptr_t>(wglGetCurrentDC()),"Wrong retained native identity");
 const auto pending=cache->measure('W');check(!atlas->has_uploaded_quote_no_current(pending),"Pending raster incorrectly accepted as uploaded pair");
 const auto q=atlas->upload('W',pending);const auto pair=atlas->sources_no_current().pair;
 check(atlas->has_uploaded_quote_no_current(q)&&!atlas->has_uploaded_quote_no_current(pending),"No-current accepted-upload proof wrong");
 auto mutated=q;mutated.slot.u1+=.01f;check(!atlas->has_uploaded_quote_no_current(mutated),"No-current upload proof ignored changed quote");
 const auto green=atlas->read_uploaded(q,false),shadow=atlas->read_uploaded(q,true);
 const auto found=std::find_if(green.begin(),green.end(),[](auto x){return x>0&&x<255;});check(found!=green.end(),"Meaningful nonbinary real brightness pixel unavailable");
 const auto offset=static_cast<std::size_t>(found-green.begin());const auto&font=cpu->current();
 const float u=(static_cast<float>(font.cell_width*(q.slot_index%font.columns)+offset%q.slot.advance)+.5f)/font.atlas_width;
 const float v=(static_cast<float>(font.cell_height*(q.slot_index/font.columns)+offset/q.slot.advance)+.5f)/font.atlas_height;
 graphics::GlRenderApi r(*context);const auto&gl=context->api();GLuint target_texture=0,target_fbo=0;
 gl.CreateTextures(GL_TEXTURE_2D,1,&target_texture);gl.TextureStorage2D(target_texture,1,GL_RGBA8,800,600);
 r.CreateFramebuffers(1,&target_fbo);r.NamedFramebufferTexture(target_fbo,GL_COLOR_ATTACHMENT0,target_texture,0);const GLenum draw_buffer=GL_COLOR_ATTACHMENT0;r.NamedFramebufferDrawBuffers(target_fbo,1,&draw_buffer);r.BindFramebuffer(GL_DRAW_FRAMEBUFFER,target_fbo);
 auto shader=graphics::GlBnwFontShader::create(context,atlas);bool current_batch=true,current_target=true;
 graphics::BnwFontShaderTarget target;target.framebuffer=target_fbo;target.color_texture=target_texture;target.width=800;target.height=600;target.retained_client=*client.current();target.current=[&]{return current_target;};
 graphics::BnwFontShaderBatch batch;batch.retained_pair=pair;batch.current=[&]{return current_batch;};batch.caller_declared_diagnostic_colors=true;
 batch.controls.source_matrix_rows={{{2.f/800,0,0,0},{0,-2.f/600,0,0},{0,0,-.1f,0},{-1,1,0,1}}};
 batch.controls.depth_pair={.3f,.7f};batch.controls.parent_alpha=.5f;batch.controls.clip_xmin_xmax_ymin_ymax={0,800,0,600};
 graphics::BnwFontShaderQuad quad;quad.uploaded_slot=q;const std::array<float,4>color{.7f,.3f,.2f,.6f};
 quad.vertices={{{4,4,u,v,color},{24,4,u,v,color},{24,24,u,v,color},{4,24,u,v,color}}};batch.layers={{0,{quad}}};
 const std::array<float,4>background{.1f,.2f,.3f,.2f};
 const auto clear=[&]{r.ClearNamedFramebufferfv(target_fbo,GL_COLOR,0,background.data());};
 const auto pixel=[&](int x,int y){std::array<std::uint8_t,4>out{};gl.GetTextureSubImage(target_texture,0,x,graphics::bnw_font_target_row_origin(target.pixel_centers)==graphics::BnwFontTargetRowOrigin::logical_top_at_gl_row0?y:599-y,0,1,1,1,GL_RGBA,GL_UNSIGNED_BYTE,4,out.data());context->check("Read actual diagnostic FontShader pixel");return out;};
 const auto expected=[&](unsigned slot,bool allow,unsigned layers){
  const double coverage=1-double(slot==2?shadow[offset]:green[offset])/255;const double alpha=coverage*color[3]*batch.controls.parent_alpha;
  std::array<double,4>out;for(unsigned k=0;k<4;++k)out[k]=std::round(background[k]*255)/255;
  for(unsigned n=0;n<layers;++n){for(unsigned k=0;k<3;++k)out[k]=std::round((out[k]*(1-alpha)+(allow?color[k]:coverage)*alpha)*255)/255;out[3]=std::round(std::min(1.0,out[3]+alpha)*255)/255;}return out;};
 const auto matches=[&](auto actual,auto wanted){for(unsigned k=0;k<4;++k)check(std::abs(actual[k]/255.0-wanted[k])<=3./255,"Brightness/color/separate-alpha control mismatch");};
 r.Disable(GL_BLEND);r.Enable(GL_DEPTH_TEST);r.Enable(GL_CULL_FACE);r.Enable(GL_FRAMEBUFFER_SRGB);r.ViewportIndexedf(0,3,7,711,509);
 for(const auto center:{graphics::BnwFontPixelCenterControl::unshifted_gl,graphics::BnwFontPixelCenterControl::d3d9_integer_center_diagnostic}){
  target.pixel_centers=center;
  for(const auto slot:{0u,1u,2u})for(bool allow:{false,true}){
   batch.layers={{slot,{quad}}};batch.controls.allow_color=allow;clear();const auto result=shader->draw(batch,target);
   check(result.source_shader_available&&!result.native_label_initialized&&!result.native_pixels_compared&&result.diagnostic_colors&&result.unused_stage0_differs_from_native&&result.actual_target_row_origin==graphics::bnw_font_target_row_origin(center)&&result.draw_calls==1&&result.quads==1,"Diagnostic draw fabricated native Label/pixel readiness");check(result.actual_context.window==native_identity.window&&result.accepted_pair.green_texture==pair.green_texture&&result.actual_target_color_texture==target_texture&&result.actual_target_framebuffer==target_fbo&&result.actual_target_width==800&&result.actual_target_height==600,"Diagnostic actual-target/source receipt mismatch");matches(pixel(10,10),expected(slot,allow,1));
   check(!r.IsEnabled(GL_BLEND)&&r.IsEnabled(GL_DEPTH_TEST)&&r.IsEnabled(GL_CULL_FACE)&&r.IsEnabled(GL_FRAMEBUFFER_SRGB),"FontShader failed caller state restoration");GLfloat viewport[4]{};r.GetFloati_v(GL_VIEWPORT,0,viewport);check(viewport[0]==3&&viewport[1]==7&&viewport[2]==711&&viewport[3]==509,"FontShader changed caller viewport");
  }
 }
 target.pixel_centers=graphics::BnwFontPixelCenterControl::d3d9_integer_center_diagnostic;batch.controls.allow_color=true;batch.layers={{1,{quad}},{0,{quad}}};clear();shader->draw(batch,target);matches(pixel(10,10),expected(0,true,2));
 batch.layers={{0,{quad}}};batch.controls.clip_xmin_xmax_ymin_ymax={10,10,10,10};clear();shader->draw(batch,target);matches(pixel(10,10),expected(0,true,1));matches(pixel(9,10),expected(0,true,0));matches(pixel(11,10),expected(0,true,0));
 batch.controls.clip_xmin_xmax_ymin_ymax={0,800,0,600};clear();current_batch=false;rejects([&]{shader->draw(batch,target);});current_batch=true;matches(pixel(10,10),expected(0,true,0));
 const auto saved_target=target;target.color_texture=pair.green_texture;rejects([&]{shader->draw(batch,target);});target=saved_target;
 auto bad=batch;bad.layers={{0,{quad}},{1,{quad}}};rejects([&]{shader->draw(bad,target);});bad=batch;bad.controls.depth_pair={0,0};rejects([&]{shader->draw(bad,target);});bad=batch;bad.retained_pair.revision++;rejects([&]{shader->draw(bad,target);});
 // Request arguments are copied before any callback. A vector reallocation
 // does not change which already accepted diagnostic quad is submitted.
 batch.current=[&]{batch.layers.clear();batch.layers.shrink_to_fit();return true;};const auto pinned=batch;clear();shader->draw(batch,target);matches(pixel(10,10),expected(0,true,1));batch=pinned;batch.current=[&]{return current_batch;};
 std::size_t callback_count=0;batch.current=[&]{return ++callback_count<2;};clear();rejects([&]{shader->draw(batch,target);});check(callback_count==2,"Post-effect source refusal not exercised");matches(pixel(10,10),expected(0,true,1));batch.current=[&]{return current_batch;};
 // A caught nested draw must quarantine the outer operation and owner.
 bool nested_refused=false;batch.current=[&]{try{shader->draw(batch,target);}catch(...){nested_refused=true;}return true;};clear();rejects([&]{shader->draw(batch,target);});check(nested_refused,"Same-owner recursive draw not attempted/refused");matches(pixel(10,10),expected(0,true,0));
 batch.current=[&]{return current_batch;};rejects([&]{shader->draw(batch,target);});shader=graphics::GlBnwFontShader::create(context,atlas);
 // Last external shared_ptr may disappear inside a current callback. Method
 // keeps itself alive through source checks, GPU effect and final return.
 std::weak_ptr<graphics::GlBnwFontShader>weak_shader=shader;bool self_live_in_callback=false;
 batch.current=[&]{shader.reset();self_live_in_callback=!weak_shader.expired();return true;};clear();const auto lifetime_result=shader->draw(batch,target);check(self_live_in_callback&&weak_shader.expired()&&lifetime_result.source_shader_available,"Draw callback destroyed borrowed Impl or falsely retained consumer forever");matches(pixel(10,10),expected(0,true,1));batch.current=[&]{return current_batch;};shader=graphics::GlBnwFontShader::create(context,atlas);
 // Source identity getters retain exact source lifetime, not copied state.
 auto retained=atlas->source_cache_owner();auto retained_cpu=retained->source_cpu_owner();check(retained.get()==cache.get()&&retained_cpu.get()==cpu.get(),"Source lifetime identity changed");
 bool thread_rejected=false;std::thread foreign([&]{try{atlas->sources_no_current();}catch(...){thread_rejected=true;}check(!context->native_identity().owner_thread,"Foreign-thread native identity mislabeled");});foreign.join();check(thread_rejected,"Mutable no-current pair snapshot allowed foreign thread");
 auto other=std::make_shared<graphics::GlContext>(32,32,false);const auto other_identity=other->native_identity();const auto original_queries=queries;
 rejects([&]{shader->draw(batch,target);});check(wglGetCurrentContext()==reinterpret_cast<HGLRC>(other_identity.context)&&wglGetCurrentDC()==reinterpret_cast<HDC>(other_identity.dc)&&queries==original_queries,"Foreign-context refusal switched WGL or called source callbacks");
 rejects([&]{graphics::GlBnwFontShader::create(other,atlas);});check(wglGetCurrentContext()==reinterpret_cast<HGLRC>(other_identity.context),"Wrong source context creation switched WGL");
 context->check("Explicit fixture return to original context");r.BindFramebuffer(GL_DRAW_FRAMEBUFFER,target_fbo);
 check(!atlas->current().native_label_initialized&&!atlas->current().shader_draw_available&&!cpu->current().gpu_pair_ready,"Diagnostic consumer changed original source readiness flags");
 shader.reset();r.BindFramebuffer(GL_DRAW_FRAMEBUFFER,0);r.DeleteFramebuffers(1,&target_fbo);gl.DeleteTextures(1,&target_texture);context->check("Destroy only diagnostic target/program");
 std::cout<<"PASS diagnostic SAME real source font pair consumer: brightness/color/slot binding/Blend8 alpha/clip controls, current source/target refusal, pinned arguments and no WGL switch; original shader oracle/native Label/pixels remain pending\n";
}
