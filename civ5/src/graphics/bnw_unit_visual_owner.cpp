#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <charconv>
#include <limits>
#include <set>
#include <string_view>
namespace civ5::graphics {
namespace {
std::string_view trim(std::string_view value) {
    const auto start=value.find_first_not_of(" \t\r\n");
    if(start==std::string_view::npos)return {};
    return value.substr(start,value.find_last_not_of(" \t\r\n")-start+1);
}
std::int64_t scalar(const saves::SavedUnitObservation &unit,std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)if(field.name && field.name==name){
        if(found)throw std::invalid_argument("Duplicate visual unit field");found=&field;
    }
    if(!found||found->values.size()!=1)throw std::invalid_argument("Missing visual unit field: "+std::string(name));
    return found->values[0];
}
std::vector<BnwImportedUnitVisualRecord> roster(const saves::SavedSessionSnapshot &world) {
    if(!world.all_players||world.all_players->layout!=saves::GameStateLayout::expansion2_version1)
        throw std::invalid_argument("Unit renderer requires complete BNW player roster");
    if(world.all_players->players.size()!=world.all_players->context.player_count)throw std::invalid_argument("Visual roster is not complete");
    std::vector<BnwImportedUnitVisualRecord> records;
    std::set<std::pair<std::int64_t,std::int64_t>> identities;
    std::optional<std::uint64_t> prior_player;
    std::uint64_t player_ordinal=0;
    for(const auto &player:world.all_players->players) {
        if(player.index!=player_ordinal++)throw std::invalid_argument("Missing visual player slot");
        if(prior_player&&player.index<=*prior_player)throw std::invalid_argument("Visual players not in source order");
        prior_player=player.index;
        std::optional<std::uint64_t> prior_unit;
        for(const auto &unit:player.units.units) {
            if(unit.core.layout!=saves::GameStateLayout::expansion2_version1 ||
               (prior_unit&&unit.index<=*prior_unit))throw std::invalid_argument("Visual units not in BNW source order");
            prior_unit=unit.index;
            const auto owner=scalar(unit,"m_eOwner"),id=scalar(unit,"m_iID"),embarked=scalar(unit,"m_bEmbarked");
            if(owner!=player.saved_player_id || id<0 || (embarked!=0&&embarked!=1) ||
               !identities.emplace(owner,id).second || records.size()>=100000)
                throw std::invalid_argument("Invalid visual unit identity/roster extent");
            records.push_back({owner,id,scalar(unit,"m_eUnitType"),scalar(unit,"m_iX"),scalar(unit,"m_iY"),
                scalar(unit,"m_eFacingDirection"),embarked!=0,static_cast<std::uint32_t>(records.size())});
        }
    }
    return records;
}
}
BnwUnitVisualConfiguration read_bnw_unit_visual_configuration(const assets::AssetHandle &handle) {
    auto bytes=handle.read_bytes(65536);
    const std::string text(reinterpret_cast<const char *>(bytes.data()),bytes.size());
    std::map<std::string,std::string> values;std::string section;
    for(std::size_t start=0;start<text.size();) {
        const auto end=text.find('\n',start);auto line=trim(std::string_view(text).substr(start,end==std::string::npos?end:end-start));
        start=end==std::string::npos?text.size():end+1;
        if(line.empty()||line[0]==';')continue;
        if(line.front()=='['&&line.back()==']'){section=std::string(line.substr(1,line.size()-2));continue;}
        if(section!="Debugging")continue;
        const auto equal=line.find('=');if(equal==std::string_view::npos)throw std::invalid_argument("Malformed visual INI");
        if(!values.emplace(std::string(trim(line.substr(0,equal))),std::string(trim(line.substr(equal+1)))).second)
            throw std::invalid_argument("Duplicate visual INI key");
    }
    auto number=[&](const char *key) {
        const auto value=values.find(key);if(value==values.end())throw std::invalid_argument("Missing visual INI key");
        double n=0;const auto result=std::from_chars(value->second.data(),value->second.data()+value->second.size(),n);
        if(result.ec!=std::errc{}||result.ptr!=value->second.data()+value->second.size()||!std::isfinite(n))
            throw std::invalid_argument("Malformed visual INI number");
        return n;
    };
    const auto scale=number("GlobalUnitScale"),members=number("GlobalUnitMemberMultiplier"),seed=number("GlobalUnitPlacementSeed"),thread=number("UnitRenderMultiThreaded");
    if(scale<=0||scale>10000||members!=100||seed!=-1||(thread!=0&&thread!=1))
        throw std::invalid_argument("Unsupported visual configuration for owned Warrior branch");
    // Native SSE float percent conversion. Source config is retained, no default.
    return {static_cast<float>(scale)*.01F,static_cast<float>(members)*.01F,
        static_cast<std::int32_t>(seed),thread!=0,runtime::identify_bytes(bytes).sha256,handle.candidate().provenance};
}
BnwImportedUnitVisualOwner::BnwImportedUnitVisualOwner(const saves::SavedSessionSnapshot &world,
    BnwUnitVisualConfiguration configuration,std::int64_t warrior_type):configuration_(std::move(configuration)),warrior_type_(warrior_type),records_(roster(world)) {
    if(warrior_type_<0||!std::isfinite(configuration_.scene_spacing)||configuration_.scene_spacing<=0||configuration_.member_multiplier!=1||
       configuration_.placement_seed!=-1||configuration_.config_sha256.empty())throw std::invalid_argument("Missing owned visual configuration");
    for(std::size_t i=0;i<records_.size();++i)identity_.emplace(std::pair{records_[i].owner,records_[i].id},i);
}
const BnwImportedUnitVisualRecord &BnwImportedUnitVisualOwner::find(std::int64_t player,std::int64_t id) const {
    const auto at=identity_.find({player,id});if(at==identity_.end())throw std::invalid_argument("Unit absent from owned visual roster");
    return records_[at->second];
}
bool BnwImportedUnitVisualOwner::matches(const saves::SavedSessionSnapshot &world) const {return records_==roster(world);}
BnwWarriorFormation BnwImportedUnitVisualOwner::warrior_formation(std::int64_t player,std::int64_t id) const {
    const auto &unit=find(player,id);if(unit.type!=warrior_type_)throw std::invalid_argument("Unit does not resolve to Warrior profile");if(unit.embarked)throw std::invalid_argument("Embarked Warrior needs its own model route");
    return bnw_warrior_authored_formation(unit.pool_index,configuration_.scene_spacing);
}
float bnw_unit_world_height(const terrain::HeightOwnerState &owner,float x,float y,bool wrap_x) {
    const auto &p=owner.parameters;
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(p.scale_x)||!std::isfinite(p.scale_y)||
       p.width<=0||p.height<=0||p.scale_x<=0||p.scale_y<=0||!std::isfinite(p.scale_z)||!std::isfinite(p.center_x)||!std::isfinite(p.center_y)||owner.height.element_bytes()!=2)throw std::invalid_argument("Invalid unit height coordinates");
    // Match the actual PageStorage producer's 16-sample physical domain.
    // Reject a foreign-size store before conversion, sampling or zero fallback.
    {
        constexpr auto max=std::numeric_limits<std::int32_t>::max();
        const auto &physical=owner.height.domain();
        if(p.width>max-15||p.height>max-15||
           physical.width()!=((p.width+15)&-16)||physical.height()!=((p.height+15)&-16))
            throw std::invalid_argument("Unit height page domain does not match owner parameters");
    }
    const float fx=(x+p.center_x)/p.scale_x,fy=(y+p.center_y)/p.scale_y;
    if(!std::isfinite(fx)||!std::isfinite(fy)||fx<-2147483648.0F||fx>=2147483648.0F||fy<-2147483648.0F||fy>=2147483648.0F)
        throw std::invalid_argument("Unrepresentable unit height coordinate");
    auto ix=static_cast<std::int32_t>(fx);const auto iy=static_cast<std::int32_t>(fy);
    const auto &domain=owner.height.domain();
    if(wrap_x)ix=terrain::wrap_add(terrain::wrap_mul(p.width,100),ix)%p.width;
    if(ix<0||iy<0||ix>=domain.width()||iy>=domain.height())return 0;
    const auto raw=owner.height.value_at(ix,iy);
    return (static_cast<float>(raw)*0.003921568859368563F)*p.scale_z;
}
BnwWarriorFormationPoint bnw_unit_world_center(const terrain::WorldLayout &world,
    const terrain::HeightOwnerParameters &owner,std::int32_t x,std::int32_t y) {
    const auto projection=terrain::terrain_ice_projection(world,owner);
    if(x<0||y<0||x>=world.source_grid.width||y>=world.source_grid.height)throw std::invalid_argument("Unit outside source map");
    const terrain::LogicalHex source{terrain::wrap_add(x,-(y>>1)),y,terrain::wrap_neg(terrain::wrap_add(terrain::wrap_add(x,-(y>>1)),y))};
    const auto &m=world.source_grid.second_matrix;
    const auto component=[&](std::size_t c){return terrain::wrap_add(terrain::wrap_add(terrain::wrap_add(terrain::wrap_mul(m[8+c],source.z),terrain::wrap_mul(m[c],source.x)),terrain::wrap_mul(m[4+c],source.y)),m[12+c]);};
    const auto cx=component(0),cy=component(1);
    const terrain::Vector4 v{static_cast<float>(cx),static_cast<float>(cy),static_cast<float>(terrain::wrap_neg(terrain::wrap_add(cx,cy))),1};
    const auto projected=terrain::vector4_transform(v,projection.world_matrix,terrain::X87Precision::double_precision,terrain::Vec4Path::sse);
    return {projected[0]*projection.scale_x-projection.center_x,projected[1]*projection.scale_y-projection.center_y};
}
std::array<std::array<float,16>,12> bnw_warrior_owned_scene_transforms(
    const BnwImportedUnitVisualOwner &units,const saves::SavedSessionSnapshot &current,
    std::int64_t player,std::int64_t id,const terrain::WorldLayout &world,
    const terrain::HeightOwnerState &height,bool wrap_x,float minimum_height,float member_scale) {
    if(!units.matches(current)||!std::isfinite(minimum_height))throw std::invalid_argument("Unit visual roster/revision mismatch");
    const auto &unit=units.find(player,id);
    if(unit.x<0||unit.x>INT32_MAX||unit.y<0||unit.y>INT32_MAX||unit.facing<0||unit.facing>=6)
        throw std::invalid_argument("Invalid selected unit coordinates/facing");
    const auto center=bnw_unit_world_center(world,height.parameters,static_cast<std::int32_t>(unit.x),static_cast<std::int32_t>(unit.y));
    const auto formation=units.warrior_formation(player,id);
    BnwWarriorWorldTransformInput input;input.center_x=center.x;input.center_y=center.y;
    input.heading_radians=bnw_warrior_initial_heading(static_cast<std::int32_t>(unit.facing));input.model_scale=member_scale;
    const auto c=std::cos(input.heading_radians),s=std::sin(input.heading_radians);
    for(std::size_t i=0;i<12;++i){const auto p=formation.members[i];input.member_height[i]=std::fmax(minimum_height,
        bnw_unit_world_height(height,p.x*c-p.y*s+center.x,p.x*s+p.y*c+center.y,wrap_x));}
    return bnw_warrior_render_transforms(formation,input);
}
} // namespace civ5::graphics
