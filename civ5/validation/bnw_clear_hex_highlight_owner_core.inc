// Included by the actual UiScreenGlRenderer TU after WorldViewGridProvider.
// No independently constructible renderer/provider or caller-supplied grant.
#include <algorithm>
#include <chrono>
#include "../src/graphics/detail/bnw_clear_hex_model_backend.hpp"
#include <bit>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <map>
#include <limits>
#include <mutex>
#include <vector>
namespace civ5::graphics {
struct BnwClearHexHighlightOwner::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<ui::BnwClientSceneFrame> scene;
    const std::weak_ptr<const assets::CivisResourceBankOwner> bank;
    const std::weak_ptr<ui::BnwWorldViewRendererRunLoan::RendererLifetime> renderer_life;
    const ui::UiScreenGlRenderer*const renderer;
    const ui::CivisInputWindowOrigin window;
    const std::uint64_t map_width,map_height;
    std::weak_ptr<BnwClearHexHighlightOwner> self;
    //698BE0/4BD1D0: case-insensitive CRT lookup, stable allocated CString,
    //one synchronized actual interner shared by both native subscribers.
    struct Compare final {
        bool operator()(const std::string&a,const std::string&b)const{return _stricmp(a.c_str(),b.c_str())<0;}
    };
    std::mutex strings_mutex;
    std::map<std::string,std::unique_ptr<char[]>,Compare> strings;
    const char* intern(const char* value){
        if(!value)return nullptr;
        std::lock_guard<std::mutex> lock(strings_mutex);
        if(const auto i=strings.find(value);i!=strings.end())return i->second.get();
        const auto n=std::strlen(value);auto storage=std::make_unique<char[]>(n+1);
        std::memcpy(storage.get(),value,n+1);const auto*const identity=storage.get();
        strings.emplace(value,std::move(storage));return identity;
    }
    enum class StyleKind {hex_contour,spline_border,hex_model,static_texture,filled_hex};
    struct StyleDeclaration final {
        const char* name=nullptr;StyleKind kind;
        std::optional<float> width;
        std::optional<std::array<float,4>> colour;
        std::optional<std::string> texture,model;
    };
    //60C880/5DFAF0 are actually executed here: dimensions0, wrap=true,
    //empty owned cell vectors. Clear does not infer emptiness from a receipt.
    template<class Cell>struct Grid final {
        std::int32_t columns=0,rows=0;bool wrap=true;
        std::array<float,16> local{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        std::array<float,16> inverse=local;
        std::vector<Cell> cells;
        void require_extent()const{
            if(columns<0||rows<0||std::uint64_t(columns)*std::uint64_t(rows)!=cells.size())
                throw std::runtime_error("Highlight native grid dimensions/cell allocation differ");
        }
    };
    struct ContourCell final {std::uint32_t value=0;std::uint8_t active=0;};
    struct Contour final {StyleDeclaration declaration;Grid<ContourCell> grid;bool dirty=false;};
    struct Spline final {StyleDeclaration declaration;Grid<std::uint8_t> grid;bool dirty=false;};
    struct Model final {
        StyleDeclaration declaration;Grid<std::uint8_t> grid;bool dirty=false;
        //Only the active bytes whose constructor stores9672E0 proves. These
        //are NOT eight fabricated model/animation/backend instances.
        std::array<std::uint8_t,8> instance_active{};
        std::shared_ptr<detail::BnwClearHexModelBackend> backend;
        const detail::BnwClearHexModelConstructionScope* active_constructor=nullptr;
        std::uint64_t constructor_generation=0;
        bool constructor_entered=false; //creator-only denial/exclusion, never a readiness grant
    };
    struct Item final {std::uint8_t active=0;};
    struct Static final {StyleDeclaration declaration;std::vector<Item> items;};
    struct Filled final {StyleDeclaration declaration;std::vector<Item> items;};
    std::vector<Contour> contours;std::vector<Spline> splines;
    std::vector<Model> models;std::vector<Static> statics;std::vector<Filled> filled;
    //4D3A80 actual map dimensions before889000 registration;603D30 really
    //scans width*width, not width*height. OWN capacity grows to cover both;
    //this deliberately does not reproduce Original's fixed0x8000 overflow.
    std::vector<const char*> map_style_tags;
    using Node=detail::BnwOwnedGlRetirementNode;
    using Resources=Node::ClearHighlightChannel;
    using Images=Node::ResourceImageChannel;
    struct Gpu final {
        std::shared_ptr<const detail::BnwRendererContextPin> pin;
        std::array<Resources::Owner,3> common;
        std::vector<Resources::Owner> contours;
        std::vector<Images::Owner> images;
        std::vector<assets::CivisLoadedResource> retained_sources;
    };
    std::shared_ptr<Gpu> gpu;
    std::deque<Queued> queue;
    std::uint64_t issued=0,consumed=0,cutoff=0;
    bool draining=false,native_applied=false;
    State(std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> s,
          std::weak_ptr<ui::BnwClientSceneFrame> c,std::weak_ptr<const assets::CivisResourceBankOwner> b,
          std::weak_ptr<ui::BnwWorldViewRendererRunLoan::RendererLifetime> life,const ui::UiScreenGlRenderer* actual,
          ui::CivisInputWindowOrigin w,std::uint64_t columns,std::uint64_t rows)
      :source(std::move(s)),scene(std::move(c)),bank(std::move(b)),renderer_life(std::move(life)),renderer(actual),
       window(std::move(w)),map_width(columns),map_height(rows){
        if(!columns||!rows||columns>INT32_MAX||rows>INT32_MAX||columns>SIZE_MAX/columns||columns>SIZE_MAX/rows)
            throw std::runtime_error("Highlight actual map dimensions exceed owned grid extent");
        map_style_tags.assign(std::max(columns*columns,columns*rows),nullptr);
    }
    void first()const{
        if(creator!=std::this_thread::get_id()||!alive.load()||failed.load())
            throw std::runtime_error("Highlight actual storage retired or foreign creator");
        const auto life=renderer_life.lock();
        if(!life||life->creator!=creator||!life->alive||life->failed||life->renderer!=renderer)
            throw std::runtime_error("Highlight actual renderer creator/lifetime retired");
    }
    void clear_graphical(const BnwClearHexHighlightOwner& actual,const runtime::BnwWorldViewSourceOperationLoan& op,const Payload& payload){
        first();const auto*const tag=intern(payload.style.data());first();
        actual.require_current_without_callbacks_owned(op);first();
        //6E0040 preserves the five separate family branches. Dirty is reset
        //inside actual cell iteration only; a zero-dimension grid keeps it.
        for(auto& style:contours)if(style.declaration.name==tag){style.grid.require_extent();
            for(auto& cell:style.grid.cells){cell.value=0;cell.active=0;style.dirty=false;}}
        for(auto& style:splines)if(style.declaration.name==tag){style.grid.require_extent();
            for(auto& cell:style.grid.cells){cell=0;style.dirty=false;}}
        for(auto& style:models)if(style.declaration.name==tag){
            style.grid.require_extent();
            for(auto& cell:style.grid.cells){cell=0;style.dirty=false;}
            if(!style.backend)throw std::runtime_error("HexModel clear lacks its actual constructed model/instances");
            style.backend->clear_instances_owned(actual,op);first();
            for(auto& active:style.instance_active)active=0;
        }
        for(auto& style:statics)if(style.declaration.name==tag)for(auto& item:style.items)item.active=0;
        for(auto& style:filled)if(style.declaration.name==tag)for(auto& item:style.items)item.active=0;
    }
    void clear_map(const BnwClearHexHighlightOwner& actual,const runtime::BnwWorldViewSourceOperationLoan& op,const Payload& payload){
        first();const auto*const tag=intern(payload.style.data());first();
        actual.require_current_without_callbacks_owned(op);first();
        const auto count=map_width*map_width;
        if(count>map_style_tags.size())throw std::runtime_error("Highlight map receiver dimensions/storage changed");
        for(std::uint64_t i=0;i<count;++i)if(map_style_tags[static_cast<std::size_t>(i)]==tag)
            map_style_tags[static_cast<std::size_t>(i)]=nullptr;
    }
};
BnwClearHexHighlightOwner::BnwClearHexHighlightOwner(std::shared_ptr<State> s):state_(std::move(s)){
    if(!state_)throw std::runtime_error("Highlight actual constructor storage missing");state_->first();
}
}
#include "../src/graphics/detail/bnw_clear_hex_model_backend_core.inc"
#include "../src/graphics/detail/bnw_clear_hex_model_sdk_seal.inc"

namespace civ5::ui {
#include "../src/graphics/detail/bnw_clear_hex_model_initializer.inc"

void UiScreenGlRenderer::prepare_clear_hex_highlights_owned(
 const BnwWorldViewRendererProjectionLoan& projection,
 const runtime::BnwWorldViewSourceOperationLoan& op,
 const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window)const {
    using Owner=graphics::BnwClearHexHighlightOwner;using State=Owner::State;
    using Pin=graphics::detail::BnwRendererContextPin;
    using Resources=State::Resources;using Images=State::Images;
    const auto life=worldview_lifetime_;const auto*const renderer=this;
    const auto precursor=[&]{
        if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
           !life->active||life->active_scope||life->renderer!=renderer||projection.renderer_!=renderer)
            throw std::runtime_error("Highlight PRE lost genuine exclusive renderer occurrence");
        projection.require_current_without_callbacks();
        op.require_origin_without_callbacks();op.require_grid_scene_origin_without_callbacks(projection.scene_);
        //This precursor reads no Context Tree, notification geometry, Label
        //state or completed Context native load. Existing Grid guards stay strict.
    };
    precursor();const auto preparation=clear_highlight_preparation_;
    unsigned idle=0;
    if(!preparation||!preparation->compare_exchange_strong(idle,1)){
        if(preparation)preparation->store(2);if(life)life->failed=true;
        throw std::runtime_error("Highlight PRE reentered or previously failed");
    }
    //This C++ caller is BEFORE protected Lua entry. Its independent storage
    //survives aborts without an automatic owner inside any Lua CFunction.
    struct EndPreparation final {
        const std::shared_ptr<std::atomic<unsigned>>& gate;
        const std::shared_ptr<BnwWorldViewRendererRunLoan::RendererLifetime>& life;
        bool returned=false;
        ~EndPreparation(){if(life->creator!=std::this_thread::get_id())std::terminate();
            if(!returned){gate->store(2);life->failed=true;}}
    } end_preparation{preparation,life};
    const auto first=[&]{precursor();
        if(preparation->load()!=1)throw std::runtime_error("Highlight PRE exclusion was poisoned by reentry");};
    const auto finish_preparation=[&]{first();unsigned entered=1;
        if(!preparation->compare_exchange_strong(entered,0)){
            preparation->store(2);life->failed=true;throw std::runtime_error("Highlight PRE exclusion did not return");}
        end_preparation.returned=true;};
    first();std::shared_ptr<Owner> prior;
    {const auto scene=projection.scene_.lock();first();
     if(!scene)throw std::runtime_error("Highlight PRE actual Scene retired");prior=scene->clear_hex_highlights_;}
    if(prior){try{prior->require_current_owned(op);first();prior.reset();first();finish_preparation();return;}
        catch(...){preparation->store(2);life->failed=true;throw;}}
    const auto full=[&]{first();op.require_current();first();gui.require_current();first();window.require_current();first();};
    std::shared_ptr<State> storage;std::shared_ptr<Owner> result;
    std::shared_ptr<State::Gpu> gpu;
    try{
        full();
        {
            const auto bank=op.selected_provider_startup_bank_owned();full();
            const auto source=op.clear_hex_highlight_source_origin_without_callbacks_owned();first();
            const auto origin=op.input_policy_window_origin_owned();full();
            std::uint64_t columns=0,rows=0;
            {const auto scene=projection.scene_.lock();first();
             if(!scene||!scene->selected_host_)throw std::runtime_error("Highlight initializer requires actual selected Scene");
             scene->verify_owned();first();columns=scene->receipt_.width;rows=scene->receipt_.height;}
            first();storage=std::make_shared<State>(source,projection.scene_,bank,life,renderer,origin,columns,rows);full();
            result=std::shared_ptr<Owner>(new Owner(storage));full();storage->self=result;
            gpu=std::make_shared<State::Gpu>();full();
            //Retain only the Window-issued existing actual context pin. No
            //Context creation, window switch, readiness flag or outside grant.
            full();gpu->pin=window.retain_context_pin_owned();first();
            window.require_context_control_owned(gpu->pin->canonical_);first();
            full();
            if(&gpu->pin->context_reference_without_callbacks()!=&context_)
                throw std::runtime_error("Highlight actual UiRenderer/context pin differs");
            const auto physical=[&]{first();storage->first();
                const auto*const api=Pin::NormalAccess::api_current_without_switch_owned(gpu->pin);first();storage->first();return api;};
            const graphics::GlApi api=*physical();const graphics::GlRenderApi render=gl_;
            const auto health=[&]{const auto*const current=physical();
                if(current->GetError!=api.GetError||current->GetGraphicsResetStatus!=api.GetGraphicsResetStatus)
                    throw std::runtime_error("Highlight loaded physical dispatch changed");
                const auto reset=api.GetGraphicsResetStatus();physical();
                const auto error=api.GetError();physical();
                if(reset!=GL_NO_ERROR||error!=GL_NO_ERROR)throw std::runtime_error("Highlight actual GL reset/error");};
            const auto effect=[&](auto&& call){full();storage->first();health();physical();call();physical();health();full();storage->first();};
            for(auto& node:gpu->common){full();node=Resources::make_owned(gpu->pin,api,render);full();}
            //715320: real common index/vertex resources. Forty-byte vertex
            //layout preserves all proved positions/colour/UV stores; unused
            //normal/padding fields are OWN initialized, not Original stack bytes.
            struct Vertex final {std::array<float,3> p{},normal{};std::uint32_t colour=0xffffffff;float unused=0;std::array<float,2> uv{};};
            static_assert(sizeof(Vertex)==40);
            const auto bits=[](std::uint32_t b){return std::bit_cast<float>(b);};
            const float a=bits(0x3f2aaaab),b=bits(0xbeaaaaab),c=bits(0x3eaaaaab),d=bits(0xbf2aaaab),one=bits(0x3f800000);
            const std::array<std::array<float,3>,6> hex{{{a,b,b},{c,c,d},{b,a,b},{d,c,c},{b,b,a},{c,d,c}}};
            std::array<std::uint16_t,1536> quad_indices{};
            for(std::uint16_t v=0;v<0x400;v+=4){const auto i=std::size_t(v/4)*6;
                quad_indices[i]=v;quad_indices[i+1]=v+3;quad_indices[i+2]=v+1;
                quad_indices[i+3]=v;quad_indices[i+4]=v+3;quad_indices[i+5]=v+2;}
            auto& batch=Resources::storage_without_callbacks(*gpu->common[0]);
            effect([&]{render.CreateBuffers(1,&batch.indices);});
            effect([&]{render.NamedBufferData(batch.indices,sizeof(quad_indices),quad_indices.data(),GL_STATIC_DRAW);});
            const std::array<std::uint16_t,6> rectangle_indices{0,3,2,0,1,3};
            std::array<Vertex,4> rectangle{};
            rectangle[0].p={bits(0xbfd55555),bits(0x3faaaaab),bits(0xbe2aaaab)};
            rectangle[1].p={c,bits(0x3faaaaab),bits(0xbfd55555)};
            rectangle[2].p={b,bits(0xbfaaaaab),bits(0x3fd55555)};
            rectangle[3].p={bits(0x3fd55555),bits(0xbfaaaaab),bits(0x3e2aaaab)};
            rectangle[0].uv={0,0};rectangle[1].uv={one,0};rectangle[2].uv={0,one};rectangle[3].uv={one,one};
            std::array<Vertex,6> hex_vertices{};for(std::size_t i=0;i<6;++i)hex_vertices[i].p=hex[i];
            const std::array<std::uint16_t,12> hex_indices{1,3,2,0,3,1,0,4,3,0,5,4};
            const auto mesh=[&](auto& node,const auto& vertices,const auto& indices){
                auto& names=Resources::storage_without_callbacks(*node);
                effect([&]{render.CreateBuffers(1,&names.vertices);});effect([&]{render.CreateBuffers(1,&names.indices);});
                effect([&]{render.NamedBufferData(names.vertices,sizeof(vertices),vertices.data(),GL_STATIC_DRAW);});
                effect([&]{render.NamedBufferData(names.indices,sizeof(indices),indices.data(),GL_STATIC_DRAW);});
                effect([&]{render.CreateVertexArrays(1,&names.vao);});
                effect([&]{render.VertexArrayVertexBuffer(names.vao,0,names.vertices,0,sizeof(Vertex));render.VertexArrayElementBuffer(names.vao,names.indices);});
                for(GLuint slot=0;slot<3;++slot)effect([&]{render.EnableVertexArrayAttrib(names.vao,slot);render.VertexArrayAttribBinding(names.vao,slot,0);});
                effect([&]{render.VertexArrayAttribFormat(names.vao,0,3,GL_FLOAT,GL_FALSE,0);});
                effect([&]{render.VertexArrayAttribFormat(names.vao,1,4,GL_UNSIGNED_BYTE,GL_TRUE,24);});
                effect([&]{render.VertexArrayAttribFormat(names.vao,2,2,GL_FLOAT,GL_FALSE,32);});
            };
            mesh(gpu->common[1],rectangle,rectangle_indices);mesh(gpu->common[2],hex_vertices,hex_indices);
            //Actual OWN GL DrawSimpleOverlay counterpart. Real compilation/
            //link/uniforms; no Original shader bytecode or pixel-equivalence claim.
            constexpr const char* vertex=R"GLSL(#version 460 core
layout(location=0) in vec3 position;layout(location=1) in vec4 colour;layout(location=2) in vec2 uv;
uniform mat4 worldViewProjection;out vec4 tint;out vec2 texcoord;
void main(){gl_Position=worldViewProjection*vec4(position,1);tint=colour;texcoord=uv;}
)GLSL";
            constexpr const char* fragment=R"GLSL(#version 460 core
layout(binding=0) uniform sampler2D authoredTexture;in vec4 tint;in vec2 texcoord;layout(location=0) out vec4 colour;
void main(){colour=tint*texture(authoredTexture,texcoord);}
)GLSL";
            effect([&]{batch.program=render.CreateProgram();});
            const std::array<const char*,2> shaders{vertex,fragment};
            for(std::size_t stage=0;stage<2;++stage){
                effect([&]{batch.stages[stage]=render.CreateShader(stage?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);});
                effect([&]{const auto*text=shaders[stage];render.ShaderSource(batch.stages[stage],1,&text,nullptr);render.CompileShader(batch.stages[stage]);});
                GLint okay=0;effect([&]{render.GetShaderiv(batch.stages[stage],GL_COMPILE_STATUS,&okay);});
                if(!okay)throw std::runtime_error("OWN DrawSimpleOverlay shader compilation failed");
                effect([&]{render.AttachShader(batch.program,batch.stages[stage]);});
            }
            effect([&]{render.LinkProgram(batch.program);});GLint linked=0;
            effect([&]{render.GetProgramiv(batch.program,GL_LINK_STATUS,&linked);});
            if(!linked)throw std::runtime_error("OWN DrawSimpleOverlay program link failed");
            full();storage->gpu=gpu;full(); //actual partial resource custody, before model constructors; not Scene publication
            //Stage only through the SAME canonical selected resource bank,
            //outside any Lua CFunction/protected script call. No loose-file read.
            const auto stage=[&](const std::string& name,assets::CivisResourceParse parse){
                full();const auto canonical=bank->retain_owner();full();
                if(canonical.get()!=bank.get()||canonical.owner_before(bank)||bank.owner_before(canonical))
                    throw std::runtime_error("Highlight actual canonical bank differs");
                canonical->load(assets::AssetKey(name),parse);full();
                const auto resource=bank->capability().loaded(assets::AssetKey(name));full();
                if(!resource.bytes)throw std::runtime_error("Highlight actual loaded payload owner absent");
                gpu->retained_sources.push_back(resource);full();return resource;
            };
            const auto xml=stage("Highlights.xml",assets::CivisResourceParse::xml);
            if(!xml.xml||xml.xml->name!="Highlights")throw std::runtime_error("Highlight actual selected XML root absent");
            const auto attr=[](const content::XmlNode& node,const char* key)->std::optional<std::string>{
                for(const auto&[k,v]:node.attributes)if(k==key)return v;return std::nullopt;};
            const auto number=[](const std::string& text){
                char* end=nullptr;errno=0;const float v=std::strtof(text.c_str(),&end);
                while(end&&std::isspace(static_cast<unsigned char>(*end)))++end;
                if(errno==ERANGE||!end||end==text.c_str()||*end||!std::isfinite(v))
                    throw std::runtime_error("Highlight authored finite numeric attribute unsupported");return v;};
            const auto upload=[&](const assets::CivisLoadedResource& resource){
                full();const auto dds=assets::DdsData::parse(*resource.bytes,assets::NumericDdsProfile::legacy_d3d9,assets::DdsHeaderProfile::civ5_d3dx_size24);full();
                if(dds.face_count()!=1)throw std::runtime_error("Highlight style requires actual authored2D DDS");
                const auto plan=graphics::dds_upload_plan(dds,graphics::DdsSamplingColor::linear);full();
                GLint maximum=0;effect([&]{api.GetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);});
                if(maximum<=0||dds.width()>static_cast<std::uint32_t>(maximum)||dds.height()>static_cast<std::uint32_t>(maximum))
                    throw std::runtime_error("Highlight actual DDS exceeds current texture extent");
                for(const auto& level:dds.levels())if(level.length>static_cast<std::uint64_t>(std::numeric_limits<GLsizei>::max()))
                    throw std::runtime_error("Highlight actual DDS mip exceeds GL transfer size");
                auto image=Images::CreationAccess::make_image_owned(gpu->pin,api);full();
                auto unpack=Images::CreationAccess::make_unpack_owned(gpu->pin,api);full();
                auto& name=Images::CreationAccess::image_name_storage_without_callbacks(*image);
                auto& transfer=Images::CreationAccess::unpack_storage_without_callbacks(*unpack);
                constexpr std::array<GLenum,12> fields{GL_UNPACK_ALIGNMENT,GL_UNPACK_ROW_LENGTH,GL_UNPACK_IMAGE_HEIGHT,
                  GL_UNPACK_SKIP_PIXELS,GL_UNPACK_SKIP_ROWS,GL_UNPACK_SKIP_IMAGES,GL_UNPACK_SWAP_BYTES,GL_UNPACK_LSB_FIRST,
                  GL_UNPACK_COMPRESSED_BLOCK_WIDTH,GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,GL_UNPACK_COMPRESSED_BLOCK_DEPTH,GL_UNPACK_COMPRESSED_BLOCK_SIZE};
                try{
                    full();health();
                    for(std::size_t i=0;i<fields.size();++i){physical();api.GetIntegerv(fields[i],&transfer.saved[i]);physical();health();transfer.captured=i+1;}
                    physical();api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&transfer.buffer);physical();health();transfer.buffer_captured=true;
                    effect([&]{transfer.buffer_changed=true;api.BindBuffer(GL_PIXEL_UNPACK_BUFFER,0);});
                    for(std::size_t i=0;i<fields.size();++i)effect([&]{transfer.changed=i+1;api.PixelStorei(fields[i],i?0:1);});
                    effect([&]{api.CreateTextures(GL_TEXTURE_2D,1,&name);});
                    effect([&]{api.TextureStorage2D(name,dds.mip_count(),plan.internal_format,dds.width(),dds.height());});
                    for(const auto& level:dds.levels()){
                        full();health();
                        for(std::size_t i=0;i<fields.size();++i){GLint v=0;physical();api.GetIntegerv(fields[i],&v);physical();
                            if(v!=(i?0:1))throw std::runtime_error("Highlight canonical unpack changed before actual DDS consumption");}
                        GLint pbo=0;physical();api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&pbo);physical();
                        if(pbo)throw std::runtime_error("Highlight DDS PBO rebound");
                        const auto bytes=dds.payload(0,level.mip);physical();
                        if(plan.compressed)api.CompressedTextureSubImage2D(name,level.mip,0,0,level.width,level.height,plan.internal_format,static_cast<GLsizei>(bytes.size()),bytes.data());
                        else api.TextureSubImage2D(name,level.mip,0,0,level.width,level.height,plan.transfer_format,plan.transfer_type,bytes.data());
                        physical();health();full();
                    }
                    effect([&]{api.TextureParameteri(name,GL_TEXTURE_BASE_LEVEL,0);api.TextureParameteri(name,GL_TEXTURE_MAX_LEVEL,dds.mip_count()-1);api.TextureParameteriv(name,GL_TEXTURE_SWIZZLE_RGBA,plan.swizzle.data());});
                    const auto restored=Images::CreationAccess::restore_unpack_without_switch(*unpack);
                    if(!restored.complete||restored.cleanup_failure)throw std::runtime_error("Highlight DDS unpack restoration incomplete");
                    full();gpu->images.push_back(std::move(image));full();
                }catch(...){const auto error=std::current_exception();
                    (void)Images::CreationAccess::restore_unpack_without_switch(*unpack);
                    image.reset();unpack.reset();std::rethrow_exception(error);}
            };
            std::size_t declarations=0;
            for(const auto& node:xml.xml->children){
                if(node.name!="style")continue;++declarations;full();
                const auto name=attr(node,"name"),type=attr(node,"type");
                if(!name||!type)throw std::runtime_error("Highlight original required name/type attribute missing");
                State::StyleDeclaration declaration;declaration.name=storage->intern(name->c_str());full();
                if(*type=="HexContour")declaration.kind=State::StyleKind::hex_contour;
                else if(*type=="SplineBorder")declaration.kind=State::StyleKind::spline_border;
                else if(*type=="HexModel")declaration.kind=State::StyleKind::hex_model;
                else if(*type=="StaticTexture")declaration.kind=State::StyleKind::static_texture;
                else if(*type=="FilledHex")declaration.kind=State::StyleKind::filled_hex;
                else throw std::runtime_error("Highlight actual authored style type is unsupported");
                if(const auto width=attr(node,"width"))declaration.width=number(*width);
                if(const auto colour=attr(node,"color")){
                    std::array<float,4> values{};std::size_t start=0;
                    for(std::size_t i=0;i<4;++i){const auto end=colour->find(',',start);
                        if((i<3)==(end==std::string::npos))throw std::runtime_error("Highlight authored colour arity differs");
                        values[i]=number(colour->substr(start,end==std::string::npos?end:end-start))*bits(0x3b808081);start=end+1;}
                    declaration.colour=values;
                }
                declaration.texture=attr(node,"texture");declaration.model=attr(node,"model");
                if((declaration.texture&&declaration.texture->size()>=64)||(declaration.model&&declaration.model->size()>=64))
                    throw std::runtime_error("Highlight original64-byte texture/model operand exceeds capacity");
                switch(declaration.kind){
                case State::StyleKind::hex_contour:{
                    if(!declaration.width||!declaration.texture)throw std::runtime_error("HexContour actual width/texture provider missing");
                    storage->contours.push_back({declaration,{ },false});full();
                    auto geometry=Resources::make_owned(gpu->pin,api,render);full();
                    std::array<Vertex,12> vertices{};
                    for(std::size_t i=0;i<6;++i){vertices[i].p=hex[i];vertices[i].uv={one,0};
                        for(std::size_t j=0;j<3;++j)vertices[i+6].p[j]=hex[i][j]*(one-*declaration.width);vertices[i+6].uv={0,0};}
                    const std::array<std::uint32_t,36> indices{0,1,7,0,7,6,1,2,8,1,8,7,2,3,9,2,9,8,3,4,10,3,10,9,4,5,11,4,11,10,5,6,0,5,6,11};
                    mesh(geometry,vertices,indices);full();gpu->contours.push_back(std::move(geometry));full();
                    upload(stage(*declaration.texture,assets::CivisResourceParse::bytes));full();break;
                }
                case State::StyleKind::spline_border:
                    if(!declaration.width||!declaration.texture||!declaration.colour)throw std::runtime_error("SplineBorder actual declaration fields absent");
                    storage->splines.push_back({declaration,{ },false});full();break;
                case State::StyleKind::hex_model:{
                    if(!declaration.model)throw std::runtime_error("HexModel authored model operand absent");
                    storage->models.push_back({declaration,{ },false,{ }});full();
                    const auto model=stage(*declaration.model,assets::CivisResourceParse::xml);
                    initialize_clear_hex_highlight_model_owned(op,result,storage->models.size()-1,model);full();break;
                }
                case State::StyleKind::static_texture:
                    if(!declaration.texture)throw std::runtime_error("StaticTexture actual authored texture operand absent");
                    storage->statics.push_back({declaration,{ }});full();upload(stage(*declaration.texture,assets::CivisResourceParse::bytes));full();break;
                case State::StyleKind::filled_hex:
                    if(!declaration.width||!declaration.colour)throw std::runtime_error("FilledHex actual width/colour fields absent");
                    storage->filled.push_back({declaration,{ }});full();break;
                }
            }
            if(!declarations)throw std::runtime_error("Highlight actual XML has no style entries");
            full();storage->gpu=gpu;storage->first();
        } //CPU declarations/staging borrowers retire before publication POST.
        full();
        {const auto scene=projection.scene_.lock();first();
         if(!scene||scene->clear_hex_highlights_)throw std::runtime_error("Highlight canonical Scene attachment replay");
         scene->clear_hex_highlights_=result;first();}
        result->require_current_owned(op);full();finish_preparation();
    }catch(...){
        const auto primary=std::current_exception();if(storage)storage->failed.store(true);
        //Partial storage/resources never issue a completed constructor/receiver.
        //Closed retirement nodes retain actual GPU IDs even on lost currentness.
        {const auto scene=projection.scene_.lock();
         if(scene&&scene->gui_thread_==std::this_thread::get_id()&&scene->clear_hex_highlights_.get()==result.get()&&
            !scene->clear_hex_highlights_.owner_before(result)&&!result.owner_before(scene->clear_hex_highlights_))
             scene->clear_hex_highlights_.reset();}
        result.reset();storage.reset();gpu.reset();
        try{full();}catch(...){if(life)life->failed=true;}
        preparation->store(2);if(life)life->failed=true;std::rethrow_exception(primary);
    }
}
void UiScreenGlRenderer::WorldViewGridProvider::prepare_clear_hex_highlights_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window)const{
    //Precursor is closed to NativeIssuer and independent of future trees.
    if(life_->creator!=std::this_thread::get_id()||!life_->alive||life_->failed||!life_->active||life_->active_scope)
        throw std::runtime_error("Highlight Grid PRE requires genuine exclusive renderer lifetime");
    projection_.require_current_without_callbacks();require_scene_for_source(op,projection_.scene_);
    projection_.renderer_->prepare_clear_hex_highlights_owned(projection_,op,gui,window);
    projection_.require_current_without_callbacks();require_scene_for_source(op,projection_.scene_);
}
std::shared_ptr<graphics::BnwClearHexHighlightOwner>
UiScreenGlRenderer::WorldViewGridProvider::clear_hex_highlight_owner_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_for_source(op);
    std::shared_ptr<graphics::BnwClearHexHighlightOwner> result;
    {const auto scene=projection_.scene_.lock();require_for_source(op);
     if(!scene)throw std::runtime_error("Highlight actual Scene retired");result=scene->clear_hex_highlights_;}
    if(!result)throw std::runtime_error("Highlight actual constructor/model/renderer initialization did not complete");
    result->require_current_owned(op);require_for_source(op);return result;
}
}
namespace civ5::graphics {
BnwClearHexHighlightOwner::~BnwClearHexHighlightOwner(){
    if(creator_!=std::this_thread::get_id())std::terminate();state_->alive.store(false);
}
void BnwClearHexHighlightOwner::first_owned()const{
    if(creator_!=std::this_thread::get_id())throw std::runtime_error("Highlight facade foreign creator");
    state_->first();
}
void BnwClearHexHighlightOwner::require_current_without_callbacks_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    first_owned();const auto s=state_;s->first();op.require_origin_without_callbacks();s->first();
    const auto source=op.clear_hex_highlight_source_origin_without_callbacks_owned();s->first();
    const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
    if(source.expired()||!same(source,s->source)||s->scene.expired()||s->bank.expired()||s->window.window.expired())
        throw std::runtime_error("Highlight SAME Source/Scene/Bank/Window retired or replaced");
    op.require_grid_scene_origin_without_callbacks(s->scene);s->first();
    const auto scene=s->scene.lock();
    if(!scene||scene->clear_hex_highlights_.get()!=this||!same(scene->clear_hex_highlights_,s->self))
        throw std::runtime_error("Highlight canonical Scene attachment replaced");
    scene->verify_owned();s->first();op.require_origin_without_callbacks();s->first();
}
void BnwClearHexHighlightOwner::require_current_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    first_owned();const auto s=state_;
    try{
        require_current_without_callbacks_owned(op);op.require_current();s->first();
        {
            const auto bank=op.selected_provider_startup_bank_owned();s->first();
            const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
            if(!bank||!same(bank,s->bank))throw std::runtime_error("Highlight actual startup Bank changed");
            bank->require_boundary();s->first();require_current_without_callbacks_owned(op);
        }
        const auto origin=op.input_policy_window_origin_owned();s->first();
        if(origin.hwnd!=s->window.hwnd||origin.occurrence!=s->window.occurrence||origin.process!=s->window.process||
           origin.ui_thread!=s->window.ui_thread||origin.window.owner_before(s->window.window)||s->window.window.owner_before(origin.window))
            throw std::runtime_error("Highlight actual renderer Window occurrence changed");
        //Viewport revisions may advance normally; current Source checks the
        //fresh viewport while the persistent receiver retains HWND occurrence.
        op.require_input_policy_window_owned(origin);s->first();require_current_without_callbacks_owned(op);
    }catch(...){s->failed.store(true);throw;}
}
void BnwClearHexHighlightOwner::enqueue_owned(const runtime::BnwWorldViewSourceOperationLoan& op,const Payload& p){
    first_owned();const auto s=state_;require_current_owned(op);s->first();
    if(p.style.back()!=0||s->issued==UINT64_MAX)throw std::runtime_error("Highlight actual CString/queue occurrence invalid");
    const auto serial=op.operation_serial();s->first();
    try{s->queue.push_back({s->issued+1,serial,0x100,0x74,p});require_current_without_callbacks_owned(op);++s->issued;
        require_current_owned(op);s->first();}
    catch(...){s->failed.store(true);throw;}
}
std::uint64_t BnwClearHexHighlightOwner::begin_drain_owned(const runtime::BnwWorldViewSourceOperationLoan& op){
    first_owned();const auto s=state_;require_current_without_callbacks_owned(op);
    if(s->draining){s->failed.store(true);throw std::runtime_error("Highlight deferred dispatcher reentered");}
    s->draining=true; //Creator FIRST and exclusion BEFORE any Source FULL.
    try{require_current_owned(op);s->cutoff=s->issued;s->native_applied=false;return s->cutoff;}
    catch(...){s->failed.store(true);throw;}
}
std::optional<BnwClearHexHighlightOwner::Queued> BnwClearHexHighlightOwner::peek_drain_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t cutoff)const{
    first_owned();const auto s=state_;require_current_owned(op);
    if(!s->draining||cutoff!=s->cutoff)throw std::runtime_error("Highlight genuine drain cutoff replaced");
    if(s->queue.empty()||s->queue.front().occurrence>cutoff)return std::nullopt;
    const auto& row=s->queue.front();
    if(row.record_stride!=0x100||row.record_kind!=0x74||!row.source_serial||row.occurrence!=s->consumed+1||row.payload.style.back()!=0)
        throw std::runtime_error("Highlight real deferred FIFO/header/CString changed");
    return row;
}
void BnwClearHexHighlightOwner::dispatch_native_owned(const runtime::BnwWorldViewSourceOperationLoan& op,const Queued& row){
    first_owned();const auto s=state_;const auto pending=peek_drain_owned(op,s->cutoff);
    if(!pending||pending->occurrence!=row.occurrence||pending->payload.style!=row.payload.style||s->native_applied)
        throw std::runtime_error("Highlight native receiver occurrence replay or payload changed");
    try{require_current_owned(op);s->clear_graphical(*this,op,row.payload);require_current_owned(op);
        s->clear_map(*this,op,row.payload);require_current_owned(op);s->native_applied=true;}
    catch(...){s->failed.store(true);throw;}
}
void BnwClearHexHighlightOwner::commit_drain_owned(const runtime::BnwWorldViewSourceOperationLoan& op,const Queued& row){
    first_owned();const auto s=state_;const auto pending=peek_drain_owned(op,s->cutoff);
    if(!pending||!s->native_applied||pending->occurrence!=row.occurrence||pending->payload.style!=row.payload.style)
        throw std::runtime_error("Highlight actual subscribers have not returned for this FIFO occurrence");
    s->queue.pop_front();s->consumed=row.occurrence;s->native_applied=false;require_current_owned(op);
}
void BnwClearHexHighlightOwner::finish_drain_owned(const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t cutoff){
    first_owned();const auto s=state_;require_current_owned(op);
    try{
        if(!s->draining||s->cutoff!=cutoff||s->native_applied||s->consumed!=cutoff||!s->queue.empty())
            throw std::runtime_error("Highlight deferred work remains; no first-draw completion");
        //Exclusion remains held through the LAST callback-capable FULL.
        require_current_owned(op);require_current_without_callbacks_owned(op);s->first();
        if(!s->draining||s->cutoff!=cutoff||s->native_applied||s->consumed!=cutoff||s->issued!=cutoff||!s->queue.empty())
            throw std::runtime_error("Highlight drain tail changed during final Source POST");
        s->draining=false; //No callback/FULL follows this actual normal release.
    }catch(...){s->failed.store(true);throw;}
}
void BnwClearHexHighlightOwner::quarantine_owned()noexcept{
    if(creator_!=std::this_thread::get_id())std::terminate();state_->failed.store(true);
}
}
