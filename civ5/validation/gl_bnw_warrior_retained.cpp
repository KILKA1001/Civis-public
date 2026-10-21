#include "gl_bnw_warrior_retained.hpp"
#include "bnw_warrior_gr2_metadata_cache.hpp"
#include "bnw_warrior_material_sources.hpp"
#include "bnw_warrior_material_pose.hpp"
#include "bnw_warrior_native_vertex_cache.hpp"
#include "civ5/graphics/bnw_warrior_code_material.hpp"
#include "civ5/graphics/bnw_warrior_owned_material_uniforms.hpp"
#include "civ5/graphics/gl_bnw_warrior_material_samplers.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace civ5::graphics {
namespace {
constexpr auto gandhi_save="b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab";
constexpr auto original_exe="7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
bool digest(std::string_view s) {
    return s.size()==64 && std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
void unique(std::string_view json,std::string_view key) {
    const auto tag='"'+std::string(key)+'"';const auto first=json.find(tag);
    require(first!=std::string_view::npos && json.find(tag,first+tag.size())==std::string_view::npos,"Warrior placement field missing or duplicated");
}
template<class T> std::vector<T> numbers(std::string_view json,std::string_view key,std::size_t count) {
    unique(json,key);return warrior_json_numbers<T>(warrior_json_array(json,key),count);
}
std::string string(std::string_view json,std::string_view key) {
    unique(json,key);return warrior_json_string(json,key);
}
void matrix_valid(const terrain::Matrix4 &m,bool placement) {
    for(const auto value:m) require(std::isfinite(value),"Warrior matrix nonfinite");
    if(!placement) return;
    require(m[3]==0 && m[7]==0 && m[11]==0 && m[15]==1,"Warrior placement matrix is not affine");
    for(int row=0;row<3;++row) {
        double q=0;for(int col=0;col<3;++col)q+=double(m[row*4+col])*m[row*4+col];
        require(std::abs(std::sqrt(q)-0.14)<0.00001,"Warrior placement scale differs from authored 0.14");
    }
}
std::int64_t unit_core(const saves::SavedUnitObservation &unit,std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &f:unit.core.fields) if(f.name && name==f.name) {
        require(!found,"Duplicate Warrior unit core field");found=&f;
    }
    require(found && found->values.size()==1,"Warrior unit core field unavailable");
    return found->values[0];
}
GLuint create_program(GlRenderApi &gl) {
    const char *vs="#version 460 core\nlayout(location=0) in vec3 position;layout(location=2) in vec2 sourceUV;"
        "uniform mat4 memberWorld,view,projection;out vec2 uv;"
        "void main(){gl_Position=projection*view*memberWorld*vec4(position,1);uv=sourceUV;}";
    const char *fs="#version 460 core\nin vec2 uv;uniform sampler2D authoredDiffuse;out vec4 pixel;"
        "void main(){pixel=texture(authoredDiffuse,uv);}";
    const auto p=gl.CreateProgram();
    try {
        for(const auto &[stage,source]:{std::pair{GL_VERTEX_SHADER,vs},std::pair{GL_FRAGMENT_SHADER,fs}}) {
            const auto s=gl.CreateShader(stage);gl.ShaderSource(s,1,&source,nullptr);gl.CompileShader(s);
            GLint ok=0;gl.GetShaderiv(s,GL_COMPILE_STATUS,&ok);
            if(!ok) {char log[2048]{};gl.GetShaderInfoLog(s,sizeof(log),nullptr,log);gl.DeleteShader(s);throw std::runtime_error(log);}
            gl.AttachShader(p,s);gl.DeleteShader(s);
        }
        gl.LinkProgram(p);GLint ok=0;gl.GetProgramiv(p,GL_LINK_STATUS,&ok);
        if(!ok) {char log[2048]{};gl.GetProgramInfoLog(p,sizeof(log),nullptr,log);throw std::runtime_error(log);}
        return p;
    } catch(...) {gl.DeleteProgram(p);throw;}
}
GLuint create_material_program(GlRenderApi &gl,bool code,bool format4=false) {
    const char *vs=R"GLSL(#version 460 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 rawNormal;
layout(location=2) in vec2 sourceUV;
uniform mat4 memberWorld,view,projection;
uniform vec3 f3CameraWorld;
out vec4 warriorNormalU,warriorViewV;
out vec3 warriorWorld,warriorClip;
void main(){
 vec4 world=memberWorld*vec4(position,1);
 vec4 clip=projection*view*world;
 gl_Position=clip;
 warriorNormalU=vec4(mat3(memberWorld)*rawNormal,sourceUV.x);
 warriorViewV=vec4(world.xyz-f3CameraWorld,sourceUV.y);
 warriorWorld=world.xyz;warriorClip=vec3(clip.x,clip.y,clip.w);
}
)GLSL";
    if(format4)vs=bnw_warrior_native_format4_vertex_shader;
    const auto &fragment=code?bnw_warrior_code_material_fragment:std::string(bnw_warrior_main_material_fragment);
    const char *fs=fragment.c_str();const auto p=gl.CreateProgram();
    try {
        for(const auto &[stage,source]:{std::pair{GL_VERTEX_SHADER,vs},std::pair{GL_FRAGMENT_SHADER,fs}}) {
            const auto s=gl.CreateShader(stage);gl.ShaderSource(s,1,&source,nullptr);gl.CompileShader(s);
            GLint ok=0;gl.GetShaderiv(s,GL_COMPILE_STATUS,&ok);
            if(!ok){char log[2048]{};gl.GetShaderInfoLog(s,sizeof(log),nullptr,log);gl.DeleteShader(s);throw std::runtime_error(log);}
            gl.AttachShader(p,s);gl.DeleteShader(s);
        }
        gl.LinkProgram(p);GLint ok=0;gl.GetProgramiv(p,GL_LINK_STATUS,&ok);
        if(!ok){char log[2048]{};gl.GetProgramInfoLog(p,sizeof(log),nullptr,log);throw std::runtime_error(log);}
        return p;
    }catch(...){gl.DeleteProgram(p);throw;}
}
} // namespace

BnwWarriorPlacementReceipt load_bnw_warrior_placement_receipt(const BnwWarriorPlacementPin &pin) {
    require(digest(pin.record_sha256) && digest(pin.native_snapshot_sha256),"Warrior placement needs an independently accepted native capture pin");
    require_bnw_warrior_asset_sha(runtime::identify_file(pin.record).sha256,pin.record_sha256);
    require_bnw_warrior_asset_sha(runtime::identify_file(pin.native_snapshot).sha256,pin.native_snapshot_sha256);
    const auto json=warrior_read_bounded(pin.record,100'000);
    require(string(json,"schema")=="civis-bnw-warrior-native-placement-v1" &&
            string(json,"sourceKind")=="passive-original-dx9-unit-transforms",
            "Warrior placement is not an accepted native unit transform capture");
    require(string(json,"sourceSaveSha256")==gandhi_save && string(json,"sourceExeSha256")==original_exe,
            "Warrior placement original save or EXE differs");
    require(string(json,"nativeSnapshotSha256")==pin.native_snapshot_sha256,"Warrior native transform snapshot differs");
    require(numbers<int>(json,"unitIdentity",5)==std::vector<int>{0,16385,592,32,14},"Warrior capture unit/plot differs");
    require(numbers<int>(json,"memberArt",12)==std::vector<int>{0,0,0,1,1,1,1,1,1,2,2,2},"Warrior captured 3/6/3 art mapping differs");
    const auto heading=numbers<int>(json,"facingDirection",1)[0];
    require(heading>=0 && heading<6,"Warrior captured heading unavailable");
    const auto matrices=numbers<float>(json,"memberWorldRowVector",12*16);
    BnwWarriorPlacementReceipt result;result.save_=gandhi_save;result.record_=pin.record_sha256;result.facing_=heading;
    for(std::size_t i=0;i<12;++i) {
        std::copy_n(matrices.begin()+i*16,16,result.member_world_[i].begin());
        matrix_valid(result.member_world_[i],true);
    }
    return result;
}

struct GlBnwWarriorRetainedBackend::Impl {
    struct Member {
        BnwWarriorSourceMesh mesh;BnwWarriorSkinDiagnostic skin;
        GLuint vao=0,vbo=0,ibo=0;std::unique_ptr<GlTexture> texture;
    };
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    std::thread::id owner=std::this_thread::get_id();
    std::array<Member,3> members;
    BnwWarriorGr2MetadataCache metadata;
    std::filesystem::path assets,cache;
    struct CodeMaterial {
        struct Uniforms {GLint world,view,projection,camera,team,fog,color,water,alpha;GLint bones=-1;};
        struct NativeFormat4 {
            GlRenderApi gl;
            std::array<GLuint,3> vao{},vbo{};
            std::array<GLuint,2> programs{};
            std::array<Uniforms,2> uniforms{};
            explicit NativeFormat4(const GlContext& context):gl(context){}
            ~NativeFormat4(){for(auto id:programs)if(id)gl.DeleteProgram(id);for(auto id:vao)if(id)gl.DeleteVertexArrays(1,&id);for(auto id:vbo)if(id)gl.DeleteBuffers(1,&id);}
        };
        std::unique_ptr<NativeFormat4> native;
        GlRenderApi gl;
        const runtime::PregameRoster *roster;
        const content::RulesDatabase *rules;
        std::int32_t player,unit;
        std::uint64_t revision;
        BnwWarriorRetainedMaterialPackets packets;
        std::array<std::unique_ptr<GlTexture>,9> owned_textures;
        std::array<const GlTexture*,9> resources{};
        std::array<std::unique_ptr<GlBnwWarriorMaterialSamplers>,3> samplers;
        std::array<GLuint,3> vao{},vbo{};
        std::array<GLuint,2> programs{};
        std::array<Uniforms,2> uniforms{};
        std::array<std::size_t,3> fourth{};
        const GlDistanceFog *fog_owner=nullptr;
        CodeMaterial(const GlContext &context,const runtime::PregameRoster& r,
            const content::RulesDatabase& d,std::int32_t p,std::int32_t u,std::uint64_t epoch)
            :gl(context),roster(&r),rules(&d),player(p),unit(u),revision(epoch),packets(u,epoch) {
            (void)project_bnw_warrior_team_color_rules(r,d,p,u,epoch);
        }
        ~CodeMaterial() {
            for(auto id:programs)if(id)gl.DeleteProgram(id);
            for(auto id:vao)if(id)gl.DeleteVertexArrays(1,&id);
            for(auto id:vbo)if(id)gl.DeleteBuffers(1,&id);
        }
    };
    std::unique_ptr<CodeMaterial> code;
    GLuint program=0;
    GLint world_uniform=-1,view_uniform=-1,projection_uniform=-1;
    int sample=-1;std::size_t uploads=0;
    explicit Impl(std::shared_ptr<GlContext> c):context(std::move(c)),gl(*context) {}
    void thread() const {if(owner!=std::this_thread::get_id())throw std::logic_error("Warrior GL call requires its owning thread");(void)context->api();}
    ~Impl() {
        // Cross-thread destruction cannot issue GL deletion on another context.
        if(owner!=std::this_thread::get_id())std::terminate();
        (void)context->api();
        for(auto &m:members) {
            if(m.vao)gl.DeleteVertexArrays(1,&m.vao);
            if(m.vbo)gl.DeleteBuffers(1,&m.vbo);
            if(m.ibo)gl.DeleteBuffers(1,&m.ibo);
        }
        if(program)gl.DeleteProgram(program);
    }
    GlBnwWarriorDrawResult draw(const std::array<terrain::Matrix4,12> &world,const GlBnwWarriorFrame &frame) {
        thread();require(frame.width>0 && frame.height>0,"Warrior viewport unavailable");
        matrix_valid(frame.view,false);matrix_valid(frame.projection,false);
        GLint viewport[4]{};context->api().GetIntegerv(GL_VIEWPORT,viewport);
        require(viewport[2]==frame.width && viewport[3]==frame.height,"Warrior frame differs from current host viewport");
        require(frame.depth_function==GL_LESS || frame.depth_function==GL_LEQUAL ||
                frame.depth_function==GL_GREATER || frame.depth_function==GL_GEQUAL,"Warrior depth convention unavailable");
        for(const auto &m:world)matrix_valid(m,true);
        const auto &api=context->api();
        const auto active=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
        GLint prior_program=0,prior_vao=0,prior_active=0,prior_texture=0,prior_sampler=0,prior_depth_func=0;
        GLint prior_depth_mask=0,prior_origin=0,prior_clip=0;
        api.GetIntegerv(GL_CURRENT_PROGRAM,&prior_program);api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&prior_vao);
        api.GetIntegerv(GL_ACTIVE_TEXTURE,&prior_active);active(GL_TEXTURE0);
        api.GetIntegerv(GL_TEXTURE_BINDING_2D,&prior_texture);api.GetIntegerv(GL_SAMPLER_BINDING,&prior_sampler);
        api.GetIntegerv(GL_DEPTH_FUNC,&prior_depth_func);api.GetIntegerv(GL_DEPTH_WRITEMASK,&prior_depth_mask);
        api.GetIntegerv(GL_CLIP_ORIGIN,&prior_origin);api.GetIntegerv(GL_CLIP_DEPTH_MODE,&prior_clip);
        const bool depth=gl.IsEnabled(GL_DEPTH_TEST),blend=gl.IsEnabled(GL_BLEND),cull=gl.IsEnabled(GL_CULL_FACE);
        const auto restore=[&] {
            gl.UseProgram(prior_program);gl.BindVertexArray(prior_vao);
            api.BindTexture(GL_TEXTURE_2D,prior_texture);gl.BindSampler(0,prior_sampler);active(prior_active);
            gl.DepthFunc(prior_depth_func);gl.DepthMask(prior_depth_mask?GL_TRUE:GL_FALSE);
            gl.ClipControl(prior_origin,prior_clip);
            (depth?gl.Enable:gl.Disable)(GL_DEPTH_TEST);(blend?gl.Enable:gl.Disable)(GL_BLEND);(cull?gl.Enable:gl.Disable)(GL_CULL_FACE);
        };
        GlBnwWarriorDrawResult result;result.sdk_sample_index=sample;
        try {
            gl.UseProgram(program);gl.Enable(GL_DEPTH_TEST);gl.Disable(GL_BLEND);gl.Disable(GL_CULL_FACE);
            gl.DepthFunc(frame.depth_function);gl.DepthMask(GL_TRUE);gl.ClipControl(GL_LOWER_LEFT,GL_ZERO_TO_ONE);
            gl.ProgramUniformMatrix4fv(program,view_uniform,1,GL_FALSE,frame.view.data());
            gl.ProgramUniformMatrix4fv(program,projection_uniform,1,GL_FALSE,frame.projection.data());
            std::size_t instance=0;
            for(std::size_t i=0;i<3;++i) {
                auto &m=members[i];m.texture->bind(0);gl.BindSampler(0,0);gl.BindVertexArray(m.vao);
                for(int count=0;count<bnw_gandhi_warrior_art[i].count;++count) {
                    gl.ProgramUniformMatrix4fv(program,world_uniform,1,GL_FALSE,world[instance++].data());
                    gl.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(m.mesh.indices.size()),GL_UNSIGNED_INT,nullptr);
                    result.triangles+=m.mesh.indices.size()/3;
                }
            }
            context->check("Draw retained source-skinned Warrior diagnostic");
            result.drawn=true;result.members=instance;restore();return result;
        } catch(...) {restore();throw;}
    }
    GlBnwWarriorDrawResult draw_code(const std::array<terrain::Matrix4,12>& world,
        const GlBnwWarriorFrame& frame,const GlBnwWarriorMaterialView& source) {
        thread();require(bool(code),"Missing retained Warrior CODE material owner");auto& c=*code;
        require(source.unit==c.unit&&source.scene_revision==c.revision,"Foreign/stale Warrior CODE scene owner");
        require(std::isfinite(source.renderer_clock),"Missing current renderer clock");
        for(auto x:source.camera_world)require(std::isfinite(x),"Nonfinite Warrior camera owner");
        require(source.fog_owner&&source.fog_owner->ramp().belongs_to(*context),"Missing/foreign SAME-view fog resource");
        require(!c.fog_owner||c.fog_owner==source.fog_owner,"Foreign retained Warrior fog owner");
        const auto& fog_assets=source.fog_owner->assets();
        require_bnw_warrior_asset_sha(runtime::identify_bytes(fog_assets.ramp().original_bytes()).sha256,
            "948e51cf9d409918fc42006234e117374c84df8a31dcce19a9781af225bb9d3d");
        require_bnw_warrior_asset_sha(runtime::identify_bytes(fog_assets.color_bytes()).sha256,
            "efc94e89ed0a4263c4a5477bf26e1720b03a9aee2a39b9c093e826ba2c3aa1f8");
        require(fog_assets.state().loaded,"Missing loaded source fog owner");
        const auto fog=materials::terrain_fog_constants(source.fog_owner->frame(source.normalized_zoom),source.camera_world[2],source.fog_precision);
        const auto team=project_bnw_warrior_team_color_rules(*c.roster,*c.rules,c.player,c.unit,c.revision);
        require(frame.width>0&&frame.height>0,"Missing Warrior CODE framebuffer extent");
        matrix_valid(frame.view,false);matrix_valid(frame.projection,false);
        // Verify borrowed cameraWorld belongs to this row-vector view. Allow
        // only floating-point inverse/multiply error proportional to operands.
        for(unsigned column=0;column<3;++column) {
            double value=frame.view[12+column],magnitude=std::abs(value);
            for(unsigned row=0;row<3;++row){const double product=double(source.camera_world[row])*frame.view[row*4+column];value+=product;magnitude+=std::abs(product);}
            require(std::abs(value)<=64*std::numeric_limits<float>::epsilon()*std::max(1.,magnitude),"Warrior cameraWorld differs from SAME view");
        }
        for(const auto& m:world)matrix_valid(m,true);
        std::array<BnwWarriorNativePaletteRows,12> packed_rows{};
        if(c.native)for(unsigned member=0;member<12;++member){const unsigned variant=member<3?0:member<9?1:2;packed_rows[member]=bnw_warrior_sdk_world_palette_rows(members[variant].skin,sample,world[member]);}
        GLint viewport[4]{};context->api().GetIntegerv(GL_VIEWPORT,viewport);
        require(viewport[2]==frame.width&&viewport[3]==frame.height,"CODE frame differs from SAME host viewport");
        require(frame.depth_function==GL_LESS||frame.depth_function==GL_LEQUAL||frame.depth_function==GL_GREATER||frame.depth_function==GL_GEQUAL,"Missing CODE depth convention");
        c.fog_owner=source.fog_owner;
        c.packets.tick(source.renderer_clock,c.unit,c.revision);
        GlBnwWarriorDrawResult result;result.sdk_sample_index=sample;
        result.blocker="source PS diagnostic with SDK four influences; native optimized vertex buffer, final native blend/draw and imported transient visuals unavailable";
        if(c.native)result.blocker="source-format4 three-input shader with SDK poses; native animation, final blend/draw and imported transient visuals unavailable";
        if(!source.world_visibility_gate){result.blocker="SAME world FOW gate hides Warrior";return result;}
        const auto& api=context->api();
        const auto active=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
        GLint prior_program=0,prior_vao=0,prior_active=0,prior_depth_func=0,prior_depth_mask=0,prior_origin=0,prior_clip=0;
        std::array<GLint,6> prior_2d{},prior_cube{},prior_sampler{};
        api.GetIntegerv(GL_CURRENT_PROGRAM,&prior_program);api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&prior_vao);api.GetIntegerv(GL_ACTIVE_TEXTURE,&prior_active);
        api.GetIntegerv(GL_DEPTH_FUNC,&prior_depth_func);api.GetIntegerv(GL_DEPTH_WRITEMASK,&prior_depth_mask);api.GetIntegerv(GL_CLIP_ORIGIN,&prior_origin);api.GetIntegerv(GL_CLIP_DEPTH_MODE,&prior_clip);
        const bool depth=gl.IsEnabled(GL_DEPTH_TEST),blend=gl.IsEnabled(GL_BLEND),cull=gl.IsEnabled(GL_CULL_FACE);
        for(unsigned s=0;s<6;++s){active(GL_TEXTURE0+s);api.GetIntegerv(GL_TEXTURE_BINDING_2D,&prior_2d[s]);api.GetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP,&prior_cube[s]);api.GetIntegerv(GL_SAMPLER_BINDING,&prior_sampler[s]);}
        const auto restore=[&] {
            gl.UseProgram(prior_program);gl.BindVertexArray(prior_vao);
            for(unsigned s=0;s<6;++s){active(GL_TEXTURE0+s);api.BindTexture(GL_TEXTURE_2D,prior_2d[s]);api.BindTexture(GL_TEXTURE_CUBE_MAP,prior_cube[s]);gl.BindSampler(s,prior_sampler[s]);}
            active(prior_active);gl.DepthFunc(prior_depth_func);gl.DepthMask(prior_depth_mask?GL_TRUE:GL_FALSE);gl.ClipControl(prior_origin,prior_clip);
            (depth?gl.Enable:gl.Disable)(GL_DEPTH_TEST);(blend?gl.Enable:gl.Disable)(GL_BLEND);(cull?gl.Enable:gl.Disable)(GL_CULL_FACE);
        };
        try {
            // Explicit shader-stage diagnostic: no inferred native blending.
            gl.Enable(GL_DEPTH_TEST);gl.Disable(GL_BLEND);gl.Disable(GL_CULL_FACE);gl.DepthFunc(frame.depth_function);gl.DepthMask(GL_TRUE);gl.ClipControl(GL_LOWER_LEFT,GL_ZERO_TO_ONE);
            for(unsigned member:c.packets.draw_order()) {
                const unsigned variant=member<3?0:member<9?1:2;
                const auto family=c.packets.family(member,c.unit,c.revision);
                require(family==1||family==2,"Unknown retained Warrior packet shader family");
                const unsigned stage=family==2?1:0;const auto program=c.native?c.native->programs[stage]:c.programs[stage];const auto& u=c.native?c.native->uniforms[stage]:c.uniforms[stage];
                gl.UseProgram(program);gl.BindVertexArray(c.native?c.native->vao[variant]:c.vao[variant]);
                for(unsigned s=0;s<5;++s)c.resources[s<2?variant*2+s:s+4]->bind(s);
                source.fog_owner->ramp().bind(5);
                for(unsigned s=0;s<6;++s)gl.BindSampler(s,c.samplers[variant]->sampler(*context,s,variant,c.unit,c.revision));
                if(c.native)gl.ProgramUniform4fv(program,u.bones,60,packed_rows[member].data());
                else gl.ProgramUniformMatrix4fv(program,u.world,1,GL_FALSE,world[member].data());
                gl.ProgramUniformMatrix4fv(program,u.view,1,GL_FALSE,frame.view.data());gl.ProgramUniformMatrix4fv(program,u.projection,1,GL_FALSE,frame.projection.data());
                gl.ProgramUniform3f(program,u.camera,source.camera_world[0],source.camera_world[1],source.camera_world[2]);gl.ProgramUniform4fv(program,u.team,1,team.team_color_s.data());gl.ProgramUniform4fv(program,u.fog,1,fog[1].data());gl.ProgramUniform3f(program,u.color,fog[0][0],fog[0][1],fog[0][2]);
                if(stage==1) {
                    const auto alpha=bnw_warrior_code_alpha_from_owned_frame(c.packets.frame(),c.unit,c.revision,member,world[member][14]);
                    gl.ProgramUniform1f(program,u.water,alpha.water_height);gl.ProgramUniform3f(program,u.alpha,alpha.alpha_values_s[0],alpha.alpha_values_s[1],alpha.alpha_values_s[2]);
                }
                gl.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(members[variant].mesh.indices.size()),GL_UNSIGNED_INT,nullptr);++result.members;result.triangles+=members[variant].mesh.indices.size()/3;
            }
            context->check("Draw SAME retained Warrior source material packets diagnostic");result.drawn=result.members>0;restore();return result;
        }catch(...){restore();throw;}
    }
};

GlBnwWarriorRetainedBackend::GlBnwWarriorRetainedBackend(std::shared_ptr<GlContext> context,
    const std::filesystem::path &assets,const std::filesystem::path &cache) {
    require(bool(context),"Warrior retained context unavailable");
    impl_=std::make_unique<Impl>(std::move(context));auto &p=*impl_;
    p.assets=assets;p.cache=cache;
    p.metadata=load_bnw_warrior_gr2_metadata_cache(cache);
    require(probe_bnw_gandhi_warrior_art(assets,*p.context)==48,"Warrior retained art chain differs");
    p.program=create_program(p.gl);
    p.world_uniform=p.gl.GetUniformLocation(p.program,"memberWorld");
    p.view_uniform=p.gl.GetUniformLocation(p.program,"view");p.projection_uniform=p.gl.GetUniformLocation(p.program,"projection");
    const auto diffuse=p.gl.GetUniformLocation(p.program,"authoredDiffuse");
    require(p.world_uniform>=0 && p.view_uniform>=0 && p.projection_uniform>=0 && diffuse>=0,"Warrior retained shader uniforms unavailable");
    p.gl.ProgramUniform1i(p.program,diffuse,0);
    const assets::AssetEnvironment environment(assets);
    const auto archive=environment.open_fpk_archive("Resource/DX9/UnitsTextures.fpk");
    const auto *textures=std::get_if<std::shared_ptr<assets::FpkSource>>(&archive);
    require(textures,"Warrior retained texture archive unavailable");
    constexpr std::array<std::size_t,3> vertices{829,863,796},triangles{654,674,638};
    for(std::size_t i=0;i<3;++i) {
        auto &m=p.members[i];const auto &spec=bnw_gandhi_warrior_art[i];
        m.mesh=load_bnw_warrior_source_mesh(cache,spec,vertices[i],triangles[i]);
        m.skin=load_bnw_warrior_skin_diagnostic(cache,spec,vertices[i]);
        const auto bytes=warrior_archive_asset(*textures,spec.diffuse_name);
        require_bnw_warrior_asset_sha(runtime::identify_bytes(bytes).sha256,spec.diffuse_sha256);
        m.texture=std::make_unique<GlTexture>(*p.context,assets::DdsData::parse(bytes),DdsSamplingColor::linear);
        p.gl.CreateVertexArrays(1,&m.vao);p.gl.CreateBuffers(1,&m.vbo);p.gl.CreateBuffers(1,&m.ibo);
        const auto posed=m.skin.pose(m.mesh);
        p.gl.NamedBufferData(m.vbo,static_cast<GLsizeiptr>(posed.size()*sizeof(float)),posed.data(),GL_DYNAMIC_DRAW);
        p.gl.NamedBufferData(m.ibo,static_cast<GLsizeiptr>(m.mesh.indices.size()*sizeof(std::uint32_t)),m.mesh.indices.data(),GL_STATIC_DRAW);
        p.uploads+=2;
        p.gl.VertexArrayVertexBuffer(m.vao,0,m.vbo,0,8*sizeof(float));p.gl.VertexArrayElementBuffer(m.vao,m.ibo);
        for(GLuint a=0;a<3;++a) {p.gl.EnableVertexArrayAttrib(m.vao,a);p.gl.VertexArrayAttribBinding(m.vao,a,0);}
        p.gl.VertexArrayAttribFormat(m.vao,0,3,GL_FLOAT,GL_FALSE,0);
        p.gl.VertexArrayAttribFormat(m.vao,1,3,GL_FLOAT,GL_FALSE,3*sizeof(float));
        p.gl.VertexArrayAttribFormat(m.vao,2,2,GL_FLOAT,GL_FALSE,6*sizeof(float));
    }
    p.context->check("Retain verified Warrior source mesh/skin resources");
}
GlBnwWarriorRetainedBackend::~GlBnwWarriorRetainedBackend()=default;
std::size_t GlBnwWarriorRetainedBackend::buffer_uploads() const noexcept {return impl_->uploads;}
float GlBnwWarriorRetainedBackend::native_model_scale(float compiled_scale) const {
    impl_->thread();return impl_->metadata.native_model_scale(compiled_scale);
}
const std::string &GlBnwWarriorRetainedBackend::metadata_sha256() const noexcept {return impl_->metadata.metadata_sha256;}
void GlBnwWarriorRetainedBackend::select_sdk_sample(int index) {
    auto &p=*impl_;p.thread();require(index>=-1 && index<=3,"Warrior unsupported SDK time");
    if(index==p.sample)return;
    // Compute every pose before replacing any GPU buffer.
    std::array<std::vector<float>,3> poses;
    std::array<BnwWarriorMaterialPose,3> raw;
    for(std::size_t i=0;i<3;++i)poses[i]=p.members[i].skin.pose(p.members[i].mesh,index);
    if(p.code)for(std::size_t i=0;i<3;++i)raw[i]=prepare_bnw_warrior_material_pose(p.members[i].mesh,p.members[i].skin,index);
    for(std::size_t i=0;i<3;++i) {
        p.gl.NamedBufferSubData(p.members[i].vbo,0,static_cast<GLsizeiptr>(poses[i].size()*sizeof(float)),poses[i].data());++p.uploads;
        if(p.code){p.gl.NamedBufferSubData(p.code->vbo[i],0,static_cast<GLsizeiptr>(raw[i].vertex_pnuv.size()*sizeof(float)),raw[i].vertex_pnuv.data());++p.uploads;}
    }
    p.context->check("Update retained Warrior SDK pose buffers");p.sample=index;
}
void GlBnwWarriorRetainedBackend::retain_code_material(const std::filesystem::path& metadata,
    const std::filesystem::path& sampler_metadata,const runtime::PregameRoster& roster,
    const content::RulesDatabase& rules,std::int32_t player,std::int32_t unit,std::uint64_t revision) {
    auto& p=*impl_;p.thread();require(!p.code,"Warrior material already retained; stale owner needs replacement");
    require_bnw_warrior_asset_sha(runtime::identify_file(sampler_metadata).sha256,bnw_warrior_sampler_metadata_sha);
    const auto sources=load_bnw_warrior_material_sources(p.assets,p.cache,metadata);
    auto c=std::make_unique<Impl::CodeMaterial>(*p.context,roster,rules,player,unit,revision);
    for(unsigned i=0;i<9;++i) {
        if(i<6&&i%2==0)c->resources[i]=p.members[i/2].texture.get();
        else {c->owned_textures[i]=std::make_unique<GlTexture>(*p.context,sources.resources.at(i).data,DdsSamplingColor::linear);c->resources[i]=c->owned_textures[i].get();}
        require(c->resources[i]->belongs_to(*p.context)&&c->resources[i]->face_count()==(i<6?1u:6u),"Missing/foreign Warrior material texture owner");
    }
    for(unsigned i=0;i<3;++i) {
        const BnwWarriorNativeSamplerPolicy policy(sources.metadata_sha256,bnw_warrior_sampler_metadata_sha,i,unit,revision);
        c->samplers[i]=std::make_unique<GlBnwWarriorMaterialSamplers>(p.context,policy);
        const auto raw=prepare_bnw_warrior_material_pose(p.members[i].mesh,p.members[i].skin,p.sample);
        c->fourth[i]=raw.nonzero_fourth_influences;
        p.gl.CreateVertexArrays(1,&c->vao[i]);p.gl.CreateBuffers(1,&c->vbo[i]);
        p.gl.NamedBufferData(c->vbo[i],static_cast<GLsizeiptr>(raw.vertex_pnuv.size()*sizeof(float)),raw.vertex_pnuv.data(),GL_DYNAMIC_DRAW);
        p.gl.VertexArrayVertexBuffer(c->vao[i],0,c->vbo[i],0,8*sizeof(float));p.gl.VertexArrayElementBuffer(c->vao[i],p.members[i].ibo);
        for(GLuint a=0;a<3;++a){p.gl.EnableVertexArrayAttrib(c->vao[i],a);p.gl.VertexArrayAttribBinding(c->vao[i],a,0);}
        p.gl.VertexArrayAttribFormat(c->vao[i],0,3,GL_FLOAT,GL_FALSE,0);p.gl.VertexArrayAttribFormat(c->vao[i],1,3,GL_FLOAT,GL_FALSE,3*sizeof(float));p.gl.VertexArrayAttribFormat(c->vao[i],2,2,GL_FLOAT,GL_FALSE,6*sizeof(float));
    }
    require(c->fourth==std::array<std::size_t,3>{48,0,8},"Actual fourth-weight diagnostic frontier changed");
    for(unsigned stage=0;stage<2;++stage) {
        const auto program=c->programs[stage]=create_material_program(p.gl,stage==1);
        const auto location=[&](const char* name,bool mandatory=true){const auto v=p.gl.GetUniformLocation(program,name);require(v>=0||!mandatory,"Missing source material shader uniform");return v;};
        c->uniforms[stage]={location("memberWorld"),location("view"),location("projection"),location("f3CameraWorld"),location("TeamColorS"),location("g_FogConsts"),location("f3FogColor"),location("fWaterHeight",stage==1),location("AlphaValuesS",stage==1)};
    }
    p.context->check("Retain exact Warrior source material/samplers/raw SDK pose on SAME backend");p.uploads+=3;p.code=std::move(c);
}
void GlBnwWarriorRetainedBackend::retain_native_format4_diagnostic() {
    auto& p=*impl_;p.thread();require(bool(p.code),"Native format4 requires SAME retained material owner");
    require(!p.code->native,"Native format4 diagnostic already retained");
    // No GL allocations until every full stream, original weight/index byte,
    // pinned named palette and source-format4 refusal has passed.
    std::array<BnwWarriorNativeFormat4Vertices,3> streams;
    for(unsigned i=0;i<3;++i){streams[i]=load_bnw_warrior_native_vertex_cache(p.cache,bnw_gandhi_warrior_art[i],p.members[i].mesh,p.members[i].skin);(void)p.members[i].skin.matrices(p.sample);}
    auto native=std::make_unique<Impl::CodeMaterial::NativeFormat4>(*p.context);
    for(unsigned i=0;i<3;++i){
        p.gl.CreateVertexArrays(1,&native->vao[i]);p.gl.CreateBuffers(1,&native->vbo[i]);
        p.gl.NamedBufferData(native->vbo[i],static_cast<GLsizeiptr>(streams[i].bytes.size()),streams[i].bytes.data(),GL_STATIC_DRAW);
        p.gl.VertexArrayVertexBuffer(native->vao[i],0,native->vbo[i],0,28);p.gl.VertexArrayElementBuffer(native->vao[i],p.members[i].ibo);
        for(GLuint a=0;a<5;++a){p.gl.EnableVertexArrayAttrib(native->vao[i],a);p.gl.VertexArrayAttribBinding(native->vao[i],a,0);}
        p.gl.VertexArrayAttribFormat(native->vao[i],0,4,GL_HALF_FLOAT,GL_FALSE,0);
        p.gl.VertexArrayAttribFormat(native->vao[i],1,4,GL_HALF_FLOAT,GL_FALSE,16);
        p.gl.VertexArrayAttribFormat(native->vao[i],2,2,GL_HALF_FLOAT,GL_FALSE,24);
        p.gl.VertexArrayAttribFormat(native->vao[i],3,4,GL_UNSIGNED_BYTE,GL_TRUE,8);
        p.gl.VertexArrayAttribFormat(native->vao[i],4,4,GL_UNSIGNED_BYTE,GL_FALSE,12);
    }
    for(unsigned stage=0;stage<2;++stage){
        const auto program=native->programs[stage]=create_material_program(p.gl,stage==1,true);
        const auto location=[&](const char* name,bool mandatory=true){const auto v=p.gl.GetUniformLocation(program,name);require(v>=0||!mandatory,"Missing source-format4 uniform");return v;};
        native->uniforms[stage]={-1,location("view"),location("projection"),location("f3CameraWorld"),location("TeamColorS"),location("g_FogConsts"),location("f3FogColor"),location("fWaterHeight",stage==1),location("AlphaValuesS",stage==1),location("boneRows[0]")};
    }
    p.context->check("Retain verified whole native-format4 streams on SAME Warrior material backend");p.uploads+=3;p.code->native=std::move(native);
}
bool GlBnwWarriorRetainedBackend::native_format4_diagnostic_retained()const {impl_->thread();return impl_->code&&bool(impl_->code->native);}
void GlBnwWarriorRetainedBackend::update_code_visibility(std::span<const std::uint8_t,12> states,
    float clock,std::int32_t unit,std::uint64_t revision) {
    auto& p=*impl_;p.thread();require(bool(p.code),"Missing retained Warrior material owner");
    p.code->packets.visibility(states,clock,unit,revision);
}
GlBnwWarriorDrawResult GlBnwWarriorRetainedBackend::draw_code_diagnostic(
    const std::array<terrain::Matrix4,12>& world,const GlBnwWarriorFrame& frame,const GlBnwWarriorMaterialView& source) {
    GlBnwWarriorDrawResult blocked;blocked.sdk_sample_index=impl_->sample;
    try{return impl_->draw_code(world,frame,source);}catch(const std::exception& e){blocked.blocker=e.what();return blocked;}
}
GlBnwWarriorMaterialState GlBnwWarriorRetainedBackend::code_material_state()const {
    auto& p=*impl_;p.thread();GlBnwWarriorMaterialState result;
    if(!p.code)return result;auto& c=*p.code;result.retained=true;
    result.visible_members=c.packets.draw_order().size();result.visibility_flushes=c.packets.flushes();
    for(unsigned i=0;i<12;++i)result.packet_family[i]=c.packets.family(i,c.unit,c.revision);
    result.source_ps_sdk_four_diagnostic=!c.native;return result;
}
GlBnwWarriorDrawResult GlBnwWarriorRetainedBackend::draw_diagnostic(
    const std::array<terrain::Matrix4,12> &world,const GlBnwWarriorFrame &frame) {
    auto result=impl_->draw(world,frame);result.blocker="diagnostic placement has no native source acceptance";return result;
}
GlBnwWarriorDrawResult GlBnwWarriorRetainedBackend::draw_selected(const BnwRendererSceneReceipt &scene,
    const runtime::LiveSessionState &live,const BnwWarriorPlacementReceipt *placement,const GlBnwWarriorFrame &frame) {
    impl_->thread();GlBnwWarriorDrawResult blocked;blocked.sdk_sample_index=impl_->sample;
    try {
        if(!placement) {blocked.blocker="native formation, height and heading placement receipt is unavailable";return blocked;}
        if(!selected_gandhi_warrior_from_live_scene(scene,live,0)) {blocked.blocker="selected Warrior is absent or invisible";return blocked;}
        require(scene.scene_file_sha256==placement->save_sha256(),"Warrior placement is stale for this saved scene");
        require(live.world.all_players.has_value(),"Warrior decoded live unit unavailable");
        const saves::SavedUnitObservation *unit=nullptr;
        for(const auto &player:live.world.all_players->players) for(const auto &u:player.units.units)
            if(unit_core(u,"m_eOwner")==0 && unit_core(u,"m_iID")==16385) {require(!unit,"Warrior identity duplicated");unit=&u;}
        require(unit && unit_core(*unit,"m_iX")==32 && unit_core(*unit,"m_iY")==14 &&
            unit_core(*unit,"m_eUnitType")==83 && unit_core(*unit,"m_eFacingDirection")==placement->facing_direction() &&
            unit_core(*unit,"m_iDamage")==0 && unit_core(*unit,"m_bEmbarked")==0 && unit_core(*unit,"m_bDeathDelay")==0,
            "Warrior native placement is stale for live unit state");
        auto result=impl_->draw(placement->member_world(),frame);result.model_drawable=true;return result;
    } catch(const std::exception &e) {blocked.blocker=e.what();return blocked;}
}
} // namespace civ5::graphics
