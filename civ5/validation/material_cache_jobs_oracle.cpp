#include "material_cache_jobs_fixture.hpp"
#include "material_generation_fixture.hpp"

namespace gv = generation_validation;
namespace jv = material_jobs_validation;
namespace lv = cache_lifecycle_validation;
namespace {
void events(gv::Bytes &out, const std::vector<jv::Event> &items, bool snapshots) {
    gv::word(out, static_cast<std::uint32_t>(items.size()));
    for (const auto &event : items) {
        gv::word(out, event.kind);
        gv::word(out, static_cast<std::uint32_t>(event.arguments.size()));
        for (auto value : event.arguments) {
            gv::word(out, static_cast<std::uint32_t>(value));
            gv::word(out, static_cast<std::uint32_t>(value >> 32));
        }
        if (snapshots) {
            gv::require(event.patch.size() == 1024, "Missing callback snapshot");
            out.insert(out.end(), event.patch.begin(), event.patch.end());
        }
    }
}
gv::Bytes allocation_cases(gv::Reader input) {
    input.take(8);
    const auto count = input.word();
    gv::require(count > 0 && count <= 1000, "Material allocation case count");
    gv::Bytes out{'C', '5', 'J', 'A', 'L', 'C', 'O', '1'};
    gv::word(out, count);
    for (unsigned i = 0; i < count; ++i) {
        const auto failure = input.word(), dispatch = input.word();
        const auto entering = input.take(1024);
        lv::Case test(gv::Bytes(entering.begin(), entering.end()));
        jv::Backend backend(test.state,
                            {static_cast<int>(cache_acquisition_validation::get(test.entering, 0xd4)),
                             static_cast<int>(cache_acquisition_validation::get(test.entering, 0xd8)), 0});
        backend.failure_mask = failure;
        backend.observe(test);
        material_cache_prepare_material_jobs(test.state, backend.domain, backend, dispatch);
        backend.validate_prepared();
        const auto final = test.snapshot();
        out.insert(out.end(), final.begin(), final.end());
        events(out, backend.events, true);
        gv::word(out, static_cast<std::uint32_t>(backend.records.size()));
        gv::word(out, static_cast<std::uint32_t>(backend.queued.size()));
        std::vector<std::uint32_t> live;
        if (!backend.records.empty())
            live.push_back(backend.label(reinterpret_cast<std::uintptr_t>(backend.records.data())));
        for (const auto &[token, bytes] : backend.buffers)
            live.push_back(backend.label(token));
        std::sort(live.begin(), live.end());
        gv::vector(out, std::span<const std::uint32_t>(live));
    }
    input.end();
    return out;
}
gv::Bytes full_patch(gv::Reader input, gv::Bytes &observations) {
    gv::Bytes out;
    const gv::Source source(input);
    MaterialGeometryDomain domain{};
    auto scores = gv::scores(input, source, domain, out);
    gv::NormalTrace trace{source, domain.source, input.values<float>(4356), {}, 0};
    input.end();
    MaterialCacheLifecycleState state;
    state.textures.final_size = static_cast<std::uint32_t>(domain.cache_size);
    state.requested_state = 3;
    state.completed_state = 0;
    state.pending_jobs.store(1);
    jv::Backend backend(state, {domain.columns - 2, domain.rows - 2, 3});
    material_cache_prepare(state, backend.domain, backend, true, 0x24000000);
    gv::require(state.prepared, "Full material job preparation failed");
    backend.validate_prepared();
    std::vector<MaterialTile> tiles;
    for (const auto job : backend.queued)
        tiles.push_back(job->tile);
    backend.execute(
        domain, scores.selected, [&](int x, int y) { return source.height_at(x, y); },
        [&](int x, int y) { return trace.normal(x, y); },
        [&](int x, int y, int axis) { return source.factors(x, y, axis); });
    trace.end();
    MaterialCacheFreeLists pool;
    gv::require(material_cache_dispatch(state, pool, backend, true, 0xabcd1234, 0x42434445),
                "Full material completion failed");
    backend.validate_released();
    gv::NormalTrace owned_trace{source, domain.source, trace.seeds, {}, 0};
    const auto owned = build_material_patch_jobs(
        domain, scores.selected, [&](int x, int y) { return source.height_at(x, y); },
        [&](int x, int y) { return owned_trace.normal(x, y); },
        [&](int x, int y, int axis) { return source.factors(x, y, axis); });
    owned_trace.end();
    const auto index_bytes = std::as_bytes(std::span(owned.indices));
    gv::require(owned.columns == domain.columns && owned.rows == domain.rows &&
                    owned_trace.trace == trace.trace && owned.streams.material48 == backend.appended[0] &&
                    owned.streams.normal24 == backend.appended[1] &&
                    index_bytes.size() == backend.appended[2].size() &&
                    std::memcmp(index_bytes.data(), backend.appended[2].data(), index_bytes.size()) == 0,
                "Owned material job runner differs from observed lifecycle output");
    gv::blob(out, trace.trace);
    gv::word(out, static_cast<std::uint32_t>(tiles.size()));
    for (const auto tile : tiles) {
        for (int value : {tile.x0, tile.x1, tile.y0, tile.y1, tile.vertex_count()})
            gv::word(out, static_cast<std::uint32_t>(value));
    }
    for (const auto &stream : backend.appended)
        gv::blob(out, stream);
    gv::word(observations, domain.columns);
    gv::word(observations, domain.rows);
    gv::word(observations, backend.executed_jobs);
    gv::word(observations, state.completed_state);
    gv::word(observations, state.pending_jobs.load());
    gv::word(observations, state.frame_stamp);
    for (auto value : state.material_stream_offsets)
        gv::word(observations, value);
    for (auto value : backend.cursors)
        gv::word(observations, value);
    events(observations, backend.events, false);
    return out;
}
} // namespace
int main(int argc, char **argv) {
    try {
        gv::require(argc == 3 || argc == 4,
                    "Material jobs input, fresh output and optional observations required");
        const auto input = gv::read(argv[1]);
        const auto magic = std::string_view(reinterpret_cast<const char *>(input.data()), 8);
        if (magic == "C5JALCI1") {
            gv::require(argc == 3, "Unexpected allocation observation output");
            gv::write(argv[2], allocation_cases(gv::Reader{input}));
        } else {
            gv::require(magic == "C5MATG01" && argc == 4, "Material generation schema or observation output");
            const auto records = gv::records(input);
            gv::Bytes out(input.begin(), input.begin() + 8),
                observations{'C', '5', 'J', 'W', 'O', 'R', 'K', '1'};
            const auto patches =
                std::count_if(records.begin(), records.end(), [](auto &r) { return r.op == 5; });
            gv::word(observations, static_cast<std::uint32_t>(patches));
            gv::word(out, static_cast<std::uint32_t>(records.size()));
            for (const auto &record : records) {
                const auto result = record.op == 5 ? full_patch(gv::Reader{record.input}, observations)
                                                   : gv::run(record.op, gv::Reader{record.input});
                gv::word(out, record.op);
                gv::blob(out, record.label);
                gv::blob(out, result);
            }
            gv::write(argv[2], out);
            gv::write(argv[3], observations);
        }
        std::cout << "PASS material job allocation, execution and append output\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
