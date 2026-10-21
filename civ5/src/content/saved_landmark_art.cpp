#include "civ5/content/landmark_art_registry.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/content/saved_rules.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::content {
void seed_landmark_art_rows(RulesDatabase &rules,std::span<const LandmarkArtRow> rows){
    std::int64_t existing=-1;
    rules.query({"SELECT count(*) FROM ArtDefine_Landmarks",{}},
        [&](std::span<const DatabaseValue> values){
            existing=std::get<std::int64_t>(values[0]);
        });
    if(existing!=0)
        throw std::runtime_error("Base/DLC landmark art must precede all mod art rows");
    std::vector<DatabaseStatement> statements;
    statements.reserve(rows.size());
    for(const auto &row:rows){
        if(row.improvement.empty() || row.resource.empty() || row.era.empty() ||
           row.state.empty() || row.layout_handler.empty())
            throw std::invalid_argument("Incomplete base/DLC landmark art row");
        statements.push_back({"INSERT INTO ArtDefine_Landmarks "
            "(ImprovementType,ResourceType,Era,State,Scale,Model,LayoutHandler,TerrainContour,Tech) "
            "VALUES(?,?,?,?,?,?,?,?,?)",
            {row.improvement,row.resource,row.era,row.state,
             row.scale.empty()?DatabaseValue{1.0}:game_data_value(row.scale),
             row.model.empty()?DatabaseValue{std::monostate{}}:DatabaseValue{row.model},
             row.layout_handler,
             row.terrain_contour.empty()?DatabaseValue{std::int64_t{0}}:
                 game_data_value(row.terrain_contour),
             row.tech.empty()?DatabaseValue{std::monostate{}}:DatabaseValue{row.tech}}});
    }
    rules.apply_statements(statements);
}

void seed_landmark_type_rows(RulesDatabase &rules,std::span<const LandmarkTypeRow> rows){
    std::int64_t existing=-1;
    rules.query({"SELECT count(*) FROM ArtDefine_LandmarkTypes",{}},
        [&](std::span<const DatabaseValue> values){
            existing=std::get<std::int64_t>(values[0]);
        });
    if(existing!=0)
        throw std::runtime_error("Base/DLC landmark types must precede all mod art rows");
    std::vector<DatabaseStatement> statements;
    statements.reserve(rows.size());
    for(const auto &row:rows){
        if(row.type.empty() || row.landmark_type.empty() || row.friendly_name.empty())
            throw std::invalid_argument("Incomplete base/DLC landmark type row");
        statements.push_back({"INSERT INTO ArtDefine_LandmarkTypes "
            "(Type,LandmarkType,FriendlyName) VALUES(?,?,?)",
            {row.type,row.landmark_type,row.friendly_name}});
    }
    rules.apply_statements(statements);
}

std::vector<ActiveLandmarkArtRow> read_active_landmark_art_rows(const RulesDatabase &rules){
    std::vector<ActiveLandmarkArtRow> rows;
    const auto report=rules.query_rows(
        {"SELECT ImprovementType,ResourceType,Era,State,Scale,Model,LayoutHandler,"
         "TerrainContour,Tech FROM ArtDefine_Landmarks ORDER BY rowid",{}},
        [&](const RulesRowView &row){
            const auto improvement=row.text("ImprovementType");
            const auto resource=row.text("ResourceType");
            const auto era=row.text("Era");
            const auto state=row.text("State");
            const auto layout=row.text("LayoutHandler");
            if(!improvement || !resource || !era || !state || !layout)
                throw std::runtime_error("Incomplete active landmark art row");
            rows.push_back({*improvement,*resource,*era,*state,*layout,
                            row.real("Scale"),row.text("Model"),row.text("Tech"),
                            row.integer("TerrainContour")});
        });
    if(!report.all_succeeded())
        throw std::runtime_error("Cannot read active landmark art rows: "+report.error);
    return rows;
}

namespace {
LandmarkArtRegistry registry_from_active_rows(std::span<const ActiveLandmarkArtRow> active){
    std::vector<LandmarkArtRow> rows;
    rows.reserve(active.size());
    for(const auto &row:active)
        rows.push_back({row.improvement,row.resource,row.era,row.state,{},{},{},{},{}});
    return build_landmark_art_registry(rows);
}
}

LandmarkArtRegistry read_landmark_art_registry(const RulesDatabase &rules){
    return registry_from_active_rows(read_active_landmark_art_rows(rules));
}

SavedLandmarkArtLoadResult load_saved_landmark_art_registry(
    const std::filesystem::path &save,const std::filesystem::path &game_root,
    const std::filesystem::path &mount_plan,std::array<bool,3> enabled_categories,
    std::uint64_t maximum_source_bytes){
    namespace fs=std::filesystem;
    const auto selection=runtime::select_save(save);
    if(!selection.selected)
        throw std::runtime_error("No supported save header");
    const auto &candidate=selection.candidates[*selection.selected];
    const auto save_identity=runtime::identify_file(candidate.path);
    const auto plan_identity=runtime::identify_file(mount_plan);
    auto catalog=discover_packages({game_root/"Assets/DLC"});
    if(!catalog.diagnostics.empty())
        throw std::runtime_error("DLC catalog: "+catalog.diagnostics.front());
    const auto profile=runtime::resolve_saved_content_profile(candidate.header,catalog);
    for(const auto &requirement:profile.requirements){
        if(requirement.kind==PackageKind::dlc &&
           requirement.status!=runtime::RequirementStatus::located)
            throw std::runtime_error("Saved DLC is missing or ambiguous: "+requirement.id);
    }
    auto mounts=read_loose_resource_plan(mount_plan);
    if(runtime::identify_file(mount_plan).sha256!=plan_identity.sha256)
        throw std::runtime_error("Mount plan changed while indexing");
    if(fs::weakly_canonical(mounts.root)!=fs::weakly_canonical(game_root))
        throw std::runtime_error("Mount plan belongs to another game root");
    const auto opened=assets::FpkSource::open(game_root/"Resource/Common/MiscData.fpk",
                                               "Resource/Common/MiscData.fpk");
    const auto *source=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
    if(!source)
        throw std::runtime_error("Cannot open original MiscData.fpk");
    const auto found=(*source)->find(assets::AssetKey("civ5artdefines_landmarks.xml"));
    const auto *base=std::get_if<assets::AssetHandle>(&found);
    if(!base)
        throw std::runtime_error("Cannot locate base landmark art");
    const auto found_types=(*source)->find(assets::AssetKey("civ5artdefines_landmarktypes.xml"));
    const auto *base_types=std::get_if<assets::AssetHandle>(&found_types);
    if(!base_types)
        throw std::runtime_error("Cannot locate base landmark types");
    SavedLandmarkArtLoadResult result;
    result.art=load_landmark_art_registry(*base,*base_types,profile.enabled_dlc,catalog,mounts.resources,
                                          enabled_categories,maximum_source_bytes);
    if(runtime::identify_file(candidate.path).sha256!=save_identity.sha256)
        throw std::runtime_error("Save changed while loading art");
    result.selected_dlc=profile.enabled_dlc.size();
    result.indexed_loose_files=mounts.indexed_files;
    result.save_sha256=save_identity.sha256;
    result.mount_plan_sha256=plan_identity.sha256;
    result.catalog=std::move(catalog);
    return result;
}

SavedModLandmarkArtLoadResult load_saved_mod_landmark_art_registry(
    const SavedLandmarkArtLoadResult &dlc_art,
    const std::filesystem::path &save,const std::filesystem::path &game_root,
    const std::filesystem::path &mount_plan,
    std::span<const std::filesystem::path> mod_roots){
    if(mod_roots.empty())
        throw std::invalid_argument("Supply at least one mod root for saved art replay");
    const auto selection=runtime::select_save(save);
    if(!selection.selected)
        throw std::runtime_error("No supported save header");
    const auto &candidate=selection.candidates[*selection.selected];
    if(runtime::identify_file(candidate.path).sha256!=dlc_art.save_sha256 ||
       runtime::identify_file(mount_plan).sha256!=dlc_art.mount_plan_sha256)
        throw std::runtime_error("Saved art inputs changed before mod replay");
    std::vector<std::filesystem::path> roots{game_root/"Assets/DLC"};
    roots.insert(roots.end(),mod_roots.begin(),mod_roots.end());
    const auto catalog=discover_packages(roots);
    if(!catalog.diagnostics.empty())
        throw std::runtime_error("DLC/mod catalog: "+catalog.diagnostics.front());
    const auto profile=runtime::resolve_saved_content_profile(candidate.header,catalog);
    if(!profile.all_requirements_located || profile.enabled_dlc.size()!=dlc_art.selected_dlc)
        throw std::runtime_error("Saved art requires all selected DLC/mod manifests");
    const auto mounts=read_loose_resource_plan(mount_plan);
    if(std::filesystem::weakly_canonical(mounts.root)!=
       std::filesystem::weakly_canonical(game_root))
        throw std::runtime_error("Mount plan belongs to another game root");
    RulesDatabase rules;
    const auto activation=activate_saved_rules(
        rules,game_root,profile,catalog,mounts.resources,64ULL*1024*1024,
        [&](RulesDatabase &database){
            seed_landmark_type_rows(database,dlc_art.art.type_rows);
            seed_landmark_art_rows(database,dlc_art.art.rows);
            const auto seeded=read_landmark_art_registry(database);
            const auto &expected=dlc_art.art.registry;
            if(seeded.art_types!=expected.art_types ||
               seeded.materials.size()!=expected.materials.size())
                throw std::runtime_error("Base/DLC art changed during database seed");
            for(std::size_t i=0;i<seeded.materials.size();++i){
                const auto &a=seeded.materials[i],&b=expected.materials[i];
                if(a.improvement!=b.improvement || a.resource!=b.resource ||
                   a.era!=b.era || a.state!=b.state)
                    throw std::runtime_error("Base/DLC art row changed during database seed");
            }
        });
    SavedModLandmarkArtLoadResult result;
    result.active_rows=read_active_landmark_art_rows(rules);
    result.registry=registry_from_active_rows(result.active_rows);
    result.selected_mods=profile.enabled_mods.size();
    result.actions=activation.mods.actions.size();
    for(const auto &action:activation.mods.actions)
        if(!action.applied)
            result.failures.push_back({action.action.requested_path,action.error});
    result.profile_verified=profile.can_activate && activation.mods.all_succeeded;
    rules.query({"SELECT count(*) FROM ArtDefine_LandmarkTypes",{}},
        [&](std::span<const DatabaseValue> values){
            result.landmark_type_rows=static_cast<std::uint64_t>(
                std::get<std::int64_t>(values[0]));
        });
    if(runtime::identify_file(candidate.path).sha256!=dlc_art.save_sha256 ||
       runtime::identify_file(mount_plan).sha256!=dlc_art.mount_plan_sha256)
        throw std::runtime_error("Saved art inputs changed during mod replay");
    return result;
}
} // namespace civ5::content
