#include "civ5/assets/resource_mounts.hpp"
#ifdef _WIN32
#include "civ5/platform/windows_path.hpp"
#endif
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <atomic>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace civ5::assets {
namespace detail {
struct ResourceResolveObserverState {
    const ResourceMountTable *table;
    ScopedResourceResolveObserver::Callback callback;
    std::atomic_bool active{true},failed{false};
    ResourceResolveObserverState(const ResourceMountTable &t,ScopedResourceResolveObserver::Callback c)
        :table(&t),callback(std::move(c)) {}
};
}
namespace fs = std::filesystem;
namespace {
thread_local std::vector<std::weak_ptr<detail::ResourceResolveObserverState>> resolve_observers;
thread_local bool notifying_resolve_observers=false;
void observe_resolve(const ResourceMountTable &table,const AssetKey &key,
                     std::span<const MountedResource> result,bool all) noexcept {
    if(resolve_observers.empty() || notifying_resolve_observers) return;
    try {
        std::erase_if(resolve_observers,[](const auto &entry){return entry.expired();});
        std::vector<std::shared_ptr<detail::ResourceResolveObserverState>> callbacks;
        for(const auto &entry:resolve_observers)
            if(auto state=entry.lock();state && state->active && state->table==&table) callbacks.push_back(std::move(state));
        notifying_resolve_observers=true;
        struct Clear { ~Clear(){notifying_resolve_observers=false;} } clear;
        for(const auto &state:callbacks) {
            if(!state->active) continue;
            try {state->callback(key,result,all);} catch(...) {state->failed=true;}
        }
    } catch(...) {
        for(const auto &entry:resolve_observers)
            if(auto state=entry.lock();state && state->table==&table) state->failed=true;
    }
}
bool beneath(const fs::path &root, const fs::path &path) {
    const auto relative = path.lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != fs::path("..");
}
bool native_excluded(const fs::path &path) {
    std::string previous;
    for (const auto &part : path) {
        const auto name = normalize_asset_name(path_utf8(part));
        if (name == "scenarios" || (previous == "sounds" && name == "speech"))
            return true;
        previous = name;
    }
    return false;
}
bool invisible_or_link(const fs::path &path) {
#ifdef _WIN32
    const auto flags = GetFileAttributesW(platform::extended_windows_path(path).c_str());
    if (flags == INVALID_FILE_ATTRIBUTES)
        throw std::runtime_error("Cannot read resource file attributes");
    if (flags & FILE_ATTRIBUTE_REPARSE_POINT)
        throw std::runtime_error("Resource scan does not support reparse points");
    return (flags & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
#else
    if (fs::is_symlink(path))
        throw std::runtime_error("Resource scan does not support symbolic links");
    return false;
#endif
}
fs::path file_path(const fs::path &path) {
#ifdef _WIN32
    return platform::extended_windows_path(path);
#else
    return path;
#endif
}
fs::path canonical_file(const fs::path &path) {
#ifdef _WIN32
    return platform::unextended_windows_path(fs::canonical(file_path(path)));
#else
    return fs::canonical(path);
#endif
}
bool found(const AssetLookup &asset) { return !std::holds_alternative<std::monostate>(asset); }
} // namespace
std::string resource_basename(const std::string &name) {
    if (name.find('\0') != std::string::npos)
        throw std::runtime_error("NUL in resource name");
    const auto slash = name.find_last_of("/\\");
    return normalize_asset_name(slash == std::string::npos ? name : name.substr(slash + 1));
}
LooseResourceIndex::LooseResourceIndex(const fs::path &root) : root_(canonical_file(root)) {
    if (!fs::is_directory(file_path(root_)))
        throw std::runtime_error("Resource root is not a directory");
}
void LooseResourceIndex::add_file(const fs::path &file) {
    const auto path = canonical_file(file.is_absolute() ? file : root_ / file);
    if (!beneath(root_, path) || !fs::is_regular_file(file_path(path)))
        throw std::runtime_error("Resource file is outside its root or not regular");
    const auto relative = path_utf8(path.lexically_relative(root_));
    const auto key = resource_basename(relative);
    if (key.empty())
        throw std::runtime_error("Empty resource filename");
    if (indexed_files_ == std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("Resource record count overflow");
    // EXE 0x83B130: loose insertion modes 2 and 7 both replace earlier loose
    // records. Archive collision modes are deliberately not applied here.
    entries_[key].push_back(Entry{path, relative, fs::file_size(file_path(path))});
    auto admission_source = provenance();
    admission_source.source_relative_path = relative;
    port_admissions_.push_back(AssetHandle(AssetKey(relative),
        AssetCandidate{relative,normalize_asset_name(relative),std::move(admission_source),entries_.at(key).back().size},
        path,0,root_));
    ++indexed_files_;
}
void LooseResourceIndex::scan(const LooseResourceScan &scan) {
    const auto directory =
        fs::weakly_canonical(scan.directory.is_absolute() ? scan.directory : root_ / scan.directory);
    if (!beneath(root_, directory))
        throw std::runtime_error("Resource scan is outside its root");
    std::vector<fs::path> excluded;
    for (const auto &path : scan.excluded_directories) {
        const auto absolute = fs::weakly_canonical(path.is_absolute() ? path : root_ / path);
        if (!beneath(root_, absolute))
            throw std::runtime_error("Resource exclusion is outside its root");
        excluded.push_back(absolute);
    }
    std::error_code error;
    const auto status = fs::status(directory, error);
    // Original packages can declare absent directories (e.g. BNW Maps).
    // EXE 0x83A720 returns zero records when its directory check fails.
    if (status.type() == fs::file_type::not_found)
        return;
    if (error)
        throw fs::filesystem_error("Cannot inspect resource directory", directory, error);
    if (!fs::is_directory(status))
        throw std::runtime_error("Resource scan is not a directory");
    auto skip = [&](const fs::path &path) {
        if (invisible_or_link(path))
            return true;
        if (scan.exclude_scenarios_and_speech && native_excluded(path.lexically_relative(root_)))
            return true;
        const auto normalized = normalize_asset_name(path_utf8(path));
        for (const auto &exclude : excluded) {
            const auto prefix = normalize_asset_name(path_utf8(exclude));
            if (normalized == prefix || normalized.starts_with(prefix + '/'))
                return true;
        }
        return false;
    };
    if (skip(directory))
        return;
    // Preserve the filesystem's enumeration order, depth first at each entry,
    // as FindFirstFile/FindNextFile in EXE 0x83A720. No path sorting. Iterative
    // traversal removes the original recursive call-stack capacity limit.
    std::vector<fs::directory_iterator> stack;
    stack.emplace_back(file_path(directory));
    while (!stack.empty()) {
        auto &iterator = stack.back();
        if (iterator == fs::directory_iterator{}) {
            stack.pop_back();
            continue;
        }
        const auto entry = *iterator;
        ++iterator;
        const auto relative=entry.path().lexically_relative(file_path(root_));
        if(relative.empty()||relative.is_absolute()||*relative.begin()==fs::path(".."))
            throw std::runtime_error("Resource entry escaped scan root");
        const auto path=root_/relative;
        if (skip(path))
            continue;
        if (entry.is_directory())
            stack.emplace_back(entry.path());
        else if (entry.is_regular_file())
            add_file(path);
    }
}
Provenance LooseResourceIndex::provenance() const { return {SourceKind::loose, path_utf8(root_), ".", {}}; }
AssetLookup LooseResourceIndex::find(const AssetKey &key) const {
    const auto entry = entries_.find(resource_basename(key.original_name()));
    if (entry == entries_.end())
        return {};
    const auto &value = entry->second.back();
    auto source = provenance();
    source.source_relative_path = value.relative;
    return AssetHandle(key, {value.relative, normalize_asset_name(value.relative), source, value.size},
                       value.file, 0, root_);
}
std::vector<AssetHandle> LooseResourceIndex::find_all(const AssetKey &key) const {
    std::vector<AssetHandle> result;
    const auto entry = entries_.find(resource_basename(key.original_name()));
    if (entry == entries_.end())
        return result;
    result.reserve(entry->second.size());
    for (const auto &value : entry->second) {
        auto source = provenance();
        source.source_relative_path = value.relative;
        result.push_back(AssetHandle(
            key, AssetCandidate{value.relative, normalize_asset_name(value.relative), source, value.size},
            value.file, 0, root_));
    }
    return result;
}
std::vector<std::string> LooseResourceIndex::keys(std::string_view prefix) const {
    const auto normalized_prefix = normalize_asset_name(std::string(prefix));
    std::vector<std::string> result;
    for (const auto &[basename, entries] : entries_)
        for (const auto &entry : entries) {
            const auto normalized = normalize_asset_name(entry.relative);
            if (normalized_prefix.empty() || normalized == normalized_prefix ||
                normalized.starts_with(normalized_prefix + '/'))
                result.push_back(entry.relative);
        }
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const auto l = normalize_asset_name(left), r = normalize_asset_name(right);
        return l == r ? left < right : l < r;
    });
    result.erase(std::unique(result.begin(), result.end(), [](const auto &left, const auto &right) {
                     return normalize_asset_name(left) == normalize_asset_name(right);
                 }),
                 result.end());
    return result;
}
ResourceMountLookup resource_lookup(std::shared_ptr<const AssetSource> source, bool preferred) {
    if (!source)
        throw std::runtime_error("Null resource source");
    return [source = std::move(source), preferred](const AssetKey &key) {
        return ResourceMountHit{source->find(key), preferred};
    };
}
ResourceMountLookupAll resource_lookup_all(std::shared_ptr<const AssetSource> source, bool preferred) {
    if (!source)
        throw std::runtime_error("Null resource source");
    return [source = std::move(source), preferred](const AssetKey &key) {
        std::vector<ResourceMountHit> result;
        for (auto &asset : source->find_all(key))
            result.push_back({std::move(asset), preferred});
        return result;
    };
}
ResourceKeyLookup resource_keys(std::shared_ptr<const AssetSource> source) {
    if (!source)
        throw std::runtime_error("Null resource source");
    return [source = std::move(source)](std::string_view prefix) { return source->keys(prefix); };
}
ResourceMountTable::ResourceMountTable(ResourceMountLookup source, ResourceMountLookupAll all_source,
                                       ResourceKeyLookup default_keys,
                                       std::vector<std::shared_ptr<const AssetSource>> port_default_sources)
    : default_source_(std::move(source)), default_all_(std::move(all_source)),
      default_keys_(std::move(default_keys)),port_default_sources_(std::move(port_default_sources)) {
    if (!default_source_)
        throw std::runtime_error("Missing default resource source");
    if (!default_all_)
        default_all_ = [source = default_source_](const AssetKey &key) {
            const auto hit = source(key);
            if (std::holds_alternative<std::monostate>(hit.asset))
                return std::vector<ResourceMountHit>{};
            return std::vector<ResourceMountHit>{hit};
        };
}
ScopedResourceResolveObserver::ScopedResourceResolveObserver(const ResourceMountTable &table,Callback callback) {
    if(!callback) throw std::invalid_argument("Missing resource resolve observer callback");
    state_=std::make_shared<detail::ResourceResolveObserverState>(table,std::move(callback));
    resolve_observers.push_back(state_);
}
ScopedResourceResolveObserver::~ScopedResourceResolveObserver() {state_->active=false;}
bool ScopedResourceResolveObserver::failed() const noexcept {return state_->failed;}
void ResourceMountTable::mount(ResourceMount source) {
    if (!source.lookup)
        throw std::runtime_error("Missing mounted resource source");
    const auto existing = std::find_if(mounts_.begin(), mounts_.end(),
                                       [&](const auto &mount) { return mount.id == source.id; });
    if (source.group == 0 && source.priority == 0) {
        // Native leaves an already registered (0,0) source in place.
        if (existing != mounts_.end())
            *existing = std::move(source);
        else
            mounts_.push_back(std::move(source));
        return;
    }
    if (existing != mounts_.end())
        mounts_.erase(existing);
    auto position = mounts_.end();
    for (auto it = mounts_.begin(); it != mounts_.end(); ++it) {
        if (it->group != source.group)
            continue;
        position = it;
        if (source.priority < it->priority)
            break;
        position = std::next(it);
    }
    mounts_.insert(position, std::move(source));
}
void ResourceMountTable::unmount(std::uint64_t id) {
    std::erase_if(mounts_, [&](const auto &mount) { return mount.id == id; });
}
void ResourceMountTable::set_enabled(std::uint64_t id, bool enabled) {
    const auto found =
        std::find_if(mounts_.begin(), mounts_.end(), [&](const auto &mount) { return mount.id == id; });
    if (found == mounts_.end())
        throw std::runtime_error("Unknown resource mount");
    found->enabled = enabled;
}
MountedResource ResourceMountTable::resolve(const AssetKey &key) const {
    const auto finish=[&](MountedResource value) {
        observe_resolve(*this,key,std::span<const MountedResource>(&value,1),false);
        return value;
    };
    if (!default_enabled_ || key.original_name().empty())
        return finish({});
    MountedResource deferred;
    for (auto it = mounts_.rbegin(); it != mounts_.rend(); ++it) {
        if (!it->enabled)
            continue;
        auto hit = it->lookup(key);
        if (!found(hit.asset))
            continue;
        MountedResource result{std::move(hit.asset), it->id};
        if (it->preferred && hit.preferred)
            return finish(std::move(result));
        deferred = std::move(result);
    }
    auto fallback = default_source_(key);
    if (found(fallback.asset))
        return finish({std::move(fallback.asset), {}});
    return finish(std::move(deferred));
}
std::vector<MountedResource> ResourceMountTable::resolve_all(const AssetKey &key) const {
    if (!default_enabled_ || key.original_name().empty())
        {observe_resolve(*this,key,{},true);return {};}
    std::vector<MountedResource> result;
    auto append = [&](const ResourceMountLookupAll &lookup, std::optional<std::uint64_t> mount_id) {
        for (auto &hit : lookup(key))
            if (!std::holds_alternative<std::monostate>(hit.asset))
                result.push_back({std::move(hit.asset), mount_id});
    };
    append(default_all_, std::nullopt);
    for (const auto &mount : mounts_)
        if (mount.enabled) {
            if (mount.all_lookup)
                append(mount.all_lookup, mount.id);
            else {
                const auto hit = mount.lookup(key);
                if (!std::holds_alternative<std::monostate>(hit.asset))
                    result.push_back({hit.asset, mount.id});
            }
        }
    observe_resolve(*this,key,result,true);
    return result;
}
std::vector<std::string> ResourceMountTable::keys(std::string_view prefix) const {
    std::vector<std::string> result;
    if (default_enabled_ && default_keys_)
        result = default_keys_(prefix);
    if (default_enabled_)
        for (const auto &mount : mounts_)
            if (mount.enabled && mount.key_lookup) {
                const auto mounted = mount.key_lookup(prefix);
                result.insert(result.end(), mounted.begin(), mounted.end());
            }
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const auto l = normalize_asset_name(left), r = normalize_asset_name(right);
        return l == r ? left < right : l < r;
    });
    result.erase(std::unique(result.begin(), result.end(), [](const auto &left, const auto &right) {
                     return normalize_asset_name(left) == normalize_asset_name(right);
                 }),
                 result.end());
    return result;
}
} // namespace civ5::assets
