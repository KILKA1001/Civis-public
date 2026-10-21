// Included once after MaterialStorage::State, inside civ5::ui::detail.
// This owns new alpha targets and does not import an Original camera/frame.
namespace {
template<class A,class B>bool alpha_same_control(const A& a,const B& b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
void alpha_write_exclusive(const std::filesystem::path& path,std::span<const std::uint8_t> bytes){
    const int fd=_wopen(path.c_str(),_O_CREAT|_O_EXCL|_O_WRONLY|_O_BINARY,_S_IREAD|_S_IWRITE);
    need(fd>=0,"Alpha output already exists or cannot be created");
    struct Close{int fd;~Close(){_close(fd);}}close{fd};
    while(!bytes.empty()){
        const auto n=static_cast<unsigned>(std::min<std::size_t>(bytes.size(),INT32_MAX));
        const auto written=_write(fd,bytes.data(),n);need(written>0,"Alpha output write failed");
        bytes=bytes.subspan(static_cast<std::size_t>(written));
    }
    need(_commit(fd)==0,"Alpha output commit failed");
}
// Lossless PNG with stored DEFLATE blocks: it serializes the actual RGBA8
// readback, with no palette, resampling, artificial pixels or extra library.
assets::Bytes alpha_png(std::uint32_t width,std::uint32_t height,std::span<const std::uint8_t> rgba){
    need(rgba.size()==std::uint64_t(width)*height*4,"Alpha PNG readback extent differs");
    assets::Bytes out{137,80,78,71,13,10,26,10};
    const auto be=[](assets::Bytes& b,std::uint32_t v){for(int n=3;n>=0;--n)b.push_back(static_cast<std::uint8_t>(v>>(8*n)));};
    const auto chunk=[&](const char* name,const assets::Bytes& data){
        need(data.size()<=UINT32_MAX,"Alpha PNG chunk overflow");be(out,static_cast<std::uint32_t>(data.size()));
        const auto begin=out.size();out.insert(out.end(),name,name+4);out.insert(out.end(),data.begin(),data.end());
        std::uint32_t crc=~0u;for(auto byte:std::span(out).subspan(begin)){crc^=byte;
            for(unsigned n=0;n<8;++n)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}be(out,~crc);
    };
    assets::Bytes header;be(header,width);be(header,height);header.insert(header.end(),{8,6,0,0,0});chunk("IHDR",header);
    assets::Bytes raw;raw.reserve(rgba.size()+height);
    for(std::uint32_t row=0;row<height;++row){raw.push_back(0);
        const auto line=rgba.subspan(std::size_t(row)*width*4,std::size_t(width)*4);raw.insert(raw.end(),line.begin(),line.end());}
    assets::Bytes z{0x78,0x01};std::uint32_t a=1,b=0;
    for(auto byte:raw){a=(a+byte)%65521;b=(b+a)%65521;}
    for(std::size_t offset=0;offset<raw.size();){const auto count=std::min<std::size_t>(65535,raw.size()-offset);
        z.push_back(offset+count==raw.size()?1:0);const auto len=static_cast<std::uint16_t>(count);
        for(auto v:{len,static_cast<std::uint16_t>(~len)}){z.push_back(static_cast<std::uint8_t>(v));z.push_back(static_cast<std::uint8_t>(v>>8));}
        z.insert(z.end(),raw.begin()+offset,raw.begin()+offset+count);offset+=count;
    }
    be(z,(b<<16)|a);chunk("IDAT",z);chunk("IEND",{});return out;
}
struct AlphaRasterScope final {
    const graphics::GlContext& context;const graphics::GlRenderApi& gl;
    GLint draw=0,read=0,program=0,vao=0,active=0,depth_func=0,pack=0,pack_buffer=0;
    GLint clip_origin=0,clip_depth=0,polygon[2]{};GLfloat viewport[4]{};GLdouble range[2]{};
    std::array<GLint,6> blend{};
    struct IndexedColour {GLboolean blend;std::array<GLboolean,4> mask;std::array<GLint,6> arithmetic;};
    std::vector<IndexedColour> indexed_colour;std::vector<GLboolean> indexed_scissor;
    std::vector<std::array<GLdouble,2>> indexed_depth_ranges;
    static constexpr std::array<GLenum,7> pack_names{GL_PACK_ROW_LENGTH,GL_PACK_IMAGE_HEIGHT,GL_PACK_SKIP_ROWS,
        GL_PACK_SKIP_PIXELS,GL_PACK_SKIP_IMAGES,GL_PACK_SWAP_BYTES,GL_PACK_LSB_FIRST};
    std::array<GLint,7> pack_values{};
    std::array<GLint,16> tex2{},tex3{},cubes{},samplers{};std::vector<std::pair<GLenum,GLboolean>> enables;
    GLboolean depth_write=GL_TRUE;std::array<GLboolean,4> color_write{};
    explicit AlphaRasterScope(const graphics::GlContext& c,const graphics::GlRenderApi& g):context(c),gl(g){
        const auto& api=context.api();
        api.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);api.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
        api.GetIntegerv(GL_CURRENT_PROGRAM,&program);api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        api.GetIntegerv(GL_ACTIVE_TEXTURE,&active);api.GetIntegerv(GL_DEPTH_FUNC,&depth_func);
        api.GetIntegerv(GL_PACK_ALIGNMENT,&pack);api.GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack_buffer);
        for(unsigned n=0;n<pack_names.size();++n)api.GetIntegerv(pack_names[n],&pack_values[n]);
        const std::array<GLenum,6> blend_names{GL_BLEND_EQUATION_RGB,GL_BLEND_EQUATION_ALPHA,GL_BLEND_SRC_RGB,
            GL_BLEND_DST_RGB,GL_BLEND_SRC_ALPHA,GL_BLEND_DST_ALPHA};
        for(unsigned n=0;n<blend_names.size();++n)api.GetIntegerv(blend_names[n],&blend[n]);
        GLint buffers=0,viewports=0;api.GetIntegerv(GL_MAX_DRAW_BUFFERS,&buffers);api.GetIntegerv(GL_MAX_VIEWPORTS,&viewports);
        indexed_colour.resize(buffers);indexed_scissor.resize(viewports);indexed_depth_ranges.resize(viewports);
        for(GLint n=0;n<viewports;++n)gl.GetDoublei_v(GL_DEPTH_RANGE,n,indexed_depth_ranges[n].data());
        for(GLint n=0;n<buffers;++n){auto& v=indexed_colour[n];v.blend=gl.IsEnabledi(GL_BLEND,n);
            gl.GetBooleani_v(GL_COLOR_WRITEMASK,n,v.mask.data());
            for(unsigned k=0;k<blend_names.size();++k)gl.GetIntegeri_v(blend_names[k],n,&v.arithmetic[k]);}
        for(GLint n=0;n<viewports;++n)indexed_scissor[n]=gl.IsEnabledi(GL_SCISSOR_TEST,n);
        api.GetIntegerv(GL_CLIP_ORIGIN,&clip_origin);api.GetIntegerv(GL_CLIP_DEPTH_MODE,&clip_depth);
        api.GetIntegerv(GL_POLYGON_MODE,polygon);gl.GetFloati_v(GL_VIEWPORT,0,viewport);gl.GetDoublei_v(GL_DEPTH_RANGE,0,range);
        gl.GetBooleanv(GL_DEPTH_WRITEMASK,&depth_write);gl.GetBooleanv(GL_COLOR_WRITEMASK,color_write.data());
        for(auto flag:{GL_DEPTH_TEST,GL_STENCIL_TEST,GL_SCISSOR_TEST,GL_BLEND,GL_CULL_FACE,GL_DITHER,
                       GL_FRAMEBUFFER_SRGB,GL_RASTERIZER_DISCARD,GL_POLYGON_OFFSET_FILL,GL_POLYGON_OFFSET_LINE})
            enables.emplace_back(flag,gl.IsEnabled(flag));
        GLint clips=0;api.GetIntegerv(GL_MAX_CLIP_DISTANCES,&clips);
        for(GLint i=0;i<clips;++i)enables.emplace_back(GL_CLIP_DISTANCE0+i,gl.IsEnabled(GL_CLIP_DISTANCE0+i));
        for(unsigned n=0;n<16;++n){gl.ActiveTexture(GL_TEXTURE0+n);api.GetIntegerv(GL_TEXTURE_BINDING_2D,&tex2[n]);
            api.GetIntegerv(GL_TEXTURE_BINDING_3D,&tex3[n]);api.GetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP,&cubes[n]);
            gl.GetIntegeri_v(GL_SAMPLER_BINDING,n,&samplers[n]);}
        gl.ActiveTexture(static_cast<GLenum>(active));
    }
    ~AlphaRasterScope(){
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER,static_cast<GLuint>(draw));gl.BindFramebuffer(GL_READ_FRAMEBUFFER,static_cast<GLuint>(read));
        gl.UseProgram(static_cast<GLuint>(program));gl.BindVertexArray(static_cast<GLuint>(vao));
        gl.ClipControl(static_cast<GLenum>(clip_origin),static_cast<GLenum>(clip_depth));
        gl.ViewportIndexedf(0,viewport[0],viewport[1],viewport[2],viewport[3]);gl.DepthRangeIndexed(0,range[0],range[1]);
        gl.DepthFunc(static_cast<GLenum>(depth_func));gl.DepthMask(depth_write);gl.ColorMask(color_write[0],color_write[1],color_write[2],color_write[3]);
        gl.BlendEquationSeparate(static_cast<GLenum>(blend[0]),static_cast<GLenum>(blend[1]));
        gl.BlendFuncSeparate(static_cast<GLenum>(blend[2]),static_cast<GLenum>(blend[3]),static_cast<GLenum>(blend[4]),static_cast<GLenum>(blend[5]));
        gl.PolygonMode(GL_FRONT_AND_BACK,static_cast<GLenum>(polygon[0]));
        context.api().PixelStorei(GL_PACK_ALIGNMENT,pack);context.api().BindBuffer(GL_PIXEL_PACK_BUFFER,static_cast<GLuint>(pack_buffer));
        for(unsigned n=0;n<pack_names.size();++n)context.api().PixelStorei(pack_names[n],pack_values[n]);
        for(unsigned n=0;n<16;++n){gl.ActiveTexture(GL_TEXTURE0+n);context.api().BindTexture(GL_TEXTURE_2D,static_cast<GLuint>(tex2[n]));
            context.api().BindTexture(GL_TEXTURE_3D,static_cast<GLuint>(tex3[n]));context.api().BindTexture(GL_TEXTURE_CUBE_MAP,static_cast<GLuint>(cubes[n]));
            gl.BindSampler(n,static_cast<GLuint>(samplers[n]));}gl.ActiveTexture(static_cast<GLenum>(active));
        for(const auto& [flag,was]:enables)if(was)gl.Enable(flag);else gl.Disable(flag);
        // Global LAND/FOW calls affect every index. Restore each recorded
        // value after the global restoration, preserving distinct inputs.
        for(unsigned n=0;n<indexed_colour.size();++n){const auto& v=indexed_colour[n];
            if(v.blend)gl.Enablei(GL_BLEND,n);else gl.Disablei(GL_BLEND,n);
            gl.ColorMaski(n,v.mask[0],v.mask[1],v.mask[2],v.mask[3]);
            gl.BlendEquationSeparatei(n,static_cast<GLenum>(v.arithmetic[0]),static_cast<GLenum>(v.arithmetic[1]));
            gl.BlendFuncSeparatei(n,static_cast<GLenum>(v.arithmetic[2]),static_cast<GLenum>(v.arithmetic[3]),
                static_cast<GLenum>(v.arithmetic[4]),static_cast<GLenum>(v.arithmetic[5]));}
        for(unsigned n=0;n<indexed_scissor.size();++n)if(indexed_scissor[n])gl.Enablei(GL_SCISSOR_TEST,n);else gl.Disablei(GL_SCISSOR_TEST,n);
        // Nested LAND/FOW global DepthRange affects every viewport.
        for(unsigned n=0;n<indexed_depth_ranges.size();++n)gl.DepthRangeIndexed(n,indexed_depth_ranges[n][0],indexed_depth_ranges[n][1]);
    }
};
}
struct CivisFrontendPartyMaterialStorage::FrameControl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};bool checking=false;
    std::weak_ptr<void> targets;std::uint64_t generation=0;const FrameInputsLoan* active=nullptr;
    const std::shared_ptr<const materials::MaterialWorld>* world=nullptr;
    const terrain::CameraFrameState* camera=nullptr;const terrain::CameraControllerState* controller=nullptr;
    const graphics::GlTexture* color=nullptr;const graphics::GlTexture* depth=nullptr;
    std::uint32_t width=0,height=0;
    void first()const{need(creator==std::this_thread::get_id(),"Alpha frame loan foreign creator");
        need(alive.load()&&!failed.load()&&!targets.expired()&&!checking,"Alpha frame retired/quarantined/reentered");}
};
struct CivisFrontendAlphaLandFrame::State final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<CivisFrontendPartyMaterialStorage::State> material;
    const std::shared_ptr<graphics::GlContext> context;
    const std::shared_ptr<CivisFrontendPartyMaterialStorage::FrameControl> control=
        std::make_shared<CivisFrontendPartyMaterialStorage::FrameControl>();
    const std::uint32_t width,height;const terrain::X87Precision precision;
    const terrain::ReciprocalEstimate reciprocal;const terrain::ReciprocalSqrtEstimate reciprocal_sqrt;
    const std::shared_ptr<const materials::MaterialWorld> world;
    terrain::CameraControllerState controller{};terrain::CameraFrameState camera{};
    std::optional<materials::FogOfWarState> fog;materials::FogOfWarSceneFrame fow_frame{};
    std::vector<materials::LandMaterialConstants> land;
    std::unique_ptr<graphics::GlMaterialWorldRenderer> renderer;
    std::unique_ptr<graphics::GlFogOfWarMap> fow;
    std::optional<graphics::GlTexture> color,depth,pick_depth;
    std::unique_ptr<CivisBasicWaterSurface> water;
    graphics::GlRenderApi gl;GLuint framebuffer=0;
    std::optional<std::uint64_t> initial_plot;
    bool attempted=false,drawn=false,busy=false;
    State(std::shared_ptr<CivisFrontendPartyMaterialStorage::State> m,std::shared_ptr<graphics::GlContext> c,
          std::uint32_t w,std::uint32_t h,terrain::X87Precision p,terrain::ReciprocalEstimate r,terrain::ReciprocalSqrtEstimate s)
        :material(std::move(m)),context(std::move(c)),width(w),height(h),precision(p),reciprocal(std::move(r)),
         reciprocal_sqrt(std::move(s)),world(material->world),gl(*context){}
    void first()const{
        need(creator==std::this_thread::get_id(),"Alpha frame belongs to another creator");
        need(control->alive.load()&&!control->failed.load(),"Alpha frame retired or quarantined");material->first();
        need(alpha_same_control(material->context,std::weak_ptr<graphics::GlContext>(context)),"Alpha frame GL control differs");
    }
    void boundary(){first();material->full();first();}
    void release()noexcept{
        try{context->api();if(framebuffer)gl.DeleteFramebuffers(1,&framebuffer);framebuffer=0;}
        catch(...){std::terminate();}
    }
    ~State(){release();} // Actual water/FOW/LAND/textures retire before material.
};
CivisFrontendPartyMaterialStorage::FrameInputsLoan::FrameInputsLoan(
    const CivisFrontendPartyMaterialStorage& material,const CivisFrontendAlphaLandFrame& frame)
    :state_([&]{need(material.creator_==std::this_thread::get_id(),"Frame material creator differs");return material.state_;}()),
     frame_control_([&]{need(frame.creator_==std::this_thread::get_id(),"Frame facade creator differs");
         const auto s=frame.state_;s->first();need(s->material==state_,"Frame belongs to another storage");return s->control;}()),
     generation_([&]{frame_control_->first();need(!frame_control_->active&&frame_control_->generation<UINT64_MAX,"Frame loan repeated/overflow");
         return ++frame_control_->generation;}()){
    state_->first();frame_control_->active=this;first();
}
CivisFrontendPartyMaterialStorage::FrameInputsLoan::~FrameInputsLoan(){
    if(creator_!=std::this_thread::get_id())std::terminate();
    if(!completed_){state_->failed.store(true);frame_control_->failed.store(true);}
    if(frame_control_->active==this)frame_control_->active=nullptr;
}
CivisFrontendPartyMaterialStorage::FrameInputsLoan::FrameInputsLoan(
    std::shared_ptr<State> state,std::shared_ptr<FrameControl> control)
    :state_(std::move(state)),frame_control_(std::move(control)),generation_([&]{
        state_->first();frame_control_->first();
        need(!frame_control_->active&&frame_control_->generation<UINT64_MAX,"Camera frame loan repeated/overflow");
        return ++frame_control_->generation;}()){
    frame_control_->active=this;first();
}
void CivisFrontendPartyMaterialStorage::FrameInputsLoan::first()const{
    need(creator_==std::this_thread::get_id(),"Frame input belongs to another creator");state_->first();frame_control_->first();
    if(state_->checking){state_->failed.store(true);frame_control_->failed.store(true);throw std::runtime_error("Frame read during Source FULL callback");}
    need(!completed_&&frame_control_->active==this&&generation_==frame_control_->generation,"Frame input generation retired");
}
void CivisFrontendPartyMaterialStorage::FrameInputsLoan::require_current()const{first();}
void CivisFrontendPartyMaterialStorage::FrameInputsLoan::require_boundary()const{
    first();auto target_pin=frame_control_->targets.lock();need(bool(target_pin),"Frame targets retired");
    // No facade pin: retirement marks FrameControl/Material State false FIRST.
    try{state_->full();first();}catch(...){state_->failed.store(true);frame_control_->failed.store(true);throw;}
}
void CivisFrontendPartyMaterialStorage::FrameInputsLoan::finish_owned(){require_boundary();first();completed_=true;frame_control_->active=nullptr;}
const std::shared_ptr<const materials::MaterialWorld>& CivisFrontendPartyMaterialStorage::FrameInputsLoan::world_owned()const{first();return *frame_control_->world;}
const terrain::WorldLayout& CivisFrontendPartyMaterialStorage::FrameInputsLoan::layout_owned()const{first();return state_->world->terrain().source().layout;}
const terrain::CameraFrameState& CivisFrontendPartyMaterialStorage::FrameInputsLoan::camera_owned()const{first();return *frame_control_->camera;}
const terrain::CameraControllerState& CivisFrontendPartyMaterialStorage::FrameInputsLoan::controller_owned()const{first();return *frame_control_->controller;}
std::weak_ptr<graphics::GlContext> CivisFrontendPartyMaterialStorage::FrameInputsLoan::context_control_owned()const{first();return state_->context;}
const graphics::GlTexture& CivisFrontendPartyMaterialStorage::FrameInputsLoan::color_owned()const{first();return *frame_control_->color;}
const graphics::GlTexture& CivisFrontendPartyMaterialStorage::FrameInputsLoan::depth_owned()const{first();return *frame_control_->depth;}
const CivisSavedWorldLoadingOwner::RendererLoadingReturn& CivisFrontendPartyMaterialStorage::FrameInputsLoan::loading_owned()const{first();return *state_->loading;}
std::uint32_t CivisFrontendPartyMaterialStorage::FrameInputsLoan::width_owned()const{first();return frame_control_->width;}
std::uint32_t CivisFrontendPartyMaterialStorage::FrameInputsLoan::height_owned()const{first();return frame_control_->height;}
void CivisFrontendPartyMaterialStorage::FrameInputsLoan::require_same_world_owned(const materials::MaterialWorld& actual)const{
    first();need(state_->world.get()==&actual&&frame_control_->world->get()==&actual,"Frame material generation differs");
}
assets::AssetHandle CivisFrontendPartyMaterialStorage::FrameInputsLoan::stage_asset_owned(const assets::AssetKey& key)const{
    require_boundary();need(key.safe_relative_path(),"Unsafe alpha resource key");
    std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> source;
    std::optional<assets::AssetHandle> result;
    try{
        source=CivisFrontendPartyWorldSource::begin_loading_inputs_from_control_owned(*state_->control);first();
        source->require_context_control_owned(state_->context);first();
        {
            SelectedTerrainAssets assets(*source);auto lookup=assets.find(key);first();
            auto* handle=std::get_if<assets::AssetHandle>(&lookup);need(handle,"Alpha authored resource missing/ambiguous");
            result.emplace(std::move(*handle));source->require_current();first();
        }
        source->finish_owned();first();source.reset();first();require_boundary();return std::move(*result);
    }catch(...){const auto error=std::current_exception();state_->failed.store(true);frame_control_->failed.store(true);
        result.reset();source.reset(); // Caller retires frame targets before final Source POST.
        std::rethrow_exception(error);}
}
CivisFrontendAlphaLandFrame::CivisFrontendAlphaLandFrame(std::shared_ptr<State> state):state_(std::move(state)){}
CivisFrontendAlphaLandFrame::~CivisFrontendAlphaLandFrame(){
    if(creator_!=std::this_thread::get_id())std::terminate();state_->control->alive.store(false);
}
std::shared_ptr<CivisFrontendAlphaLandFrame::State> CivisFrontendAlphaLandFrame::retain_frame_owned(const CivisFrontendAlphaLandFrame& actual){
    need(actual.creator_==std::this_thread::get_id(),"Alpha frame facade foreign creator");auto s=actual.state_;s->first();return s;
}
std::unique_ptr<CivisFrontendAlphaLandFrame> CivisFrontendAlphaLandFrame::capture_owned(
    const CivisFrontendPartyMaterialStorage& storage,const std::shared_ptr<graphics::GlContext>& context,
    std::uint32_t width,std::uint32_t height){
    auto material=storage.retain_current_owned();const auto retirement_control=material->control;
    std::shared_ptr<State> s;
    try{
        need(context&&width&&height&&std::uint64_t(width)*height*4<=INT32_MAX,"Alpha actual framebuffer extent invalid");
        need(alpha_same_control(material->context,std::weak_ptr<graphics::GlContext>(context)),"Alpha capture context differs");
        const auto precision=material->loading->precision_owned();material->first();
        auto reciprocal=material->loading->reciprocal_owned();material->first();
        auto reciprocal_sqrt=material->loading->reciprocal_sqrt_owned();material->first();
        s=std::make_shared<State>(material,context,width,height,precision,std::move(reciprocal),std::move(reciprocal_sqrt));s->first();
        s->fog.emplace(material->loading->saved_scene_owned().fog);s->first();
        const auto& terrain=s->world->terrain();const auto& layout=terrain.source().layout;const auto& hp=terrain.owner().parameters;
        terrain::CameraControllerState prior{};prior.wrap_orientation=1; // New own controller prior, no retained Original pose.
        s->controller=terrain::initialize_camera(layout,terrain.source().map_mode,{width,height,11},prior,precision);
        // Initial own SetPosition consumes the actual active-team saved art in
        // serialized plot order. It never sorts/fits an overview or replays input.
        for(const auto& plot:material->team_art->plots){
            if(plot.revealed!=true||plot.visibility_count<=0)continue;
            need(plot.plot_index<material->projection->grid.size(),"Alpha initial saved plot outside map");
            const auto px=static_cast<std::int32_t>(plot.plot_index%layout.source_grid.width);
            const auto py=static_cast<std::int32_t>(plot.plot_index/layout.source_grid.width);
            const auto a=terrain::wrap_add(px,terrain::wrap_neg(py>>1)),c=terrain::wrap_neg(terrain::wrap_add(a,py));
            const auto& m=layout.source_grid.second_matrix;const auto& w=layout.transforms;
            const auto gx=terrain::wrap_add(terrain::wrap_add(terrain::wrap_add(terrain::wrap_mul(a,m[0]),m[12]),terrain::wrap_mul(c,m[8])),terrain::wrap_mul(py,m[4]));
            const auto gy=terrain::wrap_add(terrain::wrap_add(terrain::wrap_add(terrain::wrap_mul(py,m[5]),m[13]),terrain::wrap_mul(c,m[9])),terrain::wrap_mul(a,m[1]));
            const float x=static_cast<float>(gx),y=static_cast<float>(gy),z=-x-y;
            const float ground_x=hp.scale_x*(x*w[16]+z*w[24]+y*w[20]+w[28])-hp.center_x;
            const float ground_y=hp.scale_y*(x*w[17]+z*w[25]+y*w[21]+w[29])-hp.center_y;
            const terrain::Vector3 position{ground_x,ground_y+s->controller.target[2]*s->controller.camera.world[9]/s->controller.camera.world[10],s->controller.target[2]};
            auto actual=terrain::set_camera_position(layout,terrain.source().map_mode,s->controller,position,precision);
            if(actual.target!=position)continue;s->controller=std::move(actual);s->initial_plot=plot.plot_index;break;
        }
        need(s->initial_plot.has_value(),"Alpha saved art has no legal visible initial camera position");
        s->camera=terrain::camera_frame(layout,terrain.source().map_mode,s->controller.camera,{},precision,s->reciprocal);
        need(s->camera.view_count>0&&s->camera.view_count<=4,"Alpha actual camera has no views");
        const auto bounds=terrain::view_bounds(layout,std::span(s->camera.frusta).first(s->camera.view_count),{hp.scale_x,hp.scale_y},{},precision);
        s->fow_frame=materials::fog_of_war_scene_frame(layout,hp,*s->fog,s->camera,bounds,{},2,true,true,false,precision);
        const auto lighting=materials::terrain_lighting_defaults(precision);
        const auto normalized=(s->controller.zoom-s->controller.minimum_zoom)/(s->controller.maximum_zoom-s->controller.minimum_zoom);
        const auto global=material->lighting->distance_fog().frame(normalized);
        for(unsigned view=0;view<s->camera.view_count;++view){
            const materials::TerrainFrameInput input{s->camera.views[view],s->camera.inverse_views[view],s->camera.projection,{},s->fow_frame.overlays[view],
                {hp.scale_x,hp.scale_y,hp.scale_z},lighting.shadow,lighting.sun,
                {static_cast<unsigned>(s->fog->width()),static_cast<unsigned>(s->fog->height())},{width,height},s->fog->frame_scale(),
                global.color,global.parameters[0],global.parameters[1],global.parameters[3],2048,0,1,{2,1,1,1}};
            s->land.push_back(materials::land_material_frame(materials::terrain_frame(input,{},precision,s->reciprocal_sqrt)));
        }
        s->boundary();
        {
            AlphaRasterScope restore(*context,s->gl);
            s->color.emplace(graphics::GlTexture::render_target(*context,width,height,1,graphics::RenderTextureFormat::rgba8));
            s->depth.emplace(graphics::GlTexture::render_target(*context,width,height,1,graphics::RenderTextureFormat::depth24_stencil8));
            s->pick_depth.emplace(graphics::GlTexture::render_target(*context,width,height,1,graphics::RenderTextureFormat::depth24_stencil8));
            s->renderer=std::make_unique<graphics::GlMaterialWorldRenderer>(context,s->world,material->lighting->lighting_assets(),
                graphics::MaterialCacheMipPolicy{false,true,true},graphics::LightingCubeSampling::measured_coordinates,
                graphics::LandShadowArithmetic::measured_fma_t_x_z_y,graphics::LandMipSampling::d3d9_rgb565,material->lighting->color_keys_owned());
            s->fow=std::make_unique<graphics::GlFogOfWarMap>(context,*material->fog_assets);
            s->gl.CreateFramebuffers(1,&s->framebuffer);s->gl.NamedFramebufferTexture(s->framebuffer,GL_COLOR_ATTACHMENT0,s->color->id(),0);
            s->gl.NamedFramebufferTexture(s->framebuffer,GL_DEPTH_STENCIL_ATTACHMENT,s->depth->id(),0);
            const GLenum target=GL_COLOR_ATTACHMENT0;s->gl.NamedFramebufferDrawBuffers(s->framebuffer,1,&target);
            need(s->gl.CheckNamedFramebufferStatus(s->framebuffer,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Alpha actual LAND framebuffer incomplete");
        }
        s->boundary();s->control->targets=s;s->control->world=&s->world;s->control->camera=&s->camera;s->control->controller=&s->controller;
        s->control->color=&*s->color;s->control->depth=&*s->depth;s->control->width=width;s->control->height=height;
        auto result=std::unique_ptr<CivisFrontendAlphaLandFrame>(new CivisFrontendAlphaLandFrame(s));s->boundary();return result;
    }catch(...){const auto error=std::current_exception();if(s)s->control->failed.store(true);
        s.reset();material.reset();try{retirement_control->require_current_owned();}catch(...){}std::rethrow_exception(error);}
}
void CivisFrontendAlphaLandFrame::render_first_owned(const CivisFrontendPartyMaterialStorage& storage,
    const FrontendSelectedSaveInput& selected,const std::filesystem::path& output){
    auto s=retain_frame_owned(*this);const auto retirement_control=s->material->control;
    std::unique_ptr<CivisFrontendPartyMaterialStorage::FrameInputsLoan> loan;
    try{
        need(!s->attempted&&!s->busy&&s->material==storage.state_,"Alpha first frame repeated/wrong storage");s->attempted=true;s->busy=true;
        storage.require_selected_owned(selected,*s->context);s->first();s->boundary();
        need(!output.empty()&&std::filesystem::is_directory(output)&&std::filesystem::is_empty(output),"Alpha output must be this fresh empty occurrence");s->first();
        loan=std::unique_ptr<CivisFrontendPartyMaterialStorage::FrameInputsLoan>(new CivisFrontendPartyMaterialStorage::FrameInputsLoan(storage,*this));
        loan->require_boundary();s->first();
        need(!s->water,"Alpha water capture repeated");s->water=CivisBasicWaterSurface::capture_owned(*loan);s->first();loan->require_boundary();s->first();
        const auto& profile=*s->material->profile;const auto& quality=profile.configured.material.quality;
        need(quality.base_size==64&&quality.cache_mode==0&&quality.compression==0,"Alpha material quality requires ordinary diffuse");
        const auto lighting=graphics::GlMaterialWorldLighting::from_terrain(materials::terrain_lighting_defaults(s->precision),s->reciprocal_sqrt);
        assets::Bytes rgba;std::vector<float> depth;
        {
            AlphaRasterScope restore(*s->context,s->gl);
            const materials::MaterialNormalSeed normal=[&](int,int,float v){loan->require_current();return s->reciprocal_sqrt(v);};
            s->renderer->enable_scheduling(std::span<const std::uint32_t,4>(quality.pool_counts.data(),4),
                {64,0,0,0,s->precision},{quality.temporary_size,quality.cache_size});
            s->renderer->begin_loading(materials::material_cache_camera_views(s->camera),profile.configured.block_on_load!=0,
                materials::material_cache_initial_cull(profile.loading,true,1));
            const auto max_steps=(s->world->cache().patches.size()+3)/4+2;
            for(std::size_t n=0;;++n){loan->require_current();const auto loaded=s->renderer->load_step(1,normal,normal,lighting,4,true);
                loan->require_current();need(loaded.prepared==loaded.completed&&loaded.completed==loaded.generated,"Alpha material loading barrier differs");
                if(loaded.finished)break;need(n<max_steps,"Alpha material loading made no bounded progress");}
            materials::MaterialCacheSchedulerSettings schedule{};schedule.viewport_width=s->width;schedule.cull=true;schedule.precision=s->precision;
            materials::material_cache_camera_controls({},profile.configured.material,schedule);
            const auto ready=s->renderer->schedule_camera_frame(s->camera,schedule,2,normal,normal,lighting,4,true);loan->require_current();
            need(ready.prepared==ready.completed&&s->renderer->scheduler_state().pending.empty(),"Alpha material scheduler completion missing");
            {
                const std::function<void(const graphics::GlTexture&)> scene=[&](const graphics::GlTexture& mask){
                    loan->require_current();s->first();s->gl.BindFramebuffer(GL_FRAMEBUFFER,s->framebuffer);s->gl.Viewport(0,0,s->width,s->height);
                    for(auto flag:{GL_BLEND,GL_SCISSOR_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_DITHER,GL_FRAMEBUFFER_SRGB,GL_RASTERIZER_DISCARD})s->gl.Disable(flag);
                    s->gl.Enable(GL_DEPTH_TEST);s->gl.DepthFunc(GL_LEQUAL);s->gl.DepthMask(GL_TRUE);s->gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
                    s->gl.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);s->gl.DepthRange(0,1);
                    const std::array<GLfloat,4> clear{0,0,0,1};const GLfloat clear_depth=1;
                    s->gl.ClearNamedFramebufferfv(s->framebuffer,GL_COLOR,0,clear.data());s->gl.ClearNamedFramebufferfv(s->framebuffer,GL_DEPTH,0,&clear_depth);
                    for(unsigned view=0;view<s->camera.view_count;++view){
                        auto constants=s->land.at(view);graphics::GlLandMaterialInputs images{};
                        images.pixel[2]=s->material->lighting->distance_fog().ramp().id();images.pixel[4]=mask.id();
                        images.pixel[5]=s->fow->clouds().at(0).id();images.pixel[9]=s->material->lighting->spec_screen().id();
                        s->material->layers->apply_to(constants,images);
                        for(auto index:s->renderer->scheduler_state().visible[view]){
                            loan->require_current();s->renderer->draw(index,constants,images,static_cast<int>(s->width),static_cast<int>(s->height));}
                    }
                    // ONE retained water owner, SAME bound LAND colour/depth.
                    loan->require_current();s->water->draw_owned(*loan);loan->require_current();
                    const auto copy=reinterpret_cast<PFNGLCOPYIMAGESUBDATAPROC>(s->context->proc_address("glCopyImageSubData"));
                    copy(s->depth->id(),GL_TEXTURE_2D,0,0,0,0,s->pick_depth->id(),GL_TEXTURE_2D,0,0,0,0,s->width,s->height,1);
                    s->context->check("Alpha retain actual LAND/water depth before FOW");loan->require_current();
                };
                s->fow->render(s->fow_frame.commands,*s->color,*s->depth,scene);loan->require_current();
            } // Actual scene closure and all borrowed target copies retire.
            s->renderer->finish_frame();loan->require_current();
            rgba.resize(std::size_t(s->width)*s->height*4);depth.resize(std::size_t(s->width)*s->height);
            s->context->api().BindBuffer(GL_PIXEL_PACK_BUFFER,0);s->context->api().PixelStorei(GL_PACK_ALIGNMENT,1);
            for(auto name:AlphaRasterScope::pack_names)s->context->api().PixelStorei(name,0);
            s->context->api().GetTextureImage(s->color->id(),0,GL_RGBA,GL_UNSIGNED_BYTE,static_cast<GLsizei>(rgba.size()),rgba.data());
            s->context->api().GetTextureImage(s->pick_depth->id(),0,GL_DEPTH_COMPONENT,GL_FLOAT,static_cast<GLsizei>(depth.size()*4),depth.data());
            s->context->check("Alpha real colour/depth readback");loan->require_current();
        } // Restored actual framebuffer/raster state BEFORE Source FULL.
        loan->require_boundary();s->first();
        const auto populated=std::count_if(depth.begin(),depth.end(),[](float d){return std::isfinite(d)&&d>=0&&d<1;});
        need(populated>0,"Alpha actual LAND/water depth contains no produced pixels");
        const auto png=alpha_png(s->width,s->height,rgba);alpha_write_exclusive(output/"world32-alpha-diagnostic.png",png);
        const auto db=std::as_bytes(std::span(depth));alpha_write_exclusive(output/"world32-alpha-world-depth.f32",
            {reinterpret_cast<const std::uint8_t*>(db.data()),db.size()});
        std::ostringstream report;report<<"{\"schema\":\"civis-own-alpha-saved-world-diagnostic-v1\",\"saveSha256\":\""<<selected.save_identity().sha256
            <<"\",\"width\":"<<s->width<<",\"height\":"<<s->height<<",\"views\":"<<s->camera.view_count<<",\"initialSavedPlot\":"<<*s->initial_plot
            <<",\"worldDepthPixels\":"<<populated<<",\"sameCameraLayoutDepth\":true,\"basicWater\":true,\"savedFow\":true,\"cameraWorld\":[";
        for(std::size_t n=0;n<s->controller.camera.world.size();++n){if(n)report<<',';report<<std::setprecision(9)<<s->controller.camera.world[n];}
        report<<"],\"cameraProjection\":[";for(std::size_t n=0;n<s->camera.projection.size();++n){if(n)report<<',';report<<std::setprecision(9)<<s->camera.projection[n];}
        report<<"],\"installedClientSource\":false,\"normalCameraLoopReturn\":false,\"frontendPerformed\":false,\"fullGraphicsParity\":false}\n";
        const auto text=report.str();alpha_write_exclusive(output/"world32-alpha-diagnostic.json",
            {reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});
        loan->finish_owned();s->first();loan.reset();s->first();s->boundary();
        s->drawn=true;s->busy=false;
    }catch(...){const auto error=std::current_exception();s->control->failed.store(true);s->material->failed.store(true);
        // Retire actual frame targets on the live creator/context, before final
        // source validation. This facade can survive only as a quarantined shell.
        loan.reset();s->water.reset();s->fow.reset();s->renderer.reset();s->release();s->pick_depth.reset();s->depth.reset();s->color.reset();
        s->busy=false;s.reset();try{retirement_control->require_current_owned();}catch(...){}std::rethrow_exception(error);}
}
std::optional<std::int64_t> CivisFrontendAlphaLandFrame::pick_owned(const ui::BnwClientFrame& actual,int x,int y)const{
    auto s=retain_frame_owned(*this);const auto retirement_control=s->material->control;
    try{
        need(s->drawn&&!s->busy&&s->pick_depth,"Alpha pick requires its actual completed world depth");s->boundary();
        need(actual.world.map_projection&&actual.stamp.identity.owner&&actual.stamp.identity.revision&&
            actual.camera.camera.world==s->controller.camera.world&&actual.camera.camera.field_of_view==s->controller.camera.field_of_view&&
            actual.camera.camera.aspect_ratio==s->controller.camera.aspect_ratio,"Alpha pick actual client/camera differs");
        const auto& a=*actual.world.map_projection;const auto& b=*s->material->projection;
        need(a.grid.width==b.grid.width&&a.grid.height==b.grid.height&&a.wrap_x==b.wrap_x&&a.wrap_y==b.wrap_y&&
            a.cells.size()==b.cells.size(),"Alpha picked source map differs");
        for(std::size_t n=0;n<a.cells.size();++n){const auto& x=a.cells[n];const auto& y=b.cells[n];
            need(x.index==y.index&&x.x==y.x&&x.y==y.y&&x.plot_type==y.plot_type&&x.terrain_type==y.terrain_type&&
                x.feature_type==y.feature_type&&x.feature_type_hash==y.feature_type_hash&&x.native_terrain_mask==y.native_terrain_mask&&
                x.native_terrain_mask_known==y.native_terrain_mask_known&&x.native_category==y.native_category&&x.rivers==y.rivers,
                "Alpha picked immutable terrain source changed");}
        if(x<0||y<0||x>=static_cast<int>(s->width)||y>=static_cast<int>(s->height))return {};
        float depth=1;{
            AlphaRasterScope restore(*s->context,s->gl);s->context->api().BindBuffer(GL_PIXEL_PACK_BUFFER,0);
            s->context->api().PixelStorei(GL_PACK_ALIGNMENT,1);
            for(auto name:AlphaRasterScope::pack_names)s->context->api().PixelStorei(name,0);
            s->context->api().GetTextureSubImage(s->pick_depth->id(),0,x,y,0,1,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,sizeof(depth),&depth);
            s->context->check("Alpha actual canonical plot depth read");s->first();
        }
        auto result=ui::world_plot_from_rendered_depth(actual,s->world->terrain().source().layout,s->world->terrain().owner().parameters,
            s->camera,x,y,static_cast<int>(s->width),static_cast<int>(s->height),depth,s->reciprocal);s->boundary();return result;
    }catch(...){const auto error=std::current_exception();s->control->failed.store(true);s.reset();
        try{retirement_control->require_current_owned();}catch(...){}std::rethrow_exception(error);}
}
terrain::CameraControllerState CivisFrontendAlphaLandFrame::initial_controller_for_camera_owned()const{
    const auto s=retain_frame_owned(*this);need(s->drawn&&!s->busy,"Camera requires actual initialized diagnostic producer");return s->controller;
}
std::pair<std::shared_ptr<graphics::GlTexture>,std::shared_ptr<graphics::GlTexture>> CivisFrontendAlphaLandFrame::camera_targets_owned()const{
    const auto s=retain_frame_owned(*this);need(s->drawn&&!s->busy&&s->color&&s->depth,"Camera actual colour/depth targets absent");
    return {std::shared_ptr<graphics::GlTexture>(s,&*s->color),std::shared_ptr<graphics::GlTexture>(s,&*s->depth)};
}
assets::Bytes CivisFrontendAlphaLandFrame::stage_camera_asset_owned(const assets::AssetKey& key)const{
    auto s=retain_frame_owned(*this);s->boundary();
    auto loan=std::unique_ptr<CivisFrontendPartyMaterialStorage::FrameInputsLoan>(
        new CivisFrontendPartyMaterialStorage::FrameInputsLoan(s->material,s->control));
    try{
        assets::Bytes bytes;{
            const auto actual=loan->stage_asset_owned(key);loan->require_boundary();s->first();
            bytes=actual.read_bytes();loan->require_boundary();s->first();
        } // Real AssetHandle retires before the loan's final Source POST.
        loan->finish_owned();s->first();loan.reset();s->boundary();return bytes;
    }catch(...){const auto error=std::current_exception();s->control->failed.store(true);s->material->failed.store(true);
        loan.reset();s.reset();std::rethrow_exception(error);}
}
void CivisFrontendAlphaLandFrame::require_camera_boundary_owned()const{
    auto s=retain_frame_owned(*this);s->boundary();
}
void CivisFrontendAlphaLandFrame::attach_camera_producer_owned(graphics::GlWaterWorldRenderer& actual)const{
    const auto s=retain_frame_owned(*this);s->boundary();
    const std::shared_ptr<const terrain::TerrainWorld> canonical(s->world,&s->world->terrain());
    const auto& source=actual.source_world_owned_without_callbacks();
    need(source.get()==canonical.get()&&alpha_same_control(source,canonical),"Actual camera producer terrain control differs");
    actual.use_external_surface_owned();s->first();s->boundary();
}
void CivisFrontendAlphaLandFrame::with_current_camera_frame_owned(
    const graphics::GlWaterWorldRenderer& actual_water,const terrain::CameraControllerState& actual_controller,
    const std::function<void(const CivisFrontendPartyMaterialStorage::FrameInputsLoan&)>& consume)const{
    auto s=retain_frame_owned(*this);
    std::unique_ptr<CivisFrontendPartyMaterialStorage::FrameInputsLoan> loan;
    try{
        need(s->drawn&&!s->busy&&!s->control->active&&consume,"Actual Camera frame reentry/absent consumer");s->boundary();
        const auto& actual_world=actual_water.source_world_owned_without_callbacks();
        const std::shared_ptr<const terrain::TerrainWorld> canonical(s->world,&s->world->terrain());
        need(actual_world.get()==canonical.get()&&alpha_same_control(actual_world,canonical),
            "Current Camera uses another terrain pointer/control generation");
        // The existing closed CameraPreviewLoop creates actual_water from the
        // SAME alias of material world. It owns the normal controller/input;
        // borrow its already-published VP, never initialize a second camera.
        const auto& published=actual_water.camera_state();
        const auto expected=terrain::camera_frame(canonical->source().layout,canonical->source().map_mode,
            actual_controller.camera,published,s->precision,s->reciprocal);s->first();
        need(expected.view_count==published.view_count&&published.view_count<=4&&expected.projection==published.projection,
            "Current Camera controller/published projection differs");
        for(unsigned view=0;view<published.view_count;++view){
            need(expected.views[view]==published.views[view]&&expected.inverse_views[view]==published.inverse_views[view]&&
                expected.frusta[view]==published.frusta[view]&&expected.relative_offsets[view]==published.relative_offsets[view],
                "Current Camera controller/published active view differs");
        } // Inactive/zero-view storage retains the producer's actual prior bits.
        s->controller=actual_controller;s->camera=published;s->first();s->busy=true;
        loan=std::unique_ptr<CivisFrontendPartyMaterialStorage::FrameInputsLoan>(
            new CivisFrontendPartyMaterialStorage::FrameInputsLoan(s->material,s->control));loan->require_boundary();s->first();
        {
            const auto actual_consumer=consume;loan->require_current();
            {
                AlphaRasterScope restore(*s->context,s->gl);
                actual_consumer(*loan);loan->require_current();s->first();
            } // Whole LAND/basic-water/FOW target/raster restoration FIRST.
        } // Callback target destruction before fresh Source FULL.
        const auto& returned=actual_water.camera_state();
        need(actual_controller.camera.world==s->controller.camera.world&&
            actual_controller.camera.field_of_view==s->controller.camera.field_of_view&&
            actual_controller.camera.aspect_ratio==s->controller.camera.aspect_ratio&&
            actual_controller.camera.near_plane==s->controller.camera.near_plane&&
            actual_controller.camera.far_plane==s->controller.camera.far_plane&&
            returned.view_count==s->camera.view_count&&returned.projection==s->camera.projection,
            "Actual Camera generation changed during frame callback retirement");
        for(unsigned view=0;view<returned.view_count;++view){
            need(returned.views[view]==s->camera.views[view]&&returned.inverse_views[view]==s->camera.inverse_views[view]&&
                returned.frusta[view]==s->camera.frusta[view]&&returned.relative_offsets[view]==s->camera.relative_offsets[view],
                "Actual current Camera view changed during frame callback retirement");
        }
        loan->finish_owned();s->first();loan.reset();s->boundary();s->busy=false;
    }catch(...){const auto error=std::current_exception();s->control->failed.store(true);s->material->failed.store(true);
        loan.reset();s->busy=false;s.reset();
        // The caller's SceneTarget/GUI callbacks still own these actual target
        // aliases. The backend performs final Source POST after loop unwind.
        std::rethrow_exception(error);}
}
void CivisFrontendAlphaLandFrame::draw_basic_water_owned(const CivisFrontendPartyMaterialStorage::FrameInputsLoan& actual)const{
    const auto s=retain_frame_owned(*this);actual.require_current();
    need(s->busy&&s->water&&actual.frame_control_==s->control,"Basic water requires actual current Camera frame loan");
    if(s->camera.view_count==0)return; // Legitimate empty current view: no surface draws.
    s->water->draw_owned(actual);actual.require_current();s->first();
}
void CivisFrontendAlphaLandFrame::apply_land_layers_owned(const CivisFrontendPartyMaterialStorage::FrameInputsLoan& actual,
    materials::LandMaterialConstants& constants,graphics::GlLandMaterialInputs& images)const{
    const auto s=retain_frame_owned(*this);actual.require_current();
    need(s->busy&&actual.frame_control_==s->control,"LAND layers require actual current Camera frame loan");
    s->material->layers->apply_to(constants,images);actual.require_current();s->first();
}
