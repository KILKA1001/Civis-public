#include "gl_bnw_warrior_retained.hpp"
#include "bnw_warrior_material_sources.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;using namespace civ5::graphics;
static void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
static std::string read(const std::filesystem::path&p){std::ifstream f(p);check(bool(f),"Actual installed XML missing");return {std::istreambuf_iterator<char>(f),{}};}
static std::string row(const std::string& s,const std::string& type){auto at=s.find("<Type>"+type+"</Type>");check(at!=std::string::npos,"Actual installed color row missing");return s.substr(at,s.find("</Row>",at)-at);}
static std::string field(const std::string& s,const std::string& tag){const auto start=s.find("<"+tag+">");check(start!=std::string::npos,"Actual installed field missing");const auto at=start+tag.size()+2;return s.substr(at,s.find("</"+tag+">",at)-at);}
static std::size_t differs(const assets::Bytes&a,const assets::Bytes&b){check(a.size()==b.size(),"Target extent changed");std::size_t n=0;for(std::size_t i=0;i<a.size();++i)n+=a[i]!=b[i];return n;}
int main(int argc,char**argv){try{
 check(argc==6||(argc==7&&std::string_view(argv[6])=="--native-format4"),"Usage: retained_material ASSET_ROOT SDK_CACHE MATERIAL_JSON SAMPLER_JSON OUTPUT_DIR [--native-format4]");
 const bool format4=argc==7;
 const std::filesystem::path assets_root=argv[1],cache=argv[2],output=argv[5];std::filesystem::create_directories(output);
 check(std::filesystem::equivalent(assets_root,"C:/CrazyProject/civ5-assets"),"Canonical AssetRoot required");
 // Controlled authoritative fixture roster/rules. Its RGBA/Type data comes
 // from actual installed XML. This is not an imported save-roster receipt.
 content::RulesDatabase rules;rules.apply_sql("CREATE TABLE PlayerColors(ID INTEGER,Type TEXT,SecondaryColor TEXT);CREATE TABLE Colors(Type TEXT,Red REAL,Green REAL,Blue REAL,Alpha REAL)");
 const auto players=read(assets_root/"Assets/Gameplay/XML/Interface/CIV5PlayerColors.xml"),colors=read(assets_root/"Assets/Gameplay/XML/Interface/CIV5Colors.xml");
 const std::array<std::string,2> names{"PLAYERCOLOR_INDIA","PLAYERCOLOR_CHINA"};
 for(unsigned i=0;i<2;++i){const auto secondary=field(row(players,names[i]),"SecondaryColor");const auto cr=row(colors,secondary);
  rules.apply_statements(std::vector<content::DatabaseStatement>{{"INSERT INTO PlayerColors VALUES(?,?,?)",{std::int64_t(41+i),names[i],secondary}},{"INSERT INTO Colors VALUES(?,?,?,?,?)",{secondary,double(std::stof(field(cr,"Red"))),double(std::stof(field(cr,"Green"))),double(std::stof(field(cr,"Blue"))),double(std::stof(field(cr,"Alpha")))}}});}
 runtime::PregameRoster roster;roster.slots[0].player_color=41;roster.slots[0].player_color_key=names[0];
 constexpr std::uint64_t epoch=73;constexpr int unit=16385;
 auto context=std::make_shared<GlContext>(512,512,false);GlRenderApi gl(*context);
 auto color=GlTexture::render_target(*context,512,512,1,RenderTextureFormat::rgba8),depth=GlTexture::render_target(*context,512,512,1,RenderTextureFormat::depth24_stencil8);
 GLuint fbo=0;gl.CreateFramebuffers(1,&fbo);gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color.id(),0);gl.NamedFramebufferTexture(fbo,GL_DEPTH_STENCIL_ATTACHMENT,depth.id(),0);const GLenum target=GL_COLOR_ATTACHMENT0;gl.NamedFramebufferDrawBuffers(fbo,1,&target);check(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Target incomplete");gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.Viewport(0,0,512,512);
 const assets::AssetEnvironment env(assets_root);
 const auto archive=[&](const char* path){const auto r=env.open_fpk_archive(path);const auto p=std::get_if<std::shared_ptr<assets::FpkSource>>(&r);check(p!=nullptr,"Actual fog archive unavailable");return *p;};
 const auto data=archive("Resource/Common/MiscData.fpk"),textures=archive("Resource/DX9/MiscTextures.fpk");
 const auto fog_assets=materials::DistanceFogAssets::load([&](const char* name){const auto source=std::filesystem::path(name).extension()==".csv"?data:textures;auto r=source->find(assets::AssetKey(std::filesystem::path(name).filename().string()));check(std::holds_alternative<assets::AssetHandle>(r),"Actual authored fog resource missing");return std::get<assets::AssetHandle>(r);});
 GlDistanceFog fog(*context,fog_assets);
 GlBnwWarriorRetainedBackend backend(context,assets_root,cache);backend.select_sdk_sample(0);
 check(!backend.code_material_state().retained,"CODE owner exists before retention");
 backend.retain_code_material(argv[3],argv[4],roster,rules,0,unit,epoch);check(backend.buffer_uploads()==12,"CODE retained raw buffers unexpected");
 if(format4){backend.retain_native_format4_diagnostic();check(backend.native_format4_diagnostic_retained()&&backend.buffer_uploads()==15,"Wholepacked format4 resources were not retained");check(!backend.code_material_state().source_ps_sdk_four_diagnostic&&!backend.code_material_state().native_optimized_vertex_buffer,"Format4 SDK diagnostic misreported native production state");bool repeat=false;try{backend.retain_native_format4_diagnostic();}catch(const std::exception&){repeat=true;}check(repeat,"Repeated format4 publication accepted");}
 bool rejected=false;try{backend.retain_code_material(argv[3],argv[4],roster,rules,0,unit,epoch);}catch(const std::exception&){rejected=true;}check(rejected,"Repeated CODE owner accepted");
 std::array<terrain::Matrix4,12> world{};for(unsigned i=0;i<12;++i){auto&m=world[i];m[0]=m[5]=m[10]=.14f;m[15]=1;m[12]=(float(i%4)-1.5f)*13;m[13]=(float(i/4)-1)*10;m[14]=12;}
 GlBnwWarriorFrame frame;frame.width=frame.height=512;frame.view={1,0,0,0,.45f,0,1,0,0,1,0,0,0,-8,0,1};frame.projection={1.f/40,0,0,0,0,1.f/25,0,0,0,0,1.f/100,0,0,0,.5f,1};
 GlBnwWarriorMaterialView view{unit,epoch,0,{0,0,8},&fog,.5f,terrain::X87Precision::double_precision,true};
 const auto clear_depth=reinterpret_cast<PFNGLCLEARDEPTHPROC>(context->proc_address("glClearDepth"));const auto active=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
 const auto clear=[&]{gl.DepthMask(GL_TRUE);clear_depth(1);gl.ClearColor(.03f,.04f,.06f,1);gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);};
 const auto draw=[&]{clear();auto r=backend.draw_code_diagnostic(world,frame,view);check(!r.model_drawable&&!r.full_graphics_parity,"CODE diagnostic opened native readiness");return r;};
 auto r=draw();check(r.drawn&&r.members==12&&r.triangles==7920,"Source material baseline absent");const auto india=color.read_level_raw(0,0);
 // Borrowed owner is queried, not cloned/cold-restored. Change the SAME roster.
 roster.slots[0].player_color=42;roster.slots[0].player_color_key=names[1];r=draw();check(r.members==12,"Borrowed color draw absent");const auto china=color.read_level_raw(0,0);const auto color_delta=differs(india,china);check(color_delta>100,"Live borrowed secondary color did not change draw");roster.slots[0].player_color=41;roster.slots[0].player_color_key=names[0];
 std::array<std::uint8_t,12> states;states.fill(1);for(unsigned i=0;i<6;++i)states[i]=0;
 backend.update_code_visibility(states,2,unit,epoch);view.renderer_clock=2.25f;r=draw();check(r.members==12,"Fade packets removed prematurely");auto mid=backend.code_material_state();for(unsigned i=0;i<6;++i)check(mid.packet_family[i]==2,"CODE packet family transition unapplied");const auto fade=color.read_level_raw(0,0);check(differs(india,fade)>100,"Actual CODE alpha output unchanged");
 view.renderer_clock=2.5f;r=draw();check(r.members==6&&backend.code_material_state().visible_members==6,"Visibility flush did not alter actual draw list");
 states.fill(1);backend.update_code_visibility(states,3,unit,epoch);view.renderer_clock=3.25f;r=draw();check(r.members==12,"Showing packets not restored");view.renderer_clock=3.5f;r=draw();check(r.members==12&&backend.code_material_state().visibility_flushes>=4,"Visibility flush/cache effects missing");
 const auto stable=color.read_level_raw(0,0);const auto uploads=backend.buffer_uploads();for(unsigned i=0;i<20;++i)draw();check(backend.buffer_uploads()==uploads&&differs(stable,color.read_level_raw(0,0))==0,"Material retained frames drift/reupload");
 // Every touched texture target/sampler and render state must be restored.
 const auto source_material=load_bnw_warrior_material_sources(assets_root,cache,argv[3]);GlTexture cube(*context,source_material.resources.at(6).data,DdsSamplingColor::linear);
 GLuint prior_sampler=0;gl.CreateSamplers(1,&prior_sampler);for(unsigned i=0;i<6;++i){fog.ramp().bind(i);cube.bind(i);gl.BindSampler(i,prior_sampler);}active(GL_TEXTURE7);gl.Enable(GL_BLEND);gl.Enable(GL_CULL_FACE);gl.Disable(GL_DEPTH_TEST);gl.DepthFunc(GL_GREATER);gl.DepthMask(GL_FALSE);gl.ClipControl(GL_UPPER_LEFT,GL_NEGATIVE_ONE_TO_ONE);
 r=backend.draw_code_diagnostic(world,frame,view);check(r.drawn,"State check draw failed");GLint value=0;context->api().GetIntegerv(GL_ACTIVE_TEXTURE,&value);check(value==GL_TEXTURE7,"Material leaked active texture");for(unsigned i=0;i<6;++i){active(GL_TEXTURE0+i);context->api().GetIntegerv(GL_TEXTURE_BINDING_2D,&value);check(value==GLint(fog.ramp().id()),"Material leaked 2D texture");context->api().GetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP,&value);check(value==GLint(cube.id()),"Material leaked cube texture");context->api().GetIntegerv(GL_SAMPLER_BINDING,&value);check(value==GLint(prior_sampler),"Material leaked sampler");}active(GL_TEXTURE7);check(gl.IsEnabled(GL_BLEND)&&gl.IsEnabled(GL_CULL_FACE)&&!gl.IsEnabled(GL_DEPTH_TEST),"Material leaked enables");context->api().GetIntegerv(GL_DEPTH_FUNC,&value);check(value==GL_GREATER,"Material leaked depth convention");context->api().GetIntegerv(GL_DEPTH_WRITEMASK,&value);check(value==0,"Material leaked depth mask");context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&value);check(value==GLint(fbo),"Material replaced framebuffer");context->api().GetIntegerv(GL_CLIP_ORIGIN,&value);check(value==GL_UPPER_LEFT,"Material leaked clip origin");
 auto bad=view;bad.scene_revision++;check(!backend.draw_code_diagnostic(world,frame,bad).drawn,"Stale material scene accepted");bad=view;bad.fog_owner=nullptr;check(!backend.draw_code_diagnostic(world,frame,bad).drawn,"Missing fog accepted");bad=view;bad.camera_world[0]++;check(!backend.draw_code_diagnostic(world,frame,bad).drawn,"Foreign camera accepted");bad=view;bad.world_visibility_gate=false;check(!backend.draw_code_diagnostic(world,frame,bad).drawn,"FOW gate ignored");
 GlDistanceFog foreign_fog(*context,fog_assets);bad=view;bad.fog_owner=&foreign_fog;check(!backend.draw_code_diagnostic(world,frame,bad).drawn,"Foreign same-context fog owner accepted");
 clear();const auto plain=backend.draw_diagnostic(world,frame);check(plain.drawn&&plain.members==12&&!plain.model_drawable,"Explicit ordinary SDK diagnostic was replaced by CODE");
 rejected=false;std::thread worker([&]{try{backend.update_code_visibility(states,4,unit,epoch);}catch(const std::logic_error&){rejected=true;}});worker.join();check(rejected,"Foreign render thread accepted");
 // SDK pose changes update existing pose owners, never packed buffers.
 if(format4){const auto before=backend.buffer_uploads();backend.select_sdk_sample(2);check(backend.buffer_uploads()==before+6,"Format4 stream was reuploaded during SDK pose change");draw();backend.select_sdk_sample(0);check(backend.buffer_uploads()==before+12,"Format4 source buffer was replaced");}
 std::ofstream ppm(output/"retained-source-material.ppm",std::ios::binary);ppm<<"P6\n512 512\n255\n";for(int y=511;y>=0;--y)for(unsigned x=0;x<512;++x)ppm.write(reinterpret_cast<const char*>(india.data()+(std::size_t(y)*512+x)*4),3);check(bool(ppm),"Image write failed");
 std::ofstream receipt(output/"retained-material-receipt.json");receipt<<"{\"status\":\"PASS\",\"members\":12,\"triangles\":7920,\"sourceColorChangedBytes\":"<<color_delta<<",\"visibilityFlushes\":"<<backend.code_material_state().visibility_flushes<<",\"actualSourceDds\":9,\"samplers\":6,\"modelDrawable\":false,\"fullGraphicsParity\":false,\"controlledInspectionPlacement\":true,\"importedRoster\":false,\"nativeOptimizedVertexBuffer\":false,\"sourceFormat4SdkPoseDiagnostic\":"<<(format4?"true":"false")<<"}\n";check(bool(receipt),"Receipt write failed");
 context->check("SAME retained Warrior material bridge checks");gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.DeleteFramebuffers(1,&fbo);gl.DeleteSamplers(1,&prior_sampler);std::cout<<"PASS SAME backend source material/c0/c5,12 packets,live borrowed XML colors,fog/samplers,state restoration,fades; native readiness false\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
