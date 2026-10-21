// Private leaf inside civ5::ui, after the frozen scalar2 numerical bodies and
// the Root-owned complete BnwOriginalNonFontDeviceInitializer declaration.
// Root initializer lifecycle/actual creation-stage issuer is an external seam.
namespace detail {
struct BnwOriginalGeneratedResourceImageLoan::Impl final {
    using Initializer=BnwOriginalNonFontDeviceInitializer;
    using View=BnwOriginalGeneratedResourceImageLoan::Bgra8LevelView;
    const std::shared_ptr<const Initializer> initializer;
    const std::uint64_t epoch,creation_serial;
    const std::thread::id creator=std::this_thread::get_id();
    const std::array<std::vector<std::uint8_t>,7> pixels;
    const std::array<View,7> views;
    std::weak_ptr<const BnwOriginalGeneratedResourceImageLoan> canonical;

    // Actual selected D3DX9_42 format21 table34=101BD5B0, four used
    // columns of each 8-float row. Column is PACKING ITERATION, not X:
    // the original C1CDA reverses both source/output X on odd Y.
    static constexpr std::array<std::array<std::uint32_t,4>,4> offsets{{
        {0x3f780000u,0x3ef00000u,0x3f580000u,0x3eb00000u},
        {0x3e600000u,0x3f380000u,0x3dc00000u,0x3f180000u},
        {0x3f480000u,0x3e900000u,0x3f680000u,0x3ed00000u},
        {0x3d000000u,0x3f080000u,0x3e200000u,0x3f280000u}}};

    static std::array<std::uint32_t,4> original_rgba0_callback(
        std::uint32_t level,std::uint32_t x,std::uint32_t y){
        // Only the seven original dyadic mip dimensions are admitted. The
        // x87 original stores (integer+0.5)*(1/dimension) as binary32;
        // every coordinate and product used here is exactly representable.
        if(level>=7||x>=(256u>>level)||y>=(256u>>level))
            throw std::runtime_error("Original resource0 numerical domain outside actual mip");
        const auto reciprocal=std::bit_cast<float>((119u+level)<<23u);
        const auto coordinate=[reciprocal](std::uint32_t value){
            const auto half_center=_mm_add_ss(_mm_set_ss(static_cast<float>(value)),_mm_set_ss(0.5f));
            return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
                _mm_mul_ss(half_center,_mm_set_ss(reciprocal))));
        };
        return original_nonfont_resource0_fill_8e2270_output_words(
            {coordinate(x),coordinate(y)});
    }
    static std::array<std::uint8_t,4> original_bgra0_pack(
        const std::array<std::uint32_t,4>& rgba,std::uint32_t y,
        std::uint32_t packing_iteration){
        const auto offset=std::bit_cast<float>(offsets[y&3u][packing_iteration&3u]);
        const auto quantize=[offset](std::uint32_t word)->std::uint8_t{
            // Actual BFEF3 unit1->4 converter clamps RGBA to [0,1].
            // Original callback only returns these exact words, unchanged
            // by that converter. No arbitrary float/color path is granted.
            if(word!=0u&&word!=0x3f000000u&&word!=0x3f800000u)
                throw std::runtime_error("Original resource0 callback left its proved float domain");
            const auto scaled=_mm_mul_ss(_mm_set_ss(std::bit_cast<float>(word)),_mm_set_ss(255.0f));
            const auto shifted=_mm_add_ss(scaled,_mm_set_ss(offset));
            // C1D70 forces x87 truncation; selected products and sums are
            // exact dyadic binary32, so CVTT has the SAME integer result.
            const auto actual=_mm_cvttss_si32(shifted);
            if(actual<=0)return std::uint8_t{0};
            if(actual>=255)return std::uint8_t{255};
            return static_cast<std::uint8_t>(actual);
        };
        // C205F..2074 assembles A<<24|R<<16|G<<8|B. Original little-endian
        // destination bytes are B,G,R,A; no RGBA upload permutation here.
        return {quantize(rgba[2]),quantize(rgba[1]),quantize(rgba[0]),quantize(rgba[3])};
    }

    Impl(std::shared_ptr<const Initializer> owner,std::uint64_t actual_epoch,
        std::uint64_t actual_serial,std::array<std::vector<std::uint8_t>,7> rows)
        :initializer(std::move(owner)),epoch(actual_epoch),creation_serial(actual_serial),
         pixels(std::move(rows)),views{{
            {256,256,1024,pixels[0]},{128,128,512,pixels[1]},
            {64,64,256,pixels[2]},{32,32,128,pixels[3]},
            {16,16,64,pixels[4]},{8,8,32,pixels[5]},
            {4,4,16,pixels[6]}}}{}

    void first()const {
        if(creator!=std::this_thread::get_id())
            throw std::runtime_error("Original resource0 CPU image foreign creator");
        if(!initializer||!epoch||!creation_serial)
            throw std::runtime_error("Original resource0 CPU image has no genuine initializer origin");
        initializer->require_physical_without_callbacks();
        if(initializer->initialization_epoch_without_callbacks()!=epoch)
            throw std::runtime_error("Original resource0 CPU image initializer epoch changed");
        for(std::uint32_t i=0;i<7;++i){
            const auto dimension=256u>>i;
            if(views[i].width!=dimension||views[i].height!=dimension||
                views[i].row_pitch!=dimension*4u||
                views[i].bgra.data()!=pixels[i].data()||
                views[i].bgra.size()!=dimension*dimension*4u)
                throw std::runtime_error("Original resource0 immutable CPU level extent changed");
        }
        initializer->require_physical_without_callbacks();
    }
    void source(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        first();
        initializer->require_cpu_resource0_image_current_owned(operation,epoch);
        first();
        operation.require_current();
        first();
        initializer->require_cpu_resource0_image_current_owned(operation,epoch);
        first();
    }
};

BnwOriginalGeneratedResourceImageLoan::BnwOriginalGeneratedResourceImageLoan(
    std::shared_ptr<Impl> body):impl_(std::move(body)){}
BnwOriginalGeneratedResourceImageLoan::~BnwOriginalGeneratedResourceImageLoan()=default;

void BnwOriginalGeneratedResourceImageLoan::require_physical_without_callbacks()const{
    if(!impl_)throw std::runtime_error("Original resource0 CPU image payload missing");
    impl_->first();
    const auto canonical=impl_->canonical.lock();
    if(!canonical||canonical.get()!=this)
        throw std::runtime_error("Original resource0 CPU image canonical owner retired");
    impl_->first();
}
void BnwOriginalGeneratedResourceImageLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();
    held->impl_->source(operation);held->require_physical_without_callbacks();
}
std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>
BnwOriginalGeneratedResourceImageLoan::retain_owner_owned_without_callbacks()const{
    require_physical_without_callbacks();
    auto canonical=impl_->canonical.lock();
    if(!canonical||canonical.get()!=this)
        throw std::runtime_error("Original resource0 CPU image canonical retention failed");
    require_physical_without_callbacks();return canonical;
}
std::array<std::uint32_t,2>BnwOriginalGeneratedResourceImageLoan::dimensions_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();
    held->require_source_owned(operation);return {held->impl_->views[0].width,held->impl_->views[0].height};
}
std::uint32_t BnwOriginalGeneratedResourceImageLoan::original_d3d_format_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();held->require_source_owned(operation);return 21u;
}
std::uint32_t BnwOriginalGeneratedResourceImageLoan::original_level_count_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();
    held->require_source_owned(operation);return static_cast<std::uint32_t>(held->impl_->views.size());
}
std::uint64_t BnwOriginalGeneratedResourceImageLoan::original_initialization_epoch_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();held->require_source_owned(operation);return held->impl_->epoch;
}
std::uint64_t BnwOriginalGeneratedResourceImageLoan::creation_source_serial_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();
    held->require_source_owned(operation);return held->impl_->creation_serial;
}
std::span<const BnwOriginalGeneratedResourceImageLoan::Bgra8LevelView>
BnwOriginalGeneratedResourceImageLoan::bgra8_levels_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto held=retain_owner_owned_without_callbacks();held->require_source_owned(operation);return held->impl_->views;
}
std::uint32_t BnwOriginalGeneratedResourceImageLoan::row_pitch_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint32_t level)const{
    const auto held=retain_owner_owned_without_callbacks();held->require_source_owned(operation);
    if(level>=held->impl_->views.size())
        throw std::runtime_error("Original resource0 CPU level index outside actual seven levels");
    return held->impl_->views[level].row_pitch;
}

std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>
BnwOriginalGeneratedResourceImageLoan::create_resource0_owned(
    const BnwOriginalNonFontDeviceInitializer& actual_initializer,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    actual_initializer.require_physical_without_callbacks();
    const auto initializer=actual_initializer.retain_owner_owned_without_callbacks();
    if(!initializer||initializer.get()!=&actual_initializer)
        throw std::runtime_error("Original resource0 requires its SAME canonical device initializer");
    const auto canonical=initializer->retain_owner_owned_without_callbacks();
    if(!canonical||canonical.get()!=initializer.get()||
        canonical.owner_before(initializer)||initializer.owner_before(canonical))
        throw std::runtime_error("Original resource0 initializer shared control mismatch");
    initializer->require_physical_without_callbacks();
    initializer->require_cpu_resource0_creation_owned(operation);
    initializer->require_physical_without_callbacks();
    const auto epoch=initializer->initialization_epoch_without_callbacks();
    if(!epoch)
        throw std::runtime_error("Original resource0 image has no current initializer epoch/source serial");
    const auto same_epoch=[&](){
        initializer->require_physical_without_callbacks();
        if(initializer->initialization_epoch_without_callbacks()!=epoch)
            throw std::runtime_error("Original resource0 initializer changed during CPU creation");
        initializer->require_physical_without_callbacks();
    };
    same_epoch();operation.require_current();same_epoch();
    initializer->require_cpu_resource0_creation_owned(operation);same_epoch();
    const auto serial=operation.operation_serial();same_epoch();
    if(!serial)
        throw std::runtime_error("Original resource0 image has no current Source operation serial");
    const auto current=[&](){
        same_epoch();operation.require_current();same_epoch();
        initializer->require_cpu_resource0_creation_owned(operation);same_epoch();
        operation.require_operation(serial);same_epoch();
        initializer->require_cpu_resource0_creation_owned(operation);same_epoch();
    };
    current();
    std::array<std::vector<std::uint8_t>,7> rows;
    for(std::uint32_t level=0;level<7;++level){
        const auto dimension=256u>>level;
        current();rows[level].resize(dimension*dimension*4u);current();
        std::array<std::array<std::uint32_t,4>,256> actual_float_row;
        for(std::uint32_t y=0;y<dimension;++y){
            // Original D3DX evaluates the callback in ascending X first;
            // the selected row-packer then consumes odd rows in reverse.
            for(std::uint32_t x=0;x<dimension;++x)
                actual_float_row[x]=Impl::original_rgba0_callback(level,x,y);
            for(std::uint32_t iteration=0;iteration<dimension;++iteration){
                const auto x=(y&1u)?dimension-1u-iteration:iteration;
                const auto pixel=Impl::original_bgra0_pack(actual_float_row[x],y,iteration);
                const auto first=(y*dimension+x)*4u;
                for(std::uint32_t channel=0;channel<4;++channel)
                    rows[level][first+channel]=pixel[channel];
            }
        }
        current();
    }
    current();
    auto body=std::make_shared<Impl>(initializer,epoch,serial,std::move(rows));
    current();
    auto result=std::shared_ptr<const BnwOriginalGeneratedResourceImageLoan>(
        new BnwOriginalGeneratedResourceImageLoan(body));
    current();
    body->canonical=result;
    result->require_source_owned(operation);
    current();
    // Only immutable CPU bytes return here. No Original count/ID0/table slot,
    // GL upload, successful device-init return or material readiness is issued.
    return result;
}
}
