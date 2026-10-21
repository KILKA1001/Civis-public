#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_live_player_core_export.hpp"
#include "civ5/runtime/bnw_live_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool b,const char*w){if(!b)throw std::runtime_error(w);}
template<class F>void rejects(F&&f){bool no=false;try{f();}catch(const std::runtime_error&){no=true;}check(no,"Unit serial unsupported transition accepted");}
template<class F>void boundary(F&&f,std::string_view wanted){bool no=false;try{f();}catch(const std::runtime_error&e){check(std::string_view(e.what())==wanted,"Unrelated failure masked serial restore quarantine");no=true;}check(no,"Unit serial quarantined session published");}
struct Files{std::vector<std::filesystem::path>p;~Files(){for(const auto&f:p){std::error_code e;std::filesystem::remove(f,e);}}};
void natural(const std::filesystem::path&save,const std::filesystem::path&installation){
 const auto original_file=runtime::identify_file(save);
 auto result=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 check(result.preview.host&&!result.preview.blocker,"Actual selected original BNW serial fixture unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(result.preview));
 const auto owner=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(preview,0);
 const auto original=preview.session().state().world;
 const auto physical=preview.session().read_state_lease();
 check(preview.session().source().source.container==saves::SaveContainerKind::civ5,"Serial fixture requires original Civ5 input");
 const auto old_revision=physical->ui_source_revision,generation=preview.session().state_generation();
 const auto map_rng=physical->random.map.state(),game_rng=physical->random.gameplay.state();
 const auto old_units_digest=runtime::bnw_unit_checkpoint_original_identity_digest(original);
 runtime::SessionCheckpointIdentity identity;
 preview.with_owned_host_camera_input([&](auto host,const auto*){identity=host->identity();});
 // Genuine public native SetID stores then actual abort/destruction, not a
 // diagnostic getter, supplied counter increment or invented newborn record.
 (void)owner->reserve_constructor_reset_prefix();
 (void)owner->execute_constructor_storage_tail();
 const auto assigned=owner->assign_pending_unit_native_id();
 const auto archive=owner->pending_constructor_archive();
 check(assigned.serial_after==assigned.serial_before+8192&&archive.scalar_values.at("m_iID")==assigned.full_id&&
  std::find(archive.dirty_members.begin(),archive.dirty_members.end(),"m_iID")!=archive.dirty_members.end(),"Actual SetID/native archive did not burn source serial");
 rejects([&]{(void)owner->checkpoint_native_aborted_unit_serial();}); // Pending SetID is not an aborted receipt.
 owner->discard_unpublished_constructor();
 check(archive.registered_lifetime.expired(),"Aborted native constructor survived weak cleanup");
 check(preview.session().read_state_lease()==physical&&preview.session().state_generation()==generation&&
  physical->ui_source_revision==old_revision+1&&physical->random.map.state()==map_rng&&physical->random.gameplay.state()==game_rng,
  "SetID abort replaced world, spent RNG or changed revision order");
 const auto&current=physical->world;
 check(current.all_players->players.at(0).units.header.fields.at(4).value==assigned.serial_after,"Actual abort rewound burned serial");
 const auto section=runtime::make_bnw_unit_serial_checkpoint(current,original,identity);
 check(section.id==45&&section.flags==0&&section.bytes.size()==240&&section.bytes.at(8)==1,"Serial-only canonical format/record count differs");
 rejects([&]{runtime::require_bnw_unit_serial_export_unchanged(current,original);});
 rejects([&]{(void)runtime::make_bnw_unit_scalar_checkpoint(current,original,identity);});
 rejects([&]{(void)runtime::make_bnw_live_player_core_partial_export(*physical,preview.session().source(),identity,original_file.sha256);});
 rejects([&]{(void)runtime::make_bnw_live_city_partial_export(*physical,preview.session().source(),identity,original_file.sha256);});
 rejects([&]{(void)runtime::make_bnw_unit_serial_checkpoint(original,original,identity);});
 auto malformed=current;malformed.all_players->players.at(0).units.header.fields.at(4).value=assigned.serial_after+1;
 rejects([&]{(void)runtime::make_bnw_unit_serial_checkpoint(malformed,original,identity);});
 malformed=current;malformed.all_players->players.at(0).units.header.fields.at(4).value=assigned.serial_before-8192;
 rejects([&]{(void)runtime::make_bnw_unit_serial_checkpoint(malformed,original,identity);});
 malformed=current;malformed.all_players->players.at(0).units.header.next_free_indices.at(0)=0;
 rejects([&]{(void)runtime::make_bnw_unit_serial_checkpoint(malformed,original,identity);});
 malformed=current;auto&u=malformed.all_players->players.at(0).units.units.at(0);
 for(auto&f:u.core.fields)if(f.name&&std::string_view(f.name)=="m_iID")f.values.front()+=8192;
 rejects([&]{(void)runtime::make_bnw_unit_serial_checkpoint(malformed,original,identity);});
 Files files;const auto tag=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
 const auto first=std::filesystem::temp_directory_path()/("civis-unit-serial-"+tag+"-1.CivisSave");
 const auto second=std::filesystem::temp_directory_path()/("civis-unit-serial-"+tag+"-2.CivisSave");files.p={first,second};
 auto document=owner->checkpoint_native_aborted_unit_serial();
 check(std::count_if(document.sections.begin(),document.sections.end(),[&](const auto&s){return s.id==45&&s.flags==0&&s.bytes==section.bytes;})==1,
  "Genuine owned serial exporter did not issue actual earned45 exactly once");
 saves::write_civis_save(first,document);
 const auto first_source=saves::read_loaded_save(first);
 check(first_source.live_bnw_unit_serial==section.bytes&&first_source.bytes==preview.session().source().source.bytes&&
  first_source.source_sha256==std::optional<std::string>{original_file.sha256}&&first_source.source_prefix==preview.session().source().source.source_prefix,
  "Atomic first serial checkpoint lost original source/prefix identity");
 boundary([&]{(void)saves::observe_loaded_session(first_source,{});},"Unit serial checkpoint requires canonical allocator restore before session publication");
 auto restored=original;runtime::restore_bnw_unit_serial_checkpoint(section,restored,identity);
 check(runtime::bnw_unit_checkpoint_original_identity_digest(restored)==runtime::bnw_unit_checkpoint_original_identity_digest(current),"Serial codec failed complete normalized existing-unit/FLTA roundtrip");
 rejects([&]{runtime::restore_bnw_unit_serial_checkpoint(section,restored,identity);});
 auto second_document=saves::make_civis_import_document(first_source,original_file.sha256);
 check(std::count_if(second_document.sections.begin(),second_document.sections.end(),[](const auto&s){return s.id==45;})==1,"Re-save duplicated/lost serial45");
 saves::write_civis_save(second,second_document);
 const auto second_source=saves::read_loaded_save(second);
 check(second_source.live_bnw_unit_serial==section.bytes&&second_source.bytes==first_source.bytes&&second_source.source_prefix==first_source.source_prefix&&second_source.source_sha256==first_source.source_sha256,"Second atomic serial re-save changed lineage");
 restored=original;runtime::restore_bnw_unit_serial_checkpoint({45,0,*second_source.live_bnw_unit_serial},restored,identity);
 check(runtime::bnw_unit_checkpoint_original_identity_digest(restored)==runtime::bnw_unit_checkpoint_original_identity_digest(current),"Second serial data reload differs from actual burned world");
 // Two ordinary bootstrap -> genuine canonical Core -> NEXT SetID cycles.
 // Each fresh Host independently validates rules/source and privately restores
 // physical FLTA BEFORE the same native registrar; no manual Core/data restore.
 const auto next_file=std::filesystem::temp_directory_path()/("civis-unit-serial-"+tag+"-native2.CivisSave");files.p.push_back(next_file);
 const auto next_burn=[&](const std::filesystem::path&input,const std::filesystem::path&output,
                         std::int32_t before,std::int32_t full_id){
  auto boot=ui::bootstrap_selected_bnw_preview(input,installation,0);
  if(boot.preview.blocker)throw std::runtime_error(std::string("Ordinary serial restart refused: ")+boot.preview.blocker->reason);
  check(static_cast<bool>(boot.preview.host),"Ordinary serial restart issued no actual Host");
  ui::SelectedBnwReadOnlyPreview fresh(std::move(boot.preview));
  const auto core=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(fresh,0);
  const auto state=fresh.session().read_state_lease();const auto turn_rng=state->random.gameplay.state(),map_state=state->random.map.state();
  check(state->world.all_players->players.at(0).units.header.fields.at(4).value==before,
   "Ordinary serial restart reverted actual burned counter");
  const auto reservation=core->reserve_constructor_reset_prefix();
  check(reservation.current_id==before&&reservation.reserved_id==full_id,"Actual reloaded native reservation revived burned ID");
  rejects([&]{(void)core->checkpoint_native_aborted_unit_serial();});
  (void)core->execute_constructor_storage_tail();const auto earned=core->assign_pending_unit_native_id();
  check(earned.serial_before==before&&earned.full_id==full_id&&earned.serial_after==before+8192,
   "Ordinary reloaded native SetID did not continue exact serial stream");
  const auto pending=core->pending_constructor_archive();core->discard_unpublished_constructor();
  check(pending.registered_lifetime.expired()&&state->random.gameplay.state()==turn_rng&&state->random.map.state()==map_state,
   "Reloaded native abort did not destroy real node or spent RNG");
  const auto owned=core->checkpoint_native_aborted_unit_serial();
  const auto sibling=runtime::BnwNativeUnitAllocatorCore::from_selected_preview(fresh,0);
  const auto sibling_document=sibling->checkpoint_native_aborted_unit_serial();
  check(sibling_document.sections.size()==owned.sections.size()&&std::equal(sibling_document.sections.begin(),sibling_document.sections.end(),owned.sections.begin(),
   [](const auto&a,const auto&b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}),
   "Same-session native registry lost actual serial burn history");
  if(!output.empty())saves::write_civis_save(output,owned);
  check(!fresh.session().readiness().can_continue(),"Ordinary serial restore fabricated full-world continuation");
  return earned;
 };
 const auto reloaded_second=next_burn(first,next_file,assigned.serial_after,assigned.full_id+8192);
 (void)next_burn(next_file,{},reloaded_second.serial_after,reloaded_second.full_id+8192);
 std::cout<<"PASS TWO_ORDINARY_SERIAL45_SAVE_RELOAD_NEXT_SETID_CYCLES actual canonical Bank and preserved burned full IDs\n";
 const auto rejection=[&](const saves::CivisSaveSection&bad){auto w=original;rejects([&]{runtime::restore_bnw_unit_serial_checkpoint(bad,w,identity);});check(runtime::bnw_unit_checkpoint_original_identity_digest(w)==old_units_digest,"Rejected serial restore partially published unit state");};
 auto bad=section;bad.flags=1;rejection(bad);bad=section;bad.bytes.at(8)=2;rejection(bad);bad=section;bad.bytes.pop_back();rejection(bad);
 bad=section;bad.bytes.at(12)=bad.bytes.at(12)=='0'?static_cast<std::uint8_t>('1'):static_cast<std::uint8_t>('0');rejection(bad);
 bad=section;bad.bytes.at(76)=bad.bytes.at(76)=='0'?static_cast<std::uint8_t>('1'):static_cast<std::uint8_t>('0');rejection(bad);
 bad=section;bad.bytes.at(140)=bad.bytes.at(140)=='0'?static_cast<std::uint8_t>('1'):static_cast<std::uint8_t>('0');rejection(bad);
 bad=section;bad.bytes.at(204)^=1;rejection(bad);
 bad=section;bad.bytes.at(220)=4;rejection(bad); // Unknown wrap flag.
 bad=section;bad.bytes.at(224)=0;rejection(bad); // Empty sparse state forbidden.
 bad=section;bad.bytes.at(228)=64;rejection(bad); // Foreign actor.
 bad=section;for(std::size_t i=236;i<240;++i)bad.bytes[i]=0xff;rejection(bad); // No signed serial wrap.
 bad=section;std::copy_n(bad.bytes.begin()+232,4,bad.bytes.begin()+236);rejection(bad); // No-op/regression.
 bad=section;bad.bytes.push_back(0);rejection(bad);
 auto wrong_world=original;wrong_world.map_header.wrap_x=!wrong_world.map_header.wrap_x;
 const auto wrong_world_stamp=runtime::bnw_unit_checkpoint_original_identity_digest(wrong_world);
 rejects([&]{runtime::restore_bnw_unit_serial_checkpoint(section,wrong_world,identity);});
 check(runtime::bnw_unit_checkpoint_original_identity_digest(wrong_world)==wrong_world_stamp,"Wrong-wrap refusal changed unit state");
 auto foreign=identity;foreign.rules_sha256=std::string(64,'a');rejects([&]{auto w=original;runtime::restore_bnw_unit_serial_checkpoint(section,w,foreign);});
 // Refusal before an output exists; no uniqueness filtering or original overwrite.
 const auto negative=std::filesystem::temp_directory_path()/("civis-unit-serial-"+tag+"-bad.CivisSave");files.p.push_back(negative);
 auto duplicate=document;duplicate.sections.push_back(section);rejects([&]{saves::write_civis_save(negative,duplicate);});check(!std::filesystem::exists(negative),"Duplicate serial checkpoint published output");
 auto invalid=document;for(auto&s:invalid.sections)if(s.id==45)s.flags=1;rejects([&]{saves::write_civis_save(negative,invalid);});check(!std::filesystem::exists(negative),"Malformed serial checkpoint published output");
 auto mixed=document;mixed.sections.push_back({18,0,{1}});mixed.sections.push_back({17,0,{1}});
 rejects([&]{saves::write_civis_save(negative,mixed);});check(!std::filesystem::exists(negative),"Uncoordinated serial/ID18 export published output");
 rejects([&]{saves::write_civis_save(save,document);});check(runtime::identify_file(save).sha256==original_file.sha256,"Serial checkpoint overwrote original save");
 check(!preview.session().readiness().can_continue(),"Partial serial checkpoint manufactured continuation readiness");
 std::cout<<"PASS genuine SETID_ABORT_TWO_ATOMIC_SERIAL_DATA_CYCLES; partial checkpoint only\n";
}
}
int main(int argc,char**argv){try{check(argc==3,"Use original BNW save and canonical installation");natural(argv[1],argv[2]);return 0;}catch(const std::exception&e){std::cerr<<"FAIL unit serial checkpoint: "<<e.what()<<'\n';return 1;}}
