// Included OUTSIDE namespaces at the end of the existing city-strength TU.
#include "civ5/runtime/bnw_attrition_garrison_child.hpp"
#include "civ5/runtime/map_geometry.hpp"
namespace civ5::runtime {
namespace {
struct AttritionGarrisonSemantic {
 BnwAttritionGarrisonFacts facts;const saves::SavedCityObservation*city=nullptr;
 const saves::PlotObservation*plot=nullptr;bool best_required=false;
};
AttritionGarrisonSemantic attrition_garrison_semantic(const LiveSessionState&s,
 std::int32_t actor,std::int32_t id,auto&seal){
 AttritionGarrisonSemantic q;const auto*u=plot_unit(s,actor,id);
 need(u&&unit_scalar(*u,"m_iID")==id&&unit_scalar(*u,"m_eOwner")==actor,
      "Attrition garrison SAME actual imported fullID unavailable");
 put(seal,reinterpret_cast<std::uintptr_t>(u));seal.push_back(actor);seal.push_back(id);
 // Explicit operand join to the physically changed live row, not original payload.
 q.facts.observed_damage=unit_scalar(*u,"m_iDamage");seal.push_back(q.facts.observed_damage);
 const InitialDefenderView native{u,nullptr,nullptr,&s,actor,id,{}};
 // BNW Read stores MapLayer in the real plain tail.state, not core/VP.
 q.facts.layer=native.scalar("m_iMapLayer");seal.push_back(q.facts.layer);
 if(q.facts.layer!=0)return q; // Original no plot/city lookup for other layers.
 const auto x=unit_scalar(*u,"m_iX"),y=unit_scalar(*u,"m_iY");seal.push_back(x);seal.push_back(y);
 const MapGeometry geometry{s.world.map_header.grid,s.world.map_header.wrap_x,s.world.map_header.wrap_y,bnw};
 q.facts.plot=geometry.plot(x,y);seal.push_back(q.facts.plot.has_value());
 if(!q.facts.plot)return q; // Original plot() == NULL.
 const auto index=*q.facts.plot;put(seal,index);
 need(s.world.map_plots&&s.world.map_projection&&index<s.world.map_plots->plots.size()&&
      index<s.world.map_projection->cells.size(),"Attrition garrison actual map storage missing");
 const auto&plot=s.world.map_plots->plots.at(index);const auto&cell=s.world.map_projection->cells.at(index);
 q.plot=&plot;q.facts.plot_returned=true;
 put(seal,reinterpret_cast<std::uintptr_t>(&plot));put(seal,reinterpret_cast<std::uintptr_t>(&cell));
 put(seal,plot.index);put(seal,plot.x);put(seal,plot.y);put(seal,cell.index);put(seal,cell.x);put(seal,cell.y);
 const auto owner=integer(named(plot.fields,"plot_city_owner").value),city=integer(named(plot.fields,"plot_city_id").value);
 need(owner>=-1&&owner<64&&cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==city,
      "Attrition garrison genuine plot-city IDInfo mirror unavailable");
 q.facts.city_owner=owner;q.facts.city_id=city;seal.push_back(owner);seal.push_back(city);
 // getCity(IDInfo): NO_PLAYER skips GetAt; stale or -1 city returns NULL.
 if(owner!=-1)q.city=resolve_bnw_city_registry(s.world,s.cities,owner,city);
 q.facts.city_returned=q.city!=nullptr;put(seal,reinterpret_cast<std::uintptr_t>(q.city));
 if(!q.city)return q;
 q.facts.city_id=city_scalar(*q.city,"m_iID");seal.push_back(*q.facts.city_id);
 const auto plot_owner=integer(named(plot.fields,"owner").value);
 need(plot_owner>=-1&&plot_owner<64&&cell.owner==std::optional<std::int64_t>{plot_owner},
      "Attrition garrison genuine plot owner mirror unavailable");
 q.facts.plot_owner=plot_owner;seal.push_back(plot_owner);
 if(plot_owner!=actor)return q;
 need(owner==actor,"Attrition garrison own plot/city owner identity differs");
 // Includes actual post-damage imported roster and all reached comparator
 // closure. No cached m_iGarrisonCityID/m_hGarrison determines this result.
 initial_city_inputs(s,owner,*q.facts.city_id,seal);q.best_required=true;return q;
}
}
struct BnwNativeImportedGarrisonRead::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 const ui::SelectedBnwPreviewHost*host=nullptr;std::weak_ptr<const LiveSessionState>world;
 std::int32_t actor=-1,id=-1;std::uint64_t generation=0,revision=0;
 Seal inputs;AttritionGarrisonSemantic reached;SimulationRandomState rng,map_rng;
};
BnwNativeImportedGarrisonRead::BnwNativeImportedGarrisonRead(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwNativeImportedGarrisonRead::~BnwNativeImportedGarrisonRead()=default;
std::shared_ptr<const BnwNativeImportedGarrisonRead>BnwNativeImportedGarrisonRead::read_same_live(
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,std::int32_t actor,std::int32_t id){
 const auto world=host.session().read_state_lease();
 need(world&&world.get()==&s,"Attrition garrison supplier requires genuine SAME Host world");
 auto p=std::make_shared<Impl>();p->host=&host;p->world=world;p->actor=actor;p->id=id;
 p->generation=host.session().state_generation();p->revision=s.ui_source_revision;
 p->rng=s.random.gameplay.state();p->map_rng=s.random.map.state();
 InputCount count;const auto counted=attrition_garrison_semantic(s,actor,id,count);(void)counted;
 p->inputs.reserve(count.size());p->reached=attrition_garrison_semantic(s,actor,id,p->inputs);
 need(p->inputs.size()==count.size(),"Attrition garrison source changed while reserving native seal");
 if(p->reached.best_required){
  const std::function<InitialDefenderView(std::int32_t,std::int32_t)>resolve=[&](std::int32_t owner,std::int32_t handle){
   const auto*u=plot_unit(s,owner,handle);return InitialDefenderView{u,nullptr,nullptr,&s,owner,u?unit_scalar(*u,"m_iID"):-1,{}};
  };
  const auto best=initial_selected_defender(s,host,actor,*p->reached.plot,resolve);
  if(best.raw)p->reached.facts.best_full_id=unit_scalar(*best.raw,"m_iID");
  p->reached.facts.garrisoned=p->reached.facts.best_full_id==std::optional{id};
 }
 const auto issued=std::shared_ptr<const BnwNativeImportedGarrisonRead>(new BnwNativeImportedGarrisonRead(p));
 issued->require_without_callbacks(host,s);return issued;
}
void BnwNativeImportedGarrisonRead::require_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->host==&host,
      "Attrition garrison read foreign creator/Host");
 const auto world=p->world.lock();const auto current=host.session().read_state_lease();
 need(world&&world.get()==&s&&current.get()==world.get()&&!current.owner_before(world)&&!world.owner_before(current)&&
      host.session().state_generation()==p->generation&&s.ui_source_revision==p->revision&&
      s.random.gameplay.state()==p->rng&&s.random.map.state()==p->map_rng,
      "Attrition garrison native read world/generation/revision/RNG changed");
 InputMatch match{p->inputs};const auto reached=attrition_garrison_semantic(s,p->actor,p->id,match);
 need(match.equal&&match.position==p->inputs.size()&&reached.city==p->reached.city&&
      reached.plot==p->reached.plot&&reached.best_required==p->reached.best_required,
      "Attrition garrison retained native city/plot/comparator inputs changed");
}
BnwAttritionGarrisonFacts BnwNativeImportedGarrisonRead::facts_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Attrition garrison read foreign thread");return impl_->reached.facts;
}
const void*BnwNativeImportedGarrisonRead::city_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Attrition garrison read foreign thread");
 return impl_->reached.facts.garrisoned?impl_->reached.city:nullptr;
}
}
