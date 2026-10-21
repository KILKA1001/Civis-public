#include "civ5/materials/material_cache_preparation.hpp"
#include "material_cache_lifecycle_validation.hpp"
#include <bit>

using namespace cache_lifecycle_validation;
namespace {
struct Allocation {
    std::unique_ptr<Bytes> bytes;
    std::uint32_t label;
    std::uint8_t fill;
    void validate() const {
        require(bytes->size() >= 32, "Staging guard extent");
        require(std::all_of(bytes->begin(), bytes->begin() + 16, [](auto b) { return b == 0xda; }) &&
                    std::all_of(bytes->end() - 16, bytes->end(), [](auto b) { return b == 0xda; }),
                "Staging guard changed");
        require(std::all_of(bytes->begin() + 16, bytes->end() - 16, [this](auto b) { return b == fill; }),
                "Observed-only worker modified staging bytes");
    }
};
struct PreparationBackend final : MaterialCachePreparationBackend {
    Case &test;
    std::uint32_t failure_mask;
    unsigned allocation_count = 0;
    std::map<std::uintptr_t, Allocation> live;
    std::map<std::uintptr_t, std::uint32_t> labels;
    std::vector<MaterialCacheGeometryJob *> queued;
    PreparationBackend(Case &c, std::uint32_t failure) : test(c), failure_mask(failure) {
        require(failure <= 3, "Allocation failure mask");
        require(get(test.entering, 0x150) == 0, "Entering fixture geometry record owner must be absent");
        for (auto token : test.state.staging)
            require(!token, "Preparation fixture requires empty entering staging");
        test.observe_geometry_job = true;
        test.staging_label = [this](std::uintptr_t token) { return label(token); };
    }
    std::uint32_t label(std::uintptr_t token) const {
        if (!token)
            return 0;
        const auto found = labels.find(token);
        require(found != labels.end(), "Unknown physical staging allocation");
        return found->second;
    }
    std::uintptr_t allocate_staging(std::size_t size) override {
        const auto slot = allocation_count++;
        require(slot < 2 && size > 0 && size <= 0x10000, "Original fixture allocation bounds");
        std::uintptr_t token = 0;
        if (!(failure_mask & (1u << slot))) {
            Allocation allocation{std::make_unique<Bytes>(size + 32, 0xda), 0x34000000u + 0x10000u * slot,
                                  static_cast<std::uint8_t>(0xa1 + slot)};
            std::fill(allocation.bytes->begin() + 16, allocation.bytes->end() - 16, allocation.fill);
            token = reinterpret_cast<std::uintptr_t>(allocation.bytes->data() + 16);
            require(labels.emplace(token, allocation.label).second, "Native allocation address reused");
            require(live.emplace(token, std::move(allocation)).second, "Duplicate native allocation");
        }
        test.record(9, {size, label(token)});
        return token;
    }
    void submit_geometry(MaterialCacheGeometryJob &job, std::uintptr_t dispatch) override {
        require(&job == &test.state.geometry_job && job.owner == &test.state, "Native geometry record owner");
        queued.push_back(&job);
        test.record(10, {dispatch});
    }
    void prepare_material(MaterialCacheLifecycleState &state, std::uintptr_t dispatch) override {
        require(&state == &test.state, "Native material preparation owner");
        test.record(11, {dispatch});
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override { test.record(0); }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t state,
                        std::uintptr_t argument) override {
        test.record(1, {state, argument});
    }
    void release_geometry_pair(std::uint32_t vertex, std::uint32_t index) override {
        test.record(3, {vertex, index});
    }
    void release_vertex_buffer(std::uint32_t handle) override { test.record(4, {handle}); }
    void release_index_buffer(std::uint32_t handle) override { test.record(5, {handle}); }
    void release_packet(std::uint32_t handle) override { test.record(6, {handle}); }
    void free_staging(std::uintptr_t token) override {
        test.record(7, {label(token)});
        if (token) {
            const auto found = live.find(token);
            require(found != live.end(), "Staging allocation freed twice");
            found->second.validate();
            live.erase(found);
        }
    }
    void decrement_pending(std::atomic<std::uint32_t> &pending) override {
        test.record(8, {pending.load()});
        MaterialCacheLifecycleBackend::decrement_pending(pending);
    }
    std::vector<std::uint32_t> validate() const {
        std::vector<std::uint32_t> retained;
        for (auto token : test.state.staging)
            if (token) {
                require(live.contains(token), "Live staging token lost physical storage");
                retained.push_back(label(token));
            }
        require(retained.size() == live.size(), "Unowned physical staging storage");
        for (const auto &[token, allocation] : live)
            allocation.validate();
        for (const auto job : queued)
            require(job == &test.state.geometry_job && job->owner == &test.state,
                    "Queued job outlived its owner");
        std::sort(retained.begin(), retained.end());
        return retained;
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Preparation primitive input and fresh output required");
        Reader in(argv[1]);
        require(in.bytes(8) == Bytes{'C', '5', 'P', 'R', 'E', 'P', 'I', '1'}, "Preparation input schema");
        const auto count = in.word();
        require(count > 0 && count <= 20000, "Preparation case count");
        Bytes out{'C', '5', 'P', 'R', 'E', 'P', 'O', '1'};
        word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto mode = in.word(), stamp = in.word(), argument = in.word(), dispatch = in.word();
            const auto stride = in.word(), failure = in.word();
            Case test(in.bytes(1024));
            Pool pool(test);
            const auto metadata = in.word();
            for (unsigned j = 0; j < metadata; ++j) {
                const auto handle = in.word(), width = in.word();
                pool.storage.add_texture(handle, width, false);
            }
            for (auto width : widths) {
                const auto free = in.word();
                for (unsigned j = 0; j < free; ++j) {
                    const auto handle = in.word();
                    require(pool.storage.texture_width(handle) == width, "Entering free-list width mismatch");
                    pool.storage.give(handle);
                }
            }
            PreparationBackend backend(test, failure);
            material_cache_prepare(test.state,
                                   {std::bit_cast<std::int32_t>(get(test.entering, 0xd4)),
                                    std::bit_cast<std::int32_t>(get(test.entering, 0xd8)),
                                    std::bit_cast<std::int32_t>(stride)},
                                   backend, mode != 0, dispatch);
            word(out, test.state.prepared);
            const auto prepared = test.snapshot();
            out.insert(out.end(), prepared.begin(), prepared.end());
            word(out, static_cast<std::uint32_t>(test.events.size()));
            backend.validate();
            word(out, material_cache_dispatch(test.state, pool, backend, mode != 0, stamp, argument));
            const auto final = test.snapshot();
            out.insert(out.end(), final.begin(), final.end());
            word(out, static_cast<std::uint32_t>(test.events.size()));
            for (const auto &event : test.events) {
                word(out, event.kind);
                word(out, static_cast<std::uint32_t>(event.arguments.size()));
                for (const auto value : event.arguments) {
                    word(out, static_cast<std::uint32_t>(value));
                    word(out, static_cast<std::uint32_t>(value >> 32));
                }
                out.insert(out.end(), event.patch.begin(), event.patch.end());
            }
            for (auto width : widths)
                vector_words(out, pool.storage.free_handles(width));
            vector_words(out, backend.validate());
        }
        require(in.at == in.data.size(), "Trailing preparation input");
        require(!std::filesystem::exists(argv[2]), "Fresh preparation output required");
        std::ofstream stream(argv[2], std::ios::binary);
        stream.write(reinterpret_cast<const char *>(out.data()), out.size());
        require(bool(stream), "Preparation output failed");
        std::cout << "PASS native preparation/completion " << count << " cases; actual guarded allocations\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
