#include "civ5/ui/frontend_content.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace civ5::ui {
namespace {
std::filesystem::path normalized_path(const std::filesystem::path &path) {
    return std::filesystem::absolute(path).lexically_normal();
}
std::string path_key(const std::filesystem::path &path) {
    return assets::normalize_asset_name(assets::path_utf8(normalized_path(path)));
}
std::string segment(const std::string &value) {
    return std::to_string(value.size()) + ":" + value;
}
std::string skin_suffix(const content::PackageManifest &package) {
    // Stock expansion manifests declare Common and Tablet UISkins separately.
    // This is a caption of declared sets, not selection of a platform/skin.
    std::set<std::string> seen;
    std::string sets;
    for (const auto &node : package.document.children) if (node.name == "UISkin") {
        const auto value = node.attribute("set");
        if (value.empty() || !seen.insert(value).second) continue;
        if (!sets.empty()) sets += ", ";
        sets += value;
    }
    return sets.empty() ? std::string{} : " (" + sets + ")";
}
std::string source_label(const content::PackageManifest &package, std::size_t components = 2) {
    std::vector<std::string> parts;
    for (const auto &part : package.path.lexically_normal()) {
        const auto value = assets::path_utf8(part);
        if (!value.empty() && value != "/" && value != "\\") parts.push_back(value);
    }
    std::string result;
    for (auto i = parts.size() > components ? parts.size() - components : 0; i < parts.size(); ++i) {
        if (!result.empty()) result += '/';
        result += parts[i];
    }
    result += skin_suffix(package);
    return result;
}
bool same_identity(const runtime::FileIdentity &a, const runtime::FileIdentity &b) {
    return a.bytes == b.bytes && a.sha256 == b.sha256;
}
UiCommandResult unavailable(std::string reason) {
    return {UiCommandStatus::unavailable, std::move(reason)};
}
std::string profile_blocker(const runtime::SavedContentProfile &profile) {
    // Missing native files must not disguise the unsupported native boundary.
    if (profile.native_mods_require_port) return "Native mod DLL requires a Civis port";
    for (const auto &requirement : profile.requirements) {
        const auto name = requirement.name.empty() ? requirement.id : requirement.name;
        switch (requirement.status) {
        case runtime::RequirementStatus::missing: return "Required package missing: " + name;
        case runtime::RequirementStatus::version_mismatch: return "Required package version mismatch: " + name;
        case runtime::RequirementStatus::ambiguous: return "Required package source ambiguous: " + name;
        case runtime::RequirementStatus::located: break;
        }
    }
    if (!profile.files_complete) return "Selected package files missing or invalid";
    if (!profile.can_activate) return "Selected content cannot activate";
    return {};
}
std::vector<std::string> required_keys(const runtime::SavedContentProfile &profile,
                                     const content::Catalog &catalog) {
    std::vector<std::string> keys;
    for (const auto &requirement : profile.requirements)
        if (requirement.status == runtime::RequirementStatus::located && requirement.package)
            keys.push_back(FrontendCatalogContentAdapter::package_key(catalog.packages.at(*requirement.package)));
    return keys;
}
} // namespace
std::string FrontendCatalogContentAdapter::package_key(const content::PackageManifest &package) {
    const auto kind = package.kind == content::PackageKind::mod ? "mod" : "dlc";
    // Original Mod manifests may use non-RFC identifiers (Semper Fidelis).
    const auto id = package.kind == content::PackageKind::dlc ? content::canonical_guid(package.id)
                                                             : runtime::ascii_lower(package.id);
    if (id.empty()) throw std::invalid_argument("Package identity is empty");
    return std::string(kind) + ":" + segment(id) + ":" +
        std::to_string(package.version) + ":" + segment(path_key(package.path));
}
FrontendCatalogContentAdapter::FrontendCatalogContentAdapter(
        std::shared_ptr<const content::Catalog> catalog, std::vector<std::filesystem::path> explicit_saves)
    : catalog_(std::move(catalog)) {
    if (!catalog_) throw std::invalid_argument("Frontend content catalog is null");
    package_snapshots_.resize(catalog_->packages.size());
    for (std::size_t i = 0; i < catalog_->packages.size(); ++i) {
        const auto &package = catalog_->packages[i];
        FrontendPackage row;
        row.key = package_key(package);
        row.name = package.name.empty() ? package.id : package.name;
        row.mod = package.kind == content::PackageKind::mod;
        row.version = package.version;
        row.selection_identity = std::string(row.mod ? "mod:" : "dlc:") +
            (row.mod ? runtime::ascii_lower(package.id) : content::canonical_guid(package.id));
        row.source_detail = assets::path_utf8(package.path.lexically_normal());
        std::replace(row.source_detail.begin(), row.source_detail.end(), '\\', '/');
        const auto dlc_root = runtime::ascii_lower(row.source_detail).find("/assets/dlc/");
        if (dlc_root != std::string::npos) row.source_detail.erase(0, dlc_root + 12);
        row.source_detail += skin_suffix(package);
        row.source_label = source_label(package);
        if (!package_indices_.emplace(row.key, i).second)
            throw std::invalid_argument("Duplicate frontend package identity/source");
        auto &snapshot = package_snapshots_[i];
        try {
            snapshot.identity = runtime::identify_file(package.path);
            row.availability = {UiCommandStatus::ready, "Content selection; validation occurs on launch"};
        } catch (const std::exception &error) {
            snapshot.error = error.what();
            row.availability = unavailable("Package manifest unavailable: " + snapshot.error);
        }
        data_.packages.push_back(std::move(row));
    }
    for (std::size_t i = 0; i < data_.packages.size(); ++i) {
        auto &row = data_.packages[i];
        // Expand colliding short captions only as far as needed. Source keys,
        // selection identity and content admission keep the complete path.
        for (std::size_t components = 2;; ++components) {
            const auto caption = source_label(catalog_->packages[i], components);
            const bool collision = std::any_of(data_.packages.begin(), data_.packages.end(), [&](const auto &other) {
                const auto j = static_cast<std::size_t>(&other - data_.packages.data());
                return i != j && row.name == other.name && row.mod == other.mod && row.version == other.version &&
                       caption == source_label(catalog_->packages[j], components);
            });
            row.source_label = caption;
            if (!collision || caption == source_label(catalog_->packages[i], components + 1)) break;
        }
    }
    std::set<std::string> save_ids;
    for (const auto &path : explicit_saves) {
        FrontendSave row;
        row.path = normalized_path(path);
        row.id = "save:" + segment(path_key(row.path));
        if (!save_ids.insert(row.id).second) continue;
        row.name = assets::path_utf8(row.path.stem());
        SaveSnapshot snapshot;
        try {
            const auto before = runtime::identify_file(row.path);
            if (runtime::ascii_lower(assets::path_utf8(row.path.extension())) == ".civissave") {
                // The existing Civis readers materialize payload sections. Do
                // that only for the explicitly selected slot in check_content.
                row.detail = "Civis: container validation on selection";
            } else {
                snapshot.header = saves::read_header(row.path);
                row.detail = "Turn " + std::to_string(snapshot.header->turn) + " · " + snapshot.header->civilization;
            }
            const auto after = runtime::identify_file(row.path);
            if (!same_identity(before, after)) throw std::runtime_error("Save changed while reading metadata");
            snapshot.identity = after;
            row.availability = {UiCommandStatus::ready, "Save selection; payload validation occurs on launch"};
        } catch (const std::exception &error) {
            snapshot.error = error.what();
            row.detail = snapshot.error;
            row.availability = unavailable("Save metadata unavailable: " + snapshot.error);
        }
        data_.saves.push_back(std::move(row));
        save_snapshots_.push_back(std::move(snapshot));
    }
}
FrontendContentCheck FrontendCatalogContentAdapter::check_selection(const FrontendRequest &request) const {
    FrontendContentCheck result;
    try {
        using Identity = std::pair<content::PackageKind, std::string>;
        const auto identity = [](content::PackageKind kind, const std::string &id) -> Identity {
            return {kind, kind == content::PackageKind::dlc ? content::canonical_guid(id) : runtime::ascii_lower(id)};
        };
        std::map<Identity, std::size_t> chosen;
        for (const auto &selected : request.draft.packages) {
            const auto found = package_indices_.find(selected);
            if (found == package_indices_.end()) throw std::runtime_error("Selected package key outside the frontend catalog");
            const auto index = found->second;
            const auto &package = catalog_->packages.at(index);
            if (!chosen.emplace(identity(package.kind, package.id), index).second)
                throw std::runtime_error("Multiple selected sources/versions for one package");
            const auto &row = data_.packages.at(index);
            if (row.availability.status != UiCommandStatus::ready)
                throw std::runtime_error(row.availability.reason);
            if (!package_snapshots_.at(index).identity)
                throw std::runtime_error("Package manifest unavailable: " + package_snapshots_.at(index).error);
        }
        if (request.operation == "load_game") {
            const auto selected_save = request.save ? std::optional<std::string>(request.save->id) : request.draft.save;
            if (!selected_save) throw std::runtime_error("No save selected");
            const auto row = std::find_if(data_.saves.begin(), data_.saves.end(),
                [&](const auto &save) { return save.id == *selected_save; });
            if (row == data_.saves.end()) throw std::runtime_error("Selected save is not an explicit frontend slot");
            // Lexical comparison of supplied/cached paths only; no absolute,
            // canonical, status or identity queries belong in presentation.
            const auto lexical_key = [](const std::filesystem::path &path) {
                return assets::normalize_asset_name(assets::path_utf8(path.lexically_normal()));
            };
            if ((request.draft.save && *request.draft.save != row->id) ||
                (request.save && lexical_key(request.save->path) != lexical_key(row->path)))
                throw std::runtime_error("Selected save identity/path mismatch");
            const auto &snapshot = save_snapshots_.at(static_cast<std::size_t>(row - data_.saves.begin()));
            if (!snapshot.identity || !snapshot.error.empty())
                throw std::runtime_error("Save metadata unavailable: " + snapshot.error);
            if (row->availability.status != UiCommandStatus::ready)
                throw std::runtime_error(row->availability.reason);
            if (!snapshot.header) {
                // Civis metadata is not read by the cheap initial snapshot.
                // Unknown requirements are not an empty, validated profile.
                result.requirements_known = false;
            } else {
                auto requirements = runtime::locate_saved_content(*snapshot.header, *catalog_);
                for (auto &required : requirements) {
                    const auto pin = chosen.find(identity(required.kind, required.id));
                    if (required.kind == content::PackageKind::mod && pin != chosen.end()) {
                        const auto &package = catalog_->packages.at(pin->second);
                        if (package.version != required.wire_value) {
                            required.status = runtime::RequirementStatus::version_mismatch;
                            required.package.reset();
                        } else if (std::find(required.candidates.begin(), required.candidates.end(), pin->second) != required.candidates.end()) {
                            // Exact catalog index comes from the selected
                            // stable-key table, not a filesystem source pin.
                            required.status = runtime::RequirementStatus::located;
                            required.package = pin->second;
                        }
                    }
                }
                for (const auto &required : requirements) {
                    const auto name = required.name.empty() ? required.id : required.name;
                    switch (required.status) {
                    case runtime::RequirementStatus::missing: throw std::runtime_error("Required package missing: " + name);
                    case runtime::RequirementStatus::version_mismatch: throw std::runtime_error("Required package version mismatch: " + name);
                    case runtime::RequirementStatus::ambiguous: throw std::runtime_error("Required package source ambiguous: " + name);
                    case runtime::RequirementStatus::located: break;
                    }
                    if (!required.package) throw std::logic_error("Located cached requirement has no catalog source");
                    const auto &package = data_.packages.at(*required.package);
                    if (package.availability.status != UiCommandStatus::ready)
                        throw std::runtime_error(package.availability.reason);
                    result.required_packages.push_back(package.key);
                }
                const std::set<std::string> required(result.required_packages.begin(), result.required_packages.end());
                const std::set<std::string> selected(request.draft.packages.begin(), request.draft.packages.end());
                if (required != selected) throw std::runtime_error("Selected content differs from save requirements");
            }
        } else {
            // This is the explicit selection, not dependency resolution or an
            // assertion that any manifest file can be activated.
            result.required_packages = request.draft.packages;
        }
        for (const auto &[selected, index] : chosen) {
            (void)selected;
            const auto &package = catalog_->packages.at(index);
            if (package.kind == content::PackageKind::mod &&
                std::any_of(package.files.begin(), package.files.end(), [](const auto &file) {
                    return runtime::ascii_lower(file.path).ends_with(".dll");
                })) throw std::runtime_error("Native mod DLL requires a Civis port");
        }
        result.result = {UiCommandStatus::ready, "Selection only; full content preflight and executor are required before an effect"};
    } catch (const std::exception &error) {
        result.result = unavailable(error.what());
    }
    return result;
}
void FrontendCatalogContentAdapter::verify_package_sources(
        const runtime::SavedContentProfile &profile, const content::Catalog &catalog) const {
    const auto verify = [&](std::size_t index) {
        const auto &package = catalog.packages.at(index);
        const auto found = package_indices_.find(package_key(package));
        if (found == package_indices_.end()) throw std::runtime_error("Package source outside the frontend catalog");
        const auto &snapshot = package_snapshots_.at(found->second);
        if (!snapshot.identity) throw std::runtime_error("Package manifest unavailable: " + snapshot.error);
        const auto current = runtime::identify_file(package.path);
        if (!same_identity(*snapshot.identity, current))
            throw std::runtime_error("Package manifest changed; refresh the content catalog");
    };
    for (const auto index : profile.enabled_dlc) verify(index);
    for (const auto index : profile.enabled_mods) verify(index);
}
struct FrontendSelectedSaveInput::State {
    struct ManifestPin {std::filesystem::path path;runtime::FileIdentity identity;};
    std::filesystem::path save_path;
    runtime::FileIdentity save_identity;
    saves::LoadedSave loaded;
    content::Catalog catalog;
    runtime::SavedContentProfile profile;
    std::vector<std::string> required_packages;
    std::vector<ManifestPin> manifests;
};
FrontendSelectedSaveInput::FrontendSelectedSaveInput(std::unique_ptr<const State> value)
    :state_(std::move(value)) {if(!state_)throw std::invalid_argument("Null selected save input");}
FrontendSelectedSaveInput::~FrontendSelectedSaveInput()=default;
FrontendSelectedSaveInput::FrontendSelectedSaveInput(FrontendSelectedSaveInput&&) noexcept=default;
FrontendSelectedSaveInput& FrontendSelectedSaveInput::operator=(FrontendSelectedSaveInput&&) noexcept=default;
const FrontendSelectedSaveInput::State& FrontendSelectedSaveInput::state()const {
    if(!state_)throw std::logic_error("Selected save input was moved");
    return *state_;
}
const std::filesystem::path& FrontendSelectedSaveInput::save_path()const{return state().save_path;}
const runtime::FileIdentity& FrontendSelectedSaveInput::save_identity()const{return state().save_identity;}
const saves::LoadedSave& FrontendSelectedSaveInput::loaded_save()const{return state().loaded;}
const content::Catalog& FrontendSelectedSaveInput::selected_catalog()const{return state().catalog;}
const runtime::SavedContentProfile& FrontendSelectedSaveInput::selected_profile()const{return state().profile;}
const std::vector<std::string>& FrontendSelectedSaveInput::required_packages()const{return state().required_packages;}
void FrontendSelectedSaveInput::require_files_current()const {
    const auto &value=state();
    if(!same_identity(value.save_identity,runtime::identify_file(value.save_path)))
        throw std::runtime_error("Selected save input changed");
    for(const auto &pin:value.manifests)
        if(!same_identity(pin.identity,runtime::identify_file(pin.path)))
            throw std::runtime_error("Selected content manifest changed");
}
FrontendSelectedSaveInput FrontendCatalogContentAdapter::prepare_selected_save_input(
        const FrontendRequest &request)const {
    if(request.operation!="load_game")throw std::invalid_argument("Selected save input requires load_game");
    std::optional<FrontendSelectedSaveInput> input;
    const auto check=evaluate_content(request,&input);
    if(check.result.status!=UiCommandStatus::ready || !input)
        throw std::runtime_error(check.result.reason.empty()?"Selected save input unavailable":check.result.reason);
    return std::move(*input);
}
FrontendContentCheck FrontendCatalogContentAdapter::check_content(const FrontendRequest &request)const {
    return evaluate_content(request,nullptr);
}
FrontendContentCheck FrontendCatalogContentAdapter::evaluate_content(const FrontendRequest &request,
        std::optional<FrontendSelectedSaveInput> *retained_input) const {
    FrontendContentCheck result;
    try {
        std::vector<std::size_t> dlc, mods;
        std::set<std::pair<content::PackageKind, std::string>> identities;
        for (const auto &selected : request.draft.packages) {
            const auto found = package_indices_.find(selected);
            if (found == package_indices_.end()) throw std::runtime_error("Selected package key outside the frontend catalog");
            const auto &package = catalog_->packages.at(found->second);
            if (!identities.emplace(package.kind, package.id).second)
                throw std::runtime_error("Multiple selected sources/versions for one package");
            (package.kind == content::PackageKind::dlc ? dlc : mods).push_back(found->second);
        }
        content::Catalog selected_catalog = *catalog_;
        runtime::SavedContentProfile profile;
        bool observed_original_flush = false;
        std::optional<saves::LoadedSave> retained_save;
        std::filesystem::path retained_path;
        std::optional<runtime::FileIdentity> retained_identity;
        const auto selected_save = request.operation == "load_game"
            ? (request.save ? std::optional<std::string>(request.save->id) : request.draft.save)
            : std::nullopt;
        if (selected_save) {
            const auto row = std::find_if(data_.saves.begin(), data_.saves.end(),
                [&](const auto &save) { return save.id == *selected_save; });
            if (row == data_.saves.end()) throw std::runtime_error("Selected save is not an explicit frontend slot");
            if ((request.draft.save && *request.draft.save != row->id) ||
                (request.save && path_key(request.save->path) != path_key(row->path)))
                throw std::runtime_error("Selected save identity/path mismatch");
            const auto index = static_cast<std::size_t>(row - data_.saves.begin());
            const auto &snapshot = save_snapshots_.at(index);
            if (!snapshot.identity) throw std::runtime_error("Save metadata unavailable: " + snapshot.error);
            const auto before = runtime::identify_file(row->path);
            if (!same_identity(*snapshot.identity, before)) throw std::runtime_error("Save changed; refresh the save list");
            auto loaded = saves::read_loaded_save(row->path);
            const auto after = runtime::identify_file(row->path);
            if (!same_identity(before, after)) throw std::runtime_error("Save changed during payload validation");
            // Retail saves normally end at native sync flush, not RFC zlib
            // end/checksum. This accepts an import observation for metadata
            // preflight only; the host still owns codec/continuation validation.
            if (loaded.bytes.empty() || (!loaded.inflate.stream_end && !loaded.inflate.sync_flush_suffix))
                throw std::runtime_error("Save payload framing is corrupt or incomplete");
            observed_original_flush = !loaded.inflate.stream_end && loaded.inflate.sync_flush_suffix;
            if (snapshot.header && *snapshot.header != loaded.header) throw std::runtime_error("Save metadata changed");
            std::vector<std::filesystem::path> pins;
            for (const auto mod : mods) {
                const auto &package = catalog_->packages.at(mod);
                if (std::any_of(loaded.header.mods.begin(), loaded.header.mods.end(), [&](const auto &required) {
                    return runtime::ascii_lower(required.id) == package.id && required.wire_value == package.version;
                })) pins.push_back(package.path);
            }
            runtime::select_saved_mod_sources(selected_catalog, loaded.header, pins);
            profile = runtime::resolve_saved_content_profile(loaded.header, selected_catalog);
            if(retained_input) {
                retained_path=row->path;
                retained_identity=after;
                retained_save=std::move(loaded);
            }
        } else {
            if (request.operation == "load_game") throw std::runtime_error("No save selected");
            profile = runtime::resolve_selected_content_profile(dlc, mods, selected_catalog);
        }
        result.required_packages = required_keys(profile, selected_catalog);
        if (const auto blocker = profile_blocker(profile); !blocker.empty()) {
            result.result = unavailable(blocker);
            return result;
        }
        verify_package_sources(profile, selected_catalog);
        // Files were checked by the shared profile resolver. Confirm source
        // identities before publishing its read-only result.
        if (selected_save) {
            const std::set<std::string> required(result.required_packages.begin(), result.required_packages.end());
            const std::set<std::string> chosen(request.draft.packages.begin(), request.draft.packages.end());
            if (required != chosen) {
                result.result = unavailable("Selected content differs from save requirements");
                return result;
            }
        }
        if (retained_input) {
            if (!retained_save || retained_path.empty() || !retained_identity)
                throw std::logic_error("Selected save input has no validated payload");
            auto input=std::make_unique<FrontendSelectedSaveInput::State>();
            input->save_path=std::move(retained_path);
            input->save_identity=std::move(*retained_identity);
            input->loaded=std::move(*retained_save);
            input->catalog=std::move(selected_catalog);
            input->profile=std::move(profile);
            input->required_packages=result.required_packages;
            const auto pin_manifest=[&](std::size_t package_index) {
                const auto &package=input->catalog.packages.at(package_index);
                const auto original=package_indices_.at(package_key(package));
                const auto &snapshot=package_snapshots_.at(original);
                if(!snapshot.identity)throw std::logic_error("Selected manifest has no input identity");
                input->manifests.push_back({normalized_path(package.path),*snapshot.identity});
            };
            for(const auto index:input->profile.enabled_dlc)pin_manifest(index);
            for(const auto index:input->profile.enabled_mods)pin_manifest(index);
            FrontendSelectedSaveInput owned(std::move(input));
            owned.require_files_current();
            *retained_input=std::move(owned);
        }
        result.result = {UiCommandStatus::ready, observed_original_flush
            ? "Content preflight only; original sync-flush continuation validation remains host responsibility"
            : "Content preflight only; gameplay executor readiness is separate"};
    } catch (const std::exception &error) {
        result.result = unavailable(error.what());
    }
    return result;
}
} // namespace civ5::ui
