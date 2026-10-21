// SAME selected Core/Bank node and immutable RulesDatabase. Original UnitInfo
// cache methods use actual typed rows; no published-roster unit is fabricated.
namespace civ5::runtime {
std::int32_t BnwPendingCreatedUnitRead::read_domain_without_callbacks()const{
 require_returned_without_callbacks();const auto node=storage_->node.lock();need(bool(node),"Pending actual Domain node expired");
 const auto&host=*storage_->binding.host;const auto type=node->scalars.at("m_eUnitType");
 const auto&info=initial_maintenance_info(host,"Units",type);
 const auto query=[&]{
  std::optional<std::int32_t>answer;
  const auto report=host.database().query_rows({"SELECT ID,Type,Domain FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){
   need(!answer&&r.integer("ID")==type&&r.text("Type")==std::optional<std::string>{info.type},
    "Pending created actual UnitInfo Domain identity changed");
   const auto key=r.text("Domain");const auto*d=key?host.types().find(*key):nullptr;
   constexpr std::array<std::string_view,5>names={"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
   need(d&&d->table=="Domains"&&d->id>=0&&d->id<5&&d->type==names.at(static_cast<std::size_t>(d->id))&&
    d->hash==content::saved_info_type_hash(d->type),"Pending created actual native Domain enum unavailable");answer=narrow(d->id);
  });need(report.all_succeeded()&&answer,"Pending created actual Domain query failed");return *answer;
 };
 auto result=query();require_returned_without_callbacks();const auto second=query();require_returned_without_callbacks();
 need(result==second,"Pending created actual Domain changed across bookends");
 if(result==4){
  // Native HOVER is resolved from the actual pending Unit.GetPlot. Its
  // completed setup binding proves an existing canonical physical plot.
  const auto&world=*storage_->binding.state;const auto plot=storage_->binding.plot;
  need(world.world.map_plots&&world.world.map_plots->layout==bnw&&world.world.map_projection&&
   plot<world.world.map_plots->plots.size()&&plot<world.world.map_projection->cells.size(),
   "Pending HOVER actual plot provider unavailable");
  const auto&raw=world.world.map_plots->plots.at(plot);const auto&owned=world.world.map_projection->cells.at(plot);
  const auto kind=post_named(raw.fields,"plot_type").value;
  need(raw.index==plot&&owned.index==plot&&kind>=0&&kind<4&&owned.plot_type==std::optional<std::int64_t>{kind},
   "Pending HOVER actual raw/owned plot kind differs");result=kind==3?0:2;
 }
 require_returned_without_callbacks();return result;
}
bool BnwPendingCreatedUnitRead::read_trade_without_callbacks()const{
 require_returned_without_callbacks();const auto node=storage_->node.lock();need(bool(node),"Pending actual Trade node expired");
 const auto&host=*storage_->binding.host;const auto type=node->scalars.at("m_eUnitType");
 const auto&info=initial_maintenance_info(host,"Units",type);
 const auto query=[&]{
  std::optional<bool>answer;
  const auto report=host.database().query_rows({"SELECT ID,Type,Trade,typeof(Trade) AS TradeStorageType FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){
   need(!answer&&r.integer("ID")==type&&r.text("Type")==std::optional<std::string>{info.type}&&r.has_column("Trade")&&
    r.text("TradeStorageType")==std::optional<std::string>{"integer"},
    "Pending created actual UnitInfo Trade identity/source changed");answer=r.integer("Trade")!=0;
  });need(report.all_succeeded()&&answer.has_value(),"Pending created actual Trade query failed");return *answer;
 };
 const bool result=query();require_returned_without_callbacks();const bool second=query();require_returned_without_callbacks();
 need(result==second,"Pending created actual Trade changed across bookends");return result;
}
}
