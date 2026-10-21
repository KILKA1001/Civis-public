#pragma once
void simulation_random_tests() {
    using runtime::RandomAlgorithm;
    using runtime::SimulationRandom;
    using runtime::SimulationRandomState;
    struct Golden {
        RandomAlgorithm algorithm;
        std::uint64_t initial_seed, bound, value, next_seed, calls;
    };
    // Independent arbitrary-precision integer vectors. Bounds beyond the
    // source API width verify Civis scaling; smaller bounds verify its sequence.
    constexpr Golden golden[] = {
        {RandomAlgorithm::bnw, 0ULL, 0ULL,
         0ULL, 12345ULL, 1ULL},
        {RandomAlgorithm::bnw, 0ULL, 1ULL,
         0ULL, 3554416254ULL, 2ULL},
        {RandomAlgorithm::bnw, 0ULL, 2ULL,
         1ULL, 2802067423ULL, 3ULL},
        {RandomAlgorithm::bnw, 0ULL, 100ULL,
         83ULL, 3596950572ULL, 4ULL},
        {RandomAlgorithm::bnw, 0ULL, 65535ULL,
         3497ULL, 229283573ULL, 5ULL},
        {RandomAlgorithm::bnw, 0ULL, 4294967295ULL,
         3256811519ULL, 3256818826ULL, 6ULL},
        {RandomAlgorithm::bnw, 0ULL, 4294967296ULL,
         1051525120ULL, 1051550459ULL, 7ULL},
        {RandomAlgorithm::bnw, 0ULL, 4294967297ULL,
         3441229824ULL, 3441282840ULL, 8ULL},
        {RandomAlgorithm::bnw, 0ULL, 18446744073709551615ULL,
         12635411704541347839ULL, 2941955441ULL, 9ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 0ULL,
         0ULL, 3191464396ULL, 1ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 1ULL,
         0ULL, 288979989ULL, 2ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 2ULL,
         0ULL, 646343466ULL, 3ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 100ULL,
         90ULL, 3898514715ULL, 4ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 65535ULL,
         8712ULL, 571035320ULL, 5ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 4294967295ULL,
         3997302783ULL, 3997353105ULL, 6ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 4294967296ULL,
         2742550528ULL, 2742554614ULL, 7ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 4294967297ULL,
         4248764416ULL, 4248825847ULL, 8ULL},
        {RandomAlgorithm::bnw, 4294967295ULL, 18446744073709551615ULL,
         11484179049794764799ULL, 2673909348ULL, 9ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 0ULL,
         0ULL, 0ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 1ULL,
         0ULL, 0ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 2ULL,
         0ULL, 12345ULL, 1ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 100ULL,
         4ULL, 13622895711870ULL, 2ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 65535ULL,
         28413ULL, 17423423094097455071ULL, 3ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 4294967295ULL,
         3325679204ULL, 9039786828775103532ULL, 4ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 4294967296ULL,
         2219707818ULL, 12744206516323260149ULL, 5ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 4294967297ULL,
         4289380895ULL, 11820541280853630090ULL, 6ULL},
        {RandomAlgorithm::vp_reference, 0ULL, 18446744073709551615ULL,
         17089259173784322047ULL, 5472134308656145147ULL, 7ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 0ULL,
         0ULL, 5078502118120654316ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 1ULL,
         0ULL, 5078502118120654316ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 2ULL,
         1ULL, 3533596733323681205ULL, 1ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 100ULL,
         46ULL, 11503168785331846218ULL, 2ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 65535ULL,
         57418ULL, 298047138159798203ULL, 3ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 4294967295ULL,
         2516340331ULL, 16811820844877354200ULL, 4ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 4294967296ULL,
         3577039214ULL, 12079733050379788337ULL, 5ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 4294967297ULL,
         4139639658ULL, 13444360608055726870ULL, 6ULL},
        {RandomAlgorithm::vp_reference, 5078502118120654316ULL, 18446744073709551615ULL,
         15990776173495844863ULL, 10649286793774859415ULL, 7ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 0ULL,
         0ULL, 18446744073709551615ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 1ULL,
         0ULL, 18446744073709551615ULL, 0ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 2ULL,
         1ULL, 18446744072606048716ULL, 1ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 100ULL,
         74ULL, 17229011800657853461ULL, 2ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 65535ULL,
         18356ULL, 973700787803155242ULL, 3ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 4294967295ULL,
         2802509917ULL, 3864835095527333147ULL, 4ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 4294967296ULL,
         830677513ULL, 6671574337277480632ULL, 5ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 4294967297ULL,
         3110334018ULL, 14274644332777751697ULL, 6ULL},
        {RandomAlgorithm::vp_reference, 18446744073709551615ULL, 18446744073709551615ULL,
         11363043070623875071ULL, 17454155217128394742ULL, 7ULL},
    };
    std::optional<SimulationRandom> generator;
    std::uint64_t previous_initial = 0;
    auto previous_algorithm = RandomAlgorithm::bnw;
    for (const auto &row : golden) {
        if (!generator || row.initial_seed != previous_initial || row.algorithm != previous_algorithm) {
            generator.emplace(row.algorithm, SimulationRandomState{row.initial_seed, 0, 7});
            previous_initial = row.initial_seed;
            previous_algorithm = row.algorithm;
        }
        check(generator->draw(row.bound) == row.value, "Simulation RNG value disagrees with integer oracle");
        check(generator->state() == SimulationRandomState{row.next_seed, row.calls, 7},
              "Simulation RNG state disagrees with integer oracle");
    }
    for (const auto algorithm : {RandomAlgorithm::bnw, RandomAlgorithm::vp_reference}) {
        SimulationRandom wide(algorithm, {99, 0xffffffffULL, 0xffffffffULL});
        (void)wide.draw(100);
        check(wide.state().calls == 0x100000000ULL, "Simulation RNG call count retained x86 rollover");
        wide.reseed(123);
        check(wide.state().resets == 0x100000000ULL && wide.state().calls == 0x100000000ULL,
              "Simulation RNG reseed truncated counts or reset calls");
        wide.reset(123);
        check(wide.state().calls == (algorithm == RandomAlgorithm::bnw ? 0x100000000ULL : 0) &&
                  wide.state().resets == (algorithm == RandomAlgorithm::bnw ? 0x100000001ULL : 0x100000000ULL),
              "Source reset semantics changed for an unchanged seed");
        wide.reset(124);
        check(wide.state().seed == 124 && wide.state().resets ==
                  (algorithm == RandomAlgorithm::bnw ? 0x100000002ULL : 0x100000001ULL),
              "Source reset semantics changed for a new seed");
        SimulationRandom exhausted(algorithm, {1, UINT64_MAX, UINT64_MAX});
        const auto before = exhausted.state();
        rejects([&] { (void)exhausted.draw(100); });
        rejects([&] { exhausted.reseed(2); });
        rejects([&] { exhausted.reset(2); });
        check(exhausted.state() == before, "Rejected RNG operation partially mutated state");
    }
    SimulationRandom bnw(RandomAlgorithm::bnw, {0xffffffffULL, 100, 9});
    check(std::bit_cast<std::uint32_t>(bnw.draw_float()) == 1061042366U,
          "BNW floating-point draw changed");
    SimulationRandom vp(RandomAlgorithm::vp_reference, {5078502118120654316ULL, 100, 9});
    check(std::bit_cast<std::uint32_t>(vp.draw_float()) == 1062961621U,
          "VP floating-point draw changed");
    rejects([] { SimulationRandom bad(RandomAlgorithm::bnw, {0x100000000ULL, 0, 0}); });
    rejects([] { SimulationRandom bad(static_cast<RandomAlgorithm>(99), {}); });
    const auto before = bnw.state();
    rejects([&] { bnw.reseed(0x100000000ULL); });
    rejects([&] { bnw.reset(0x100000000ULL); });
    check(bnw.state() == before, "Invalid BNW residue truncated the previous state");

    saves::GameProgressionObservation progression{};
    progression.layout = saves::GameStateLayout::vp542_corpus;
    progression.random = {{{15676209089797375409ULL, 912, 18}, {5078502118120654316ULL, 876, 22}}};
    auto streams = runtime::restore_session_random(progression);
    check(streams.map.state() == SimulationRandomState{15676209089797375409ULL, 912, 18} &&
              streams.gameplay.state() == SimulationRandomState{5078502118120654316ULL, 876, 22},
          "Restoring RNG swapped streams, advanced seed or reset counts");
    const auto original_map = streams.map.state();
    (void)streams.gameplay.draw(1000);
    check(streams.map.state() == original_map && progression.random[1].calls == 876,
          "Gameplay RNG changed map RNG or the immutable save observation");
    SimulationRandom resumed(streams.gameplay.algorithm(), streams.gameplay.state());
    for (std::uint64_t range = 0; range < 100; ++range)
        check(resumed.draw(range) == streams.gameplay.draw(range),
              "Restored RNG failed to continue the same sequence");
    check(resumed.state() == streams.gameplay.state(), "Resumed RNG counts diverged");
    const runtime::SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto checkpoint=runtime::make_simulation_random_checkpoint(streams,identity);
    runtime::SessionRandomStreams restored{
        SimulationRandom(RandomAlgorithm::vp_reference,{1,2,3}),
        SimulationRandom(RandomAlgorithm::vp_reference,{4,5,6})};
    runtime::restore_simulation_random_checkpoint(checkpoint,restored,identity);
    check(restored.map.state()==streams.map.state()&&restored.gameplay.state()==streams.gameplay.state(),
          "Simulation RNG checkpoint lost either live stream");
    for(std::uint64_t range=0;range<20;++range){
        check(restored.map.draw(range+2)==streams.map.draw(range+2),
              "Checkpointed map RNG sequence diverged");
        check(restored.gameplay.draw(range+2)==streams.gameplay.draw(range+2),
              "Checkpointed gameplay RNG sequence diverged");
    }
    const auto stable_map=restored.map.state(),stable_gameplay=restored.gameplay.state();
    rejects([&]{runtime::restore_simulation_random_checkpoint(checkpoint,restored,
        {identity.payload_sha256,std::string(64,'c')});});
    auto truncated=checkpoint;truncated.bytes.pop_back();
    rejects([&]{runtime::restore_simulation_random_checkpoint(truncated,restored,identity);});
    auto wrong_algorithm=checkpoint;wrong_algorithm.bytes[168]=1;
    rejects([&]{runtime::restore_simulation_random_checkpoint(wrong_algorithm,restored,identity);});
    check(restored.map.state()==stable_map&&restored.gameplay.state()==stable_gameplay,
          "Rejected simulation RNG checkpoint partially changed live state");
    runtime::SessionRandomStreams bnw_streams{
        SimulationRandom(RandomAlgorithm::bnw,{UINT32_MAX,0x100000000ULL,0x100000001ULL}),
        SimulationRandom(RandomAlgorithm::bnw,{0,0x100000002ULL,0x100000003ULL})};
    const auto bnw_checkpoint=runtime::make_simulation_random_checkpoint(bnw_streams,identity);
    runtime::SessionRandomStreams bnw_restored{
        SimulationRandom(RandomAlgorithm::bnw,{}),SimulationRandom(RandomAlgorithm::bnw,{})};
    runtime::restore_simulation_random_checkpoint(bnw_checkpoint,bnw_restored,identity);
    check(bnw_restored.map.state()==bnw_streams.map.state()&&
          bnw_restored.gameplay.state()==bnw_streams.gameplay.state(),
          "BNW checkpoint truncated wide host-side RNG counters");
    auto invalid_bnw_seed=bnw_checkpoint;invalid_bnw_seed.bytes[176]=1;
    rejects([&]{runtime::restore_simulation_random_checkpoint(invalid_bnw_seed,bnw_restored,identity);});
    check(bnw_restored.map.state()==bnw_streams.map.state()&&
          bnw_restored.gameplay.state()==bnw_streams.gameplay.state(),
          "Invalid second RNG stream partially replaced the first one");
    progression.layout = static_cast<saves::GameStateLayout>(99);
    rejects([&] { (void)runtime::restore_session_random(progression); });
}
