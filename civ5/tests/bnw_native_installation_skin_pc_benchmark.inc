// Test-only continuation of the SAME real benchmark after endpoint52D590.
// Include inside its anonymous namespace, after need/Window and before runner.
// Existing timing/provider/prelude stay in the caller; no second Parent owner.
template<class Method,class Value,class RequireClient>
ui::BnwNativePackageDiscoveryFrame run_installed_skin_pc_benchmark_tail(
 const std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner>& parent,
 const ui::BnwNativePackageDiscoveryFrame& endpoint52,
 const Method& method,const Value& value,const RequireClient& require_client){
 need(bool(parent),"Skin/PC benchmark lost its actual endpoint52D590 Parent");
 const auto observe=[&](const char* timing,std::string_view expected){
  return value(timing,[&]{
   const auto actual=parent->current();
   need(actual.packages==endpoint52.packages&&actual.original_descriptor_order==endpoint52.original_descriptor_order&&
    actual.primary_dx9_registration_complete&&actual.common_high_quality_registration_complete&&
    actual.low_resolution_source_branch_complete&&actual.high_texture_quality_source_branch&&
    !actual.low_resource_mount_present&&!actual.native_skin_loading_complete,
    "Skin/PC benchmark changed genuine installed source lineage or claimed complete Skin");
   if(actual.next_resource_dependency!=expected)
    throw std::runtime_error(std::string("Skin/PC benchmark unexpected actual frontier at ")+timing+
     "; expected="+std::string(expected)+"; actual="+actual.next_resource_dependency);
   require_client();return actual;
  });
 };
 method("benchmark.method.Source7.initial52D590",[&]{parent->drive_installation_initial_color_skin();});
 (void)observe("benchmark.verify.Source7.return",
  "Actual initial52D590 base Color/48C800/462C20 returned; genuine later common/PC660330 and global selection/completed startup suppliers mandatory");
 method("benchmark.method.Source8a.initial628BE0",[&]{parent->advance_installation_initial_skin_selection_prefix();});
 (void)observe("benchmark.verify.Source8a.firstCommonPending",
  "Actual628BE0 zero gate and set=BaseGame/platform=Common returned; mandatory SAME pending first660330(-1) before PC/global/late Default tail");
 method("benchmark.method.Source8b.firstCommon660330",[&]{parent->drive_installation_first_common_skin();});
 (void)observe("benchmark.verify.Source8b.return",
  "Actual first Common660330(-1)/462C20 returned; genuine next6415F0 PC publication, second660330,4691E0 and late Default tail remain mandatory");
 method("benchmark.method.Source8c.PC6415F0",[&]{parent->advance_installation_pc_publication();});
 // Public production return already authenticates the SAME canonical policy's
 // real PC copy/phase and closed continuation. No new field getter/receipt.
 (void)observe("benchmark.verify.endpointPC6415F0",
  "Actual6415F0(PC) text publication returned; SAME pending778E40(platform,PC) then second660330, constraint clear,4691E0 and late Default tail remain mandatory");
 method("benchmark.method.Source8d.PCsecond660330",[&]{parent->drive_installation_pc_skin();});
 // The real new resource owner has completed physical POST and retired both
 // continuations before this public return. Actual constraint clear is next.
 (void)observe("benchmark.verify.endpointPCsecond660330",
  "Actual778E40(platform,PC)/second660330/462C20 returned; SAME pending6B9530 constraint clear then4691E0/global and late Default tail remain mandatory");
 method("benchmark.method.Source8e.initial6B9530close",[&]{parent->close_installation_initial_constraints();});
 // Actual entries clear, unique header release, full POST and new control
 // continuation teardown precede this genuine public normal return.
 return observe("benchmark.verify.endpointInitial6B9530closed",
  "Actual initial6B9530 entries clear and owned caller header release returned; SAME pending4691E0(-1) with NEW per-slot constraint/index owners before late Default/terminal suppliers");
}
