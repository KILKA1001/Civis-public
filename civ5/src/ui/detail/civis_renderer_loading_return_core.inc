// Included once inside civ5::ui::detail after Child State/Scope definitions.
// All buffers below are explicitly initialized NEW own renderer storage.
namespace {
template<class A,class B>bool render_same_control(const A& a,const B& b){return !a.owner_before(b)&&!b.owner_before(a);}
struct RenderSeedV1 {
    std::uint64_t value=14695981039346656037ull;
    void byte(std::uint8_t b){value=(value^b)*1099511628211ull;}
    void word(std::uint32_t v){for(unsigned n=0;n<4;++n)byte(static_cast<std::uint8_t>(v>>(8*n)));}
    std::uint32_t cursor(unsigned domain)const {
        auto v=value^(0x9e3779b97f4a7c15ull*domain);v^=v>>30;v*=0xbf58476d1ce4e5b9ull;
        v^=v>>27;v*=0x94d049bb133111ebull;v^=v>>31;
        return static_cast<std::uint32_t>(v^(v>>32));
    }
};
template<class G>G own_aux_grid(const terrain::WorldGrid& grid,bool wrap){
    G v{};v.coordinates=grid;v.wrap_x=wrap;v.cells.resize(G::capacity);return v;
}
}
struct CivisSavedWorldLoadingOwner::RendererLoadingReturn::State final {
    const std::thread::id creator=std::this_thread::get_id();
    // Bind FP controls only; actual sticky status flags are preserved.
    const unsigned mxcsr=_mm_getcsr() & ~0x3fu;
    std::atomic<bool> alive{true},failed{false};
    bool validating=false;
    std::weak_ptr<const RendererLoadingReturn> self;
    std::shared_ptr<const CivisFrontendPartyWorldSource::LoadingControl> source;
    const saves::SavedSessionSnapshot* snapshot_identity=nullptr;
    std::unique_ptr<const scene::SavedSceneSources> scene;
    std::unique_ptr<const terrain::LandStyleGrid> prior_styles;
    std::unique_ptr<const terrain::TerrainAuxiliaryOwnerState> auxiliary;
    std::vector<std::uint32_t> land_masks,codes,mapping;
    terrain::TerrainWorldInput input{};
    std::uint32_t river_random=0;
    void first()const {
        need(creator==std::this_thread::get_id(),"Own render loading foreign creator");
        need(alive.load()&&!failed.load()&&!self.expired()&&source,
             "Own render loading return retired/quarantined");
        need((_mm_getcsr() & ~0x3fu)==mxcsr,"Own render FP environment changed");
    }
    void full(){
        first();if(validating){failed.store(true);throw std::runtime_error("Own render loading callback reentry");}
        validating=true;struct End{State& s;~End(){s.validating=false;}}end{*this};
        try{source->require_current_owned();first();}
        catch(...){failed.store(true);throw;}
    }
    void pure() {first();if(validating){failed.store(true);throw std::runtime_error("Own render read inside Source callback");}}
};
CivisSavedWorldLoadingOwner::RendererLoadingReturn::RendererLoadingReturn(std::shared_ptr<State> s):state_(std::move(s)){}
CivisSavedWorldLoadingOwner::RendererLoadingReturn::~RendererLoadingReturn(){state_->alive.store(false);}

std::shared_ptr<const CivisSavedWorldLoadingOwner::RendererLoadingReturn>
CivisSavedWorldLoadingOwner::capture_renderer_loading_owned(
    const CivisFrontendPartyWorldSource::LoadingInputsLoan& actual,
    const terrain::TerrainPieceLibrary& pieces,const materials::MaterialAssets& assets)const {
    Scope scope(*this);
    try {
        const auto check=[&]{
            scope.check();actual.require_current();scope.check();
            need(render_same_control(actual.source_control_owned_without_callbacks(),scope.state->source->facade_)&&
                 &actual.snapshot_owned()==scope.state->snapshot_identity,
                 "Own renderer loading uses another actual Source/snapshot");scope.check();
        };
        check();auto s=std::make_shared<RendererLoadingReturn::State>();check();
        s->source=scope.state->source;s->snapshot_identity=scope.state->snapshot_identity;
        {
        // Named OWN cube-coordinate basis; never a claimed Original factory return.
        const terrain::FactoryDescriptor own_unshifted_cube_basis{0,0,0,0,0};
        const auto& saved=*scope.state->terrain_source;
        need(!saved.wrap_y&&saved.grid.width>0&&saved.grid.height>0&&saved.grid.width<=200&&saved.grid.height<=200,
             "Own alpha loading exceeds supported auxiliary grid or vertical wrap");
        const auto rcp=[&](float v){
            scope.check();need((_mm_getcsr() & ~0x3fu)==s->mxcsr,"Own layout FP changed");
            return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v)));
        };
        s->scene=std::make_unique<const scene::SavedSceneSources>(scene::build_saved_scene_sources(
            saved,*scope.state->art,0,own_unshifted_cube_basis,rcp));check();
        const auto& layout=s->scene->world;
        const auto mode=terrain::game_world_grid_mode(saved.wrap_x);
        need(layout.source_grid.width==static_cast<std::int32_t>(saved.grid.width)&&
             layout.source_grid.height==static_cast<std::int32_t>(saved.grid.height),"Own saved/layout extent mismatch");
        RenderSeedV1 seed;seed.word(0x43525631u); // CRV1 OWN visual policy, not save/game RNG.
        seed.word(static_cast<std::uint32_t>(saved.grid.width));seed.word(static_cast<std::uint32_t>(saved.grid.height));
        seed.word(saved.wrap_x);seed.word(saved.wrap_y);
        for(const auto& v:scope.state->classified){seed.word(v.mask);seed.byte(v.category);}
        for(const auto& v:scope.state->auxiliary){seed.word(v.terrain);seed.word(v.plot);seed.word(v.continent);seed.word(v.feature);}
        for(const auto b:pieces.serialized_bytes())seed.byte(b);
        s->codes=materials::terrain_material_codes(assets.rows());check();
        need(!s->codes.empty()&&!pieces.serialized_bytes().empty(),"Own render loading lacks actual authored pieces/provider");
        for(auto code:s->codes)seed.word(code);
        // Actual first supported authored continent is common fallback. Dominant
        // available saved continent selects the own coast family policy.
        std::array<std::uint64_t,5> counts{};std::array<bool,5> available{};
        int common=-1;for(const auto& row:assets.rows())if(row.continent>=0&&row.continent<5){
            available[static_cast<unsigned>(row.continent)]=true;if(common<0)common=row.continent;
        }
        need(common>=0,"Own render coast policy has no actual continent rows");
        for(const auto& v:scope.state->auxiliary)if(v.continent>=0&&v.continent<5)++counts[static_cast<unsigned>(v.continent)];
        unsigned family=static_cast<unsigned>(common);
        for(unsigned n=0;n<5;++n)if(available[n]&&counts[n]>counts[family])family=n;
        auto& in=s->input;in.map_width=layout.source_grid.width;in.map_height=layout.source_grid.height;
        in.mode=mode;in.factory=own_unshifted_cube_basis;
        // These three fields are genuinely populated now, and are re-derived
        // by MaterialWorld::build -> TerrainWorld::build_from_library too.
        const auto setup=terrain::prepare_terrain_generation(pieces,in.factory);check();
        in.orientations=setup.orientations;in.maximum_pass=setup.maximum_pass;
        s->mapping=setup.cooked_to_primary;check();
        in.cooked_to_primary=s->mapping;in.expanded_logical_cells=s->scene->terrain_grid.cells;
        in.random_states={seed.cursor(1),seed.cursor(2),seed.cursor(3)};s->river_random=seed.cursor(4);
        in.precision={terrain::X87Precision::double_precision,terrain::X87Precision::double_precision,
            terrain::X87Precision::double_precision,terrain::X87Precision::double_precision,
            terrain::Vec4Path::sse,terrain::Vec4Path::sse,terrain::MatrixPath::sse,
            terrain::CoastPowPath::sse2,terrain::CoastBoundsPrecision::double_or_extended};
        in.worker_bias=0.f;in.wrap_x=saved.wrap_x;in.outside_gate_category=255;in.coast_palette=s->codes;
        in.coast_family_prefix=0x100000u+family*0x10000u;
        in.coast_common_prefix=0x100000u+static_cast<unsigned>(common)*0x10000u;
        in.coast_dispatch_gate=0;
        // Legacy dispatch argument255 encodes OWN ordinary zero allocation
        // flags (helper-255). No Original CRC execution/result is asserted.
        in.coast_helper_return=255;
        const auto parameters=terrain::HeightOwnerParameters::from_world(layout);
        const terrain::LandLayout land(terrain::HeightDomain(parameters.width,parameters.height));
        s->land_masks.assign(static_cast<std::size_t>(land.columns())*land.rows(),0u);
        const std::vector<std::uint8_t> own_inactive_styles(terrain::LandStyleGrid::size,255);
        s->prior_styles=std::make_unique<const terrain::LandStyleGrid>(own_inactive_styles);check();
        // OWN allocated prior/fallback bytes are zero; real saved relief/ice/
        // flood mutations execute through the existing integer effect body.
        terrain::TerrainAuxiliaryOwnerState prior{true,true,false,
            own_aux_grid<terrain::TerrainAuxiliaryGrid<255,8>>(layout.source_grid,saved.wrap_x),
            own_aux_grid<terrain::TerrainAdjacencyGrid>(layout.source_grid,saved.wrap_x),
            own_aux_grid<terrain::TerrainAuxiliaryGrid<256,4,false>>(layout.source_grid,false),
            own_aux_grid<terrain::TerrainIceGrid>(layout.source_grid,saved.wrap_x),
            own_aux_grid<terrain::TerrainAuxiliaryGrid<200,4,false>>(layout.source_grid,false)};
        auto effects=terrain::apply_terrain_auxiliary_flags(prior,s->scene->auxiliary);
        effects.adjacency=terrain::finalize_terrain_auxiliary_adjacency(effects.adjacency);
        s->auxiliary=std::make_unique<const terrain::TerrainAuxiliaryOwnerState>(std::move(effects));check();
        } // Actual setup/prior/effect vectors and local RCP target retire here.
        check(); // Fresh Source POST after ALL local producer temporaries.
        auto result=std::shared_ptr<const RendererLoadingReturn>(new RendererLoadingReturn(s));s->self=result;
        check();s->full();scope.check();return result;
    }catch(...){scope.state->failed.store(true);throw;}
}
void CivisSavedWorldLoadingOwner::RendererLoadingReturn::require_current()const{state_->full();}
void CivisSavedWorldLoadingOwner::RendererLoadingReturn::require_same_source_owned(
    const CivisFrontendPartyWorldSource::LoadingInputsLoan& actual)const {
    state_->full();actual.require_current();state_->pure();
    need(render_same_control(actual.source_control_owned_without_callbacks(),state_->source->facade_)&&
         &actual.snapshot_owned()==state_->snapshot_identity,"Own render Return and consumer Source differ");state_->pure();
}
const scene::SavedSceneSources& CivisSavedWorldLoadingOwner::RendererLoadingReturn::saved_scene_owned()const{require_current();return *state_->scene;}
const terrain::TerrainAuxiliaryOwnerState& CivisSavedWorldLoadingOwner::RendererLoadingReturn::auxiliary_flags_owned()const{require_current();return *state_->auxiliary;}
terrain::TerrainWorldInput CivisSavedWorldLoadingOwner::RendererLoadingReturn::input_owned()const{require_current();return state_->input;}
terrain::TerrainWorldStorage CivisSavedWorldLoadingOwner::RendererLoadingReturn::storage_owned()const{
    require_current();const auto s=state_;
    terrain::PageAllocator allocate=[s](std::int32_t n){s->pure();need(n>=0,"Own render negative page allocation");
        auto bytes=std::vector<std::uint8_t>(static_cast<std::size_t>(n),0);s->pure();return bytes;};
    s->full();return {*s->prior_styles,s->land_masks,nullptr,nullptr,std::move(allocate)};
}
terrain::ReciprocalEstimate CivisSavedWorldLoadingOwner::RendererLoadingReturn::reciprocal_owned()const{
    require_current();const auto s=state_;terrain::ReciprocalEstimate fn=[s](float v){s->pure();
        const auto r=_mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v)));s->pure();return r;};s->full();return fn;
}
terrain::ReciprocalSqrtEstimate CivisSavedWorldLoadingOwner::RendererLoadingReturn::reciprocal_sqrt_owned()const{
    require_current();const auto s=state_;terrain::ReciprocalSqrtEstimate fn=[s](float v){s->pure();
        const auto r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v)));s->pure();return r;};s->full();return fn;
}
terrain::CoastExecutionOrder CivisSavedWorldLoadingOwner::RendererLoadingReturn::execution_order_owned()const{
    require_current();const auto s=state_;terrain::CoastExecutionOrder fn=[s](unsigned phase,std::span<const terrain::CoastDispatchJob> jobs){
        s->pure();need(phase<2,"Own coast phase out of range");std::vector<std::size_t> order(jobs.size());
        std::iota(order.begin(),order.end(),0);s->pure();return order;};s->full();return fn;
}
std::uint32_t CivisSavedWorldLoadingOwner::RendererLoadingReturn::river_random_owned()const{require_current();return state_->river_random;}
terrain::X87Precision CivisSavedWorldLoadingOwner::RendererLoadingReturn::precision_owned()const{require_current();return terrain::X87Precision::double_precision;}
std::span<const terrain::TerrainFeature2Source> CivisSavedWorldLoadingOwner::RendererLoadingReturn::marsh_sources_owned()const{require_current();return state_->scene->auxiliary.feature2;}
