// Included in the existing anonymous namespace before Attrition Impl.
// Only SAME Core-issued parent stores this privately retained child.
struct AttritionDamageChild {
 std::uint64_t previous_serial=0,serial=0,before=0,after=0;
 bool mountain=false,localization_entered=false,localization_returned=false;
 bool prepared=false,attempted=false,executed=false,failed=false;
 bool last_condition=false,last_written=false,assignment_invoked=false,changed=false;
 std::int32_t id=-1,old_damage=0,old_loaded=0,delta=0,requested=0,max_hp=0,new_damage=0;
 std::int32_t old_last=-1,new_last=-1;
 std::uint64_t old_archive_changes=0,old_last_stores=0;
 bool old_archive_dirty=false;
 std::shared_ptr<ImportedDamageArchive>archive;
 std::shared_ptr<ImportedMissionRuntime>plain;
 std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 std::string localized_text;
 BusyUnitStamp after_unit;
};
std::int32_t attrition_actual_max_hit_points(const ui::SelectedBnwPreviewHost&host){
 const auto*define=host.defines().find("MAX_HIT_POINTS");
 need(host.defines().profile()==content::GlobalDefineProfile::bnw&&define&&
      define->kind==content::GlobalDefineKind::integer&&define->lookup.found&&
      define->lookup.sqlite_code==0&&define->lookup.error.empty()&&!define->retained_previous&&
      std::holds_alternative<std::int64_t>(define->lookup.value),
      "Attrition damage actual MAX_HIT_POINTS source absent");
 const auto value=narrow(std::get<std::int64_t>(define->lookup.value));
 need(value>0,"Attrition damage MAX_HIT_POINTS cannot admit native HP range");return value;
}
