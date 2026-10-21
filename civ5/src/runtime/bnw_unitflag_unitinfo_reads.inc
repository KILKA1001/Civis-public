// Included inside actual Bootstrap::Impl. Uses SAME immutable selected rules,
// genuine full-ID native allocation and retained raw/owned provenance.
std::int32_t unit_info_type(const saves::SavedUnitObservation& unit)const {
 unit_require_current();const auto serial=operation->operation_serial();
 const saves::UnitCoreField* hash=nullptr;
 for(const auto& field:unit.core.fields)if(field.name&&std::string_view(field.name)=="idUnitType"){
  need(!hash,"UnitFlag duplicate actual UnitInfo hash");hash=&field;
 }
 need(hash&&hash->encoding==saves::UnitCoreEncoding::hash32&&hash->values.size()==1&&
  hash->values[0]>0&&hash->values[0]<=UINT32_MAX,"UnitFlag actual BNW UnitInfo hash absent");
 const auto* type=types.find_hash(static_cast<std::uint64_t>(hash->values[0]));
 need(type&&type->table=="Units"&&type->id>=0&&type->id<=INT32_MAX&&
  type->hash==content::saved_info_type_hash(type->type),"UnitFlag SAME selected UnitInfo identity unavailable");
 const auto actor=unit_scalar(unit.core,"m_eOwner"),id=unit_scalar(unit.core,"m_iID");
 const auto& owned=world->maintenance.players.at(actor).units.at(id);
 need(owned.type==type->id&&owned.source_core==std::optional{std::pair{unit.core.start_offset,unit.core.end_offset}},
  "UnitFlag UnitInfo raw/owned full-ID source differs");
 const auto result=unit_native(type->id);unit_require_current();operation->require_operation(serial);return result;
}
std::int32_t unit_info_domain(const saves::SavedUnitObservation& unit)const {
 unit_require_current();const auto serial=operation->operation_serial();const auto type=unit_info_type(unit);
 bool found=false;std::int32_t result=-1;
 need(bool(database),"UnitFlag SAME selected database missing");
 const auto report=database->query_rows({"SELECT ID,Type,Domain FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto& row){
  need(!found&&row.integer("ID")==type,"UnitFlag duplicate/different actual UnitInfo row");found=true;
  const auto name=row.text("Type"),domain=row.text("Domain");const auto* t=name?types.find(*name):nullptr;
  const auto* d=domain?types.find(*domain):nullptr;
  constexpr std::array<std::string_view,5> names{"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(t&&t->table=="Units"&&t->id==type&&d&&d->table=="Domains"&&d->id>=0&&d->id<5&&
   d->type==names.at(d->id)&&d->hash==content::saved_info_type_hash(d->type),"UnitFlag actual native domain enum differs");
  result=unit_native(d->id);
 });
 need(report.all_succeeded()&&found,"UnitFlag selected UnitInfo domain query failed");
 unit_require_current();operation->require_operation(serial);
 // SDK getDomainType: only HOVER reaches actual plot()->isWater(). Native
 // null plot returns HOVER. Missing map state is unsupported, never null.
 if(result==4){
  const auto& h=world->world.map_header;
  const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,saves::GameStateLayout::expansion2_version1};
  const auto at=geometry.plot(unit_scalar(unit.core,"m_iX"),unit_scalar(unit.core,"m_iY"));
  if(at){
   need(world->world.map_plots&&world->world.map_projection,"UnitFlag HOVER actual map owners missing");
   const auto& raw=world->world.map_plots->plots.at(*at);const auto& owned=world->world.map_projection->cells.at(*at);
   const std::remove_reference_t<decltype(raw.fields)>::value_type* field=nullptr;
   for(const auto& f:raw.fields)if(f.name&&std::string_view(f.name)=="plot_type"){
    need(!field,"UnitFlag HOVER duplicate plot kind");field=&f;
   }
   need(field&&raw.index==static_cast<std::uint64_t>(*at)&&owned.index==raw.index&&field->value>=0&&field->value<4&&
    owned.plot_type==std::optional<std::int64_t>{field->value},"UnitFlag HOVER actual raw/owned plot kind differs");
   result=field->value==3?0:2;
  }
 }
 unit_require_current();operation->require_operation(serial);return result;
}
bool unit_info_trade(const saves::SavedUnitObservation& unit)const {
 unit_require_current();const auto serial=operation->operation_serial();const auto type=unit_info_type(unit);
 bool found=false,result=false;
 need(bool(database),"UnitFlag SAME selected database missing");
 const auto report=database->query_rows({"SELECT ID,Type,Trade,typeof(Trade) AS CivisTradeStorageType FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto& row){
  need(!found&&row.integer("ID")==type,"UnitFlag duplicate/different trade UnitInfo row");found=true;
  const auto name=row.text("Type");const auto* t=name?types.find(*name):nullptr;
  const auto storage=row.text("CivisTradeStorageType");
  need(t&&t->table=="Units"&&t->id==type&&row.has_column("Trade")&&
   row.has_column("CivisTradeStorageType")&&storage&&
   (*storage=="integer"||*storage=="text"||*storage=="real"||*storage=="blob"),
   "UnitFlag actual native Trade cache source absent or NULL");
  result=row.boolean("Trade"); // Actual CvUnitEntry::CacheResults GetBool conversion.
 });
 need(report.all_succeeded()&&found,"UnitFlag selected UnitInfo Trade query failed");
 unit_require_current();operation->require_operation(serial);return result;
}
