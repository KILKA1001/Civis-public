#pragma once
#include "civ5/assets/asset_source.hpp"
#include <functional>

namespace civ5::assets {
// Native VFS lookup uses the case-insensitive basename, including when the
// request contains a directory. Files are still confined to an explicit root.
std::string resource_basename(const std::string &name);
struct LooseResourceScan {
    std::filesystem::path directory;
    std::vector<std::filesystem::path> excluded_directories;
    bool exclude_scenarios_and_speech = false;
};
// Loose records in native traversal/insertion order. No archive extraction,
// legacy 4096-directory table, 20-bit record packing, or CRC-only identity.
class LooseResourceIndex final : public AssetSource {
  public:
    explicit LooseResourceIndex(const std::filesystem::path &root);
    void scan(const LooseResourceScan &scan);
    void add_file(const std::filesystem::path &file);
    AssetLookup find(const AssetKey &key) const override;
    std::vector<AssetHandle> find_all(const AssetKey &key) const override;
    std::vector<std::string> keys(std::string_view prefix = {}) const override;
    Provenance provenance() const override;
    // OWN port admission DATA in actual add_file order; not Native FIRST authority.
    const std::vector<AssetHandle>& port_admission_order() const { return port_admissions_; }
    std::uint64_t indexed_files() const { return indexed_files_; }
    std::uint64_t unique_names() const { return entries_.size(); }

  private:
    struct Entry {
        std::filesystem::path file;
        std::string relative;
        std::uint64_t size;
    };
    std::filesystem::path root_;
    std::unordered_map<std::string, std::vector<Entry>> entries_;
    std::uint64_t indexed_files_ = 0;
    std::vector<AssetHandle> port_admissions_;
};
struct ResourceMountHit {
    AssetLookup asset;
    // Native: no archive handle, or its preference flag at +0x280 is set.
    // A source containing multiple archives can decide this for each result.
    bool preferred = true;
};
using ResourceMountLookup = std::function<ResourceMountHit(const AssetKey &)>;
using ResourceMountLookupAll = std::function<std::vector<ResourceMountHit>(const AssetKey &)>;
using ResourceKeyLookup = std::function<std::vector<std::string>(std::string_view)>;
ResourceMountLookup resource_lookup(std::shared_ptr<const AssetSource> source, bool preferred = true);
ResourceMountLookupAll resource_lookup_all(std::shared_ptr<const AssetSource> source,
                                            bool preferred = true);
ResourceKeyLookup resource_keys(std::shared_ptr<const AssetSource> source);
struct ResourceMount {
    std::uint64_t id, group = 0, priority = 0;
    ResourceMountLookup lookup;
    bool enabled = true, preferred = true;
    ResourceMountLookupAll all_lookup;
    ResourceKeyLookup key_lookup;
    // Optional OWN-port provider control. Never implies a Native namespace.
    std::shared_ptr<const AssetSource> port_source_owner;
};
struct MountedResource {
    AssetLookup asset;
    // nullopt means the default source (or no match); inspect asset first.
    std::optional<std::uint64_t> mount_id;
};
class ResourceMountTable {
  public:
    explicit ResourceMountTable(ResourceMountLookup default_source,
                                ResourceMountLookupAll default_all = {}, ResourceKeyLookup default_keys = {},
                                std::vector<std::shared_ptr<const AssetSource>> port_default_sources = {});
    void set_default_enabled(bool enabled) { default_enabled_ = enabled; }
    // Register/re-register follows EXE 0x822B50. It is not a global priority sort.
    void mount(ResourceMount source);
    void unmount(std::uint64_t id);
    void set_enabled(std::uint64_t id, bool enabled);
    MountedResource resolve(const AssetKey &key) const;
    // Return every matching candidate in default-source then mount order.
    // This is separate from resolve(), which selects one candidate.
    std::vector<MountedResource> resolve_all(const AssetKey &key) const;
    // Discovery only; resolve() remains authoritative for payload selection.
    std::vector<std::string> keys(std::string_view prefix = {}) const;
    const std::vector<ResourceMount> &mounts() const { return mounts_; }
    bool port_default_enabled() const { return default_enabled_; }
    const std::vector<std::shared_ptr<const AssetSource>>& port_default_sources() const { return port_default_sources_; }

  private:
    ResourceMountLookup default_source_;
    ResourceMountLookupAll default_all_;
    ResourceKeyLookup default_keys_;
    bool default_enabled_ = true;
    std::vector<std::shared_ptr<const AssetSource>> port_default_sources_;
    std::vector<ResourceMount> mounts_;
};
namespace detail { struct ResourceResolveObserverState; }
// External, thread-scoped instrumentation. The existing table layout and all
// lookup/priority/mount identities remain unchanged. Results are borrowed only
// for the callback; resolve_all is reported as one ordered observation.
// Nested resolves inside an observer do not recursively notify observers.
class ScopedResourceResolveObserver {
  public:
    using Callback=std::function<void(const AssetKey &,std::span<const MountedResource>,bool)>;
    ScopedResourceResolveObserver(const ResourceMountTable &,Callback);
    ~ScopedResourceResolveObserver();
    ScopedResourceResolveObserver(const ScopedResourceResolveObserver &)=delete;
    ScopedResourceResolveObserver &operator=(const ScopedResourceResolveObserver &)=delete;
    // Callback failures are suppressed to preserve lookup semantics and must
    // make a capture proof pending. Registration applies only on its thread.
    bool failed() const noexcept;
  private:
    std::shared_ptr<detail::ResourceResolveObserverState> state_;
};
} // namespace civ5::assets
