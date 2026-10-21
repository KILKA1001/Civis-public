#include "civ5/ui/bnw_slider_graphic_cache.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <cmath>
#include <iostream>
using namespace civ5;
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Unowned/missing Graphic dependency silently accepted");}
int main(int argc,char **argv){
 check(argc==2,"Explicit shared source root required; source dependency test, not gameplay epoch");const std::filesystem::path root(argv[1]);
 auto loose=std::make_shared<assets::LooseResourceIndex>(root);loose->add_file("Assets/UI/Styles.xml");assets::ResourceMountTable resources(assets::resource_lookup(loose));
 const assets::AssetEnvironment assets_root(root);std::uint64_t id=1;
 for(const auto name:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){const auto opened=assets_root.open_fpk_archive(name);const auto *fpk=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);check(fpk,"Actual shared UI archive unavailable");resources.mount({id++,0,0,assets::resource_lookup(*fpk),true,true,{},{}});}
 runtime::BnwStockLuaClientExtent client{"source-graphic-dependency",4,840,620};
 const auto xml_id=runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml");const auto shell=ui::BnwCivilopediaScrollBarOwner::create(root,resources,client,xml_id.sha256);
 const auto owner=ui::BnwSliderGraphicCacheOwner::create(root,resources,shell->render_snapshot());const auto &frame=owner->snapshot();
 const auto *track=owner->graphic(client,0,"track"),*shuttle=owner->graphic(client,0,"shuttle");check(track==&frame.track&&shuttle==&frame.shuttle&&!frame.native_slider_setter_complete&&!track->native_base_initialization_complete&&!shuttle->native_base_initialization_complete&&track->dirty_cache_branch_evaluated&&shuttle->dirty_cache_branch_evaluated,"Graphic shared frame falsely authorizes complete native setter");
 check(track->size.x==18&&track->size.y==800&&track->authored_sizes[0].y==6&&track->extents[0].y==800&&track->source_ends[0].y==12&&track->source_starts[0].y==6,"Authored source sizes were overwritten by stretched geometry cache");
 check(shuttle->size.y==18&&shuttle->extents[0].y==12&&shuttle->positions[0].y==3&&shuttle->positions[2].y==15&&shuttle->source_ends[0].y==4,"Shuttle caps/native geometry source incorrect");
 check(track->texture->source_identity.provenance.kind==assets::SourceKind::fpk&&shuttle->texture->source_identity.provenance.kind==assets::SourceKind::fpk&&track->texture->source_mount_id==3&&shuttle->texture->source_mount_id==3,"Skin assets copied/extracted/faked or wrong scoped mount selection");
 check(runtime::identify_bytes(track->texture->source.original_bytes()).sha256=="95416193a37f6d7c9ab71f173b42ade67ef9b38cae177cf917d4505955921e6d"&&runtime::identify_bytes(shuttle->texture->source.original_bytes()).sha256=="dadd1cc84c0484286eb43ee35399695fd34ff464e8420fc4455da74f2dc1d11d"&&track->texture->width()==32&&track->texture->height()==32&&shuttle->texture->width()==32&&shuttle->texture->height()==128&&!track->texture->index&&!shuttle->texture->index,"Actual current FpkSource COLOR/plain pins differ; stale inventory offsets are not acceptance evidence");
 check(shuttle->uv_spans[0].y==1.0f/static_cast<float>(shuttle->texture->height())&&shuttle->uv_starts[0].y==3.0f/static_cast<float>(shuttle->texture->height()),"UV uses stretched height or physical packed DDS dimensions");
 auto next=owner->fork_track_cache(client,0,18,448);check(next->snapshot().track.size.y==448&&next->snapshot().track.extents[0].y==448&&next->snapshot().graphic_revision==1&&frame.track.size.y==800&&next->snapshot().track.texture==frame.track.texture&&next->snapshot().track.uv_spans[0].y==frame.track.uv_spans[0].y,"Detached Graphic update mutated original source/texture/UV cache");
 auto stale=client;++stale.revision;check(!owner->graphic(stale,0,"track")&&!owner->graphic(client,1,"track")&&!owner->graphic(client,0,"other"),"Stale graphic identity accepted");rejects([&]{owner->fork_track_cache(stale,0,18,448);});rejects([&]{owner->fork_track_cache(client,0,19,448);});
 assets::ResourceMountTable missing(assets::resource_lookup(loose));rejects([&]{ui::BnwSliderGraphicCacheOwner::create(root,missing,shell->render_snapshot());});auto changed=shell->render_snapshot();changed.styles_sha256=std::string(64,'0');rejects([&]{ui::BnwSliderGraphicCacheOwner::create(root,resources,changed);});
 std::cout<<"PASS: actual shared FPK track/shuttle skins, logical DDS dimensions, native dirty geometry/source UV ends, detached cache/stale guards; native Slider/Lua/gameplay gate remains false\n";
}
