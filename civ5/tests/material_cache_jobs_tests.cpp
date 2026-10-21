#include "../validation/material_cache_jobs_fixture.hpp"
#include <bit>
#include <cstring>
#include <iostream>

using namespace material_jobs_validation;
int main() {
    try {
        for (unsigned ready = 0; ready < 2; ++ready)
            for (unsigned failure = 0; failure < 8; ++failure) {
                MaterialCacheLifecycleState state;
                state.prepared = ready;
                Backend backend(state, {11, 23, 0});
                backend.failure_mask = failure;
                material_cache_prepare_material_jobs(state, backend.domain, backend, 0x1234567812345678ull);
                backend.validate_prepared();
                require(backend.allocation_count == 3 && state.prepared == (ready && !failure),
                        "Material allocation refusal or readiness changed");
                if (!failure) {
                    require(backend.queued.size() == 6 && state.staging[2] > UINT32_MAX,
                            "Material native record ownership or tile count changed");
                    material_cache_clear_staging(state, backend);
                    backend.validate_released();
                }
            }
        MaterialCacheLifecycleState state;
        state.prepared = true;
        state.textures.final_size = 128;
        Backend backend(state, {1, 1, 0});
        material_cache_prepare_material_jobs(state, backend.domain, backend, 0);
        const auto normal_token = state.staging[4];
        state.staging[4] = 0;
        const MaterialGeometryDomain domain{
            3, 3, {0, 0}, {32, 32, {1, 1, 1}, 0}, 64, civ5::terrain::X87Precision::double_precision};
        const MaterialSelection selected(std::array<std::uint32_t, 1>{1});
        backend.execute(
            domain, selected, [](int, int) { return 10000; },
            [](int, int) { return MaterialVector3{0, 0, 1}; },
            [](int, int, int) { return std::vector<float>{1}; });
        const auto normal = backend.staging_memory(normal_token);
        float x = 0, y = 0;
        std::memcpy(&x, normal.data(), 4);
        std::memcpy(&y, normal.data() + 4, 4);
        require(x == -1.f - 1.f / 128 && y == 1.f + 1.f / 128,
                "Worker lost captured normal token or used stale final-cache size");
        state.staging[4] = normal_token;
        backend.offsets = {5, 7, 11};
        material_cache_append_material_jobs(state, backend.domain, backend);
        backend.validate_released();
        require(state.material_stream_offsets == backend.offsets &&
                    backend.cursors == std::array<std::uint32_t, 3>{14, 16, 35},
                "Appended stream offsets or counts changed");
        require(backend.appended[0].size() == 9 * 48 && backend.appended[1].size() == 9 * 24 &&
                    backend.appended[2].size() == 24 * 2,
                "Appended material stream extent changed");
        const MaterialLookup height_at = [](int, int) { return 10000; };
        const MaterialNormalLookup normal_at = [](int, int) { return MaterialVector3{0, 0, 1}; };
        const MaterialFactorLookup factors_at = [](int, int, int) { return std::vector<float>{1}; };
        for (const auto size : {std::array{2, 2}, std::array{13, 25}, std::array{67, 67}}) {
            auto owned_domain = domain;
            owned_domain.columns = size[0];
            owned_domain.rows = size[1];
            const auto direct =
                build_material_patch(owned_domain, selected, height_at, normal_at, factors_at);
            const auto owned =
                build_material_patch_jobs(owned_domain, selected, height_at, normal_at, factors_at);
            require(owned.columns == direct.columns && owned.rows == direct.rows &&
                        owned.streams.material48 == direct.streams.material48 &&
                        owned.streams.normal24 == direct.streams.normal24 && owned.indices == direct.indices,
                    "Owned material job boundary output changed");
        }
        auto failing_domain = domain;
        failing_domain.columns = 13;
        failing_domain.rows = 25;
        int visits = 0;
        bool threw = false;
        try {
            build_material_patch_jobs(
                failing_domain, selected,
                [&](int, int) {
                    if (++visits == 145)
                        throw std::runtime_error("Material source unavailable");
                    return 10000;
                },
                normal_at, factors_at);
        } catch (const std::runtime_error &error) {
            threw = std::string_view(error.what()) == "Material source unavailable";
        }
        require(threw && visits == 145, "A failed material job published a partial patch");
        const auto recovered =
            build_material_patch_jobs(failing_domain, selected, height_at, normal_at, factors_at);
        const auto reference =
            build_material_patch(failing_domain, selected, height_at, normal_at, factors_at);
        require(recovered.streams.material48 == reference.streams.material48 &&
                    recovered.streams.normal24 == reference.streams.normal24 &&
                    recovered.indices == reference.indices,
                "Failed material job affected the next owned patch");
        MaterialCacheLifecycleState persistent;
        persistent.prepared = true;
        persistent.textures.final_size = failing_domain.cache_size;
        persistent.staging[0] = 0x1234567812345678ull;
        persistent.staging[1] = 0x8765432187654321ull;
        persistent.pending_jobs = 7;
        persistent.requested_state = 3;
        persistent.completed_state = 1;
        persistent.frame_stamp = 91;
        visits = 0;
        threw = false;
        try {
            build_material_patch_jobs(
                persistent, failing_domain, selected,
                [&](int, int) {
                    require(persistent.staging[2] && persistent.staging[3] && persistent.staging[4],
                            "Material worker lost the persistent staging owner");
                    if (++visits == 145)
                        throw std::runtime_error("Persistent source unavailable");
                    return 10000;
                },
                normal_at, factors_at);
        } catch (const std::runtime_error &error) {
            threw = std::string_view(error.what()) == "Persistent source unavailable";
        }
        require(threw && visits == 145 && !persistent.prepared && !persistent.staging[2] &&
                    !persistent.staging[3] && !persistent.staging[4] &&
                    persistent.staging[0] == 0x1234567812345678ull &&
                    persistent.staging[1] == 0x8765432187654321ull && persistent.pending_jobs == 7 &&
                    persistent.requested_state == 3 && persistent.completed_state == 1 &&
                    persistent.frame_stamp == 91,
                "Persistent worker failure left stale tokens or changed outer "
                "lifecycle state");
        persistent.prepared = true;
        const auto retried =
            build_material_patch_jobs(persistent, failing_domain, selected, height_at, normal_at, factors_at);
        require(retried.streams.material48 == reference.streams.material48 &&
                    retried.streams.normal24 == reference.streams.normal24 &&
                    retried.indices == reference.indices && persistent.prepared && !persistent.staging[2] &&
                    !persistent.staging[3] && !persistent.staging[4],
                "Persistent owner did not recover after a producer exception");
        {
            MaterialCacheJobBatch first(persistent, failing_domain.columns, failing_domain.rows);
            const auto tokens = persistent.staging;
            require(&first.state() == &persistent && first.job_count() == 6 && !first.executed(),
                    "Retained batch lost its original patch identity or submissions");
            bool early_append = false, overlap = false;
            try {
                (void)first.append();
            } catch (const std::logic_error &) {
                early_append = true;
            }
            try {
                MaterialCacheJobBatch second(persistent, failing_domain.columns, failing_domain.rows);
            } catch (const std::invalid_argument &) {
                overlap = true;
            }
            require(early_append && overlap && persistent.staging == tokens && persistent.prepared,
                    "Early append or overlapping preparation damaged the live batch");
            MaterialCacheLifecycleState neighbour;
            neighbour.prepared = true;
            neighbour.textures.final_size = domain.cache_size;
            MaterialCacheJobBatch other(neighbour, domain.columns, domain.rows);
            other.execute(domain, selected, height_at, normal_at, factors_at);
            first.execute(failing_domain, selected, height_at, normal_at, factors_at);
            require(first.executed() && other.executed() && persistent.staging == tokens &&
                        neighbour.staging[2] && neighbour.staging[3] && neighbour.staging[4] &&
                        persistent.pending_jobs == 7 && persistent.frame_stamp == 91,
                    "Worker barrier appended early or changed lifecycle metadata");
            const auto appended = first.append();
            const auto neighbour_patch = other.append();
            const auto neighbour_reference =
                build_material_patch(domain, selected, height_at, normal_at, factors_at);
            require(appended.streams.material48 == reference.streams.material48 &&
                        appended.streams.normal24 == reference.streams.normal24 &&
                        appended.indices == reference.indices &&
                        neighbour_patch.streams.material48 == neighbour_reference.streams.material48 &&
                        neighbour_patch.streams.normal24 == neighbour_reference.streams.normal24 &&
                        neighbour_patch.indices == neighbour_reference.indices && persistent.prepared &&
                        neighbour.prepared && !persistent.staging[2] && !neighbour.staging[2],
                    "Interleaved retained batches aliased storage or changed output");
            bool twice = false;
            try {
                (void)first.append();
            } catch (const std::logic_error &) {
                twice = true;
            }
            require(twice && persistent.prepared, "Appended batch was published twice");
        }
        {
            MaterialCacheJobBatch first(persistent, failing_domain.columns, failing_domain.rows, 0, true);
            MaterialCacheJobBatch second(persistent, failing_domain.columns, failing_domain.rows, 0, true);
            require(persistent.prepared && persistent.staging[2] == 0 && persistent.staging[3] == 0 &&
                        persistent.staging[4] == 0 && first.job_count() == second.job_count(),
                    "Independent repeated batches replaced the logical owner's staging");
            first.execute(failing_domain, selected, height_at, normal_at, factors_at);
            second.execute(failing_domain, selected, height_at, normal_at, factors_at);
            const auto first_patch = first.append();
            const auto second_patch = second.append();
            require(first_patch.streams.material48 == reference.streams.material48 &&
                        first_patch.streams.normal24 == reference.streams.normal24 &&
                        first_patch.indices == reference.indices &&
                        second_patch.streams.material48 == reference.streams.material48 &&
                        second_patch.streams.normal24 == reference.streams.normal24 &&
                        second_patch.indices == reference.indices && persistent.prepared &&
                        persistent.staging[2] == 0 && persistent.staging[3] == 0 && persistent.staging[4] == 0,
                    "Independent repeated batches aliased or changed material output");
        }
        {
            MaterialCacheJobBatch cancelled(persistent, failing_domain.columns, failing_domain.rows);
            cancelled.execute(failing_domain, selected, height_at, normal_at, factors_at);
        }
        require(!persistent.prepared && !persistent.staging[2] && !persistent.staging[3] &&
                    !persistent.staging[4] && persistent.staging[0] == 0x1234567812345678ull &&
                    persistent.staging[1] == 0x8765432187654321ull && persistent.pending_jobs == 7 &&
                    persistent.frame_stamp == 91,
                "Abandoned completed workers left stale staging or changed external ownership");
        std::cout << "PASS material job allocation failures, native records, "
                     "captured storage and append "
                     "ownership\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
