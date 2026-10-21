#include "civ5/runtime/bnw_plot_build_progress_diagnostic.hpp"
#include "civ5/content/info_types.hpp"
#include "bnw_plot_build_progress_source_fixture.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
std::size_t checks=0;
void check(bool value,const char *why) {
    ++checks;
    if(!value) throw std::runtime_error(why);
}
template<class F> void rejects(F function,std::string_view reason) {
    try {function();} catch(const std::invalid_argument &e) {
        check(std::string_view(e.what()).find(reason)!=std::string_view::npos,"wrong refusal reason");
        return;
    }
    throw std::runtime_error("invalid source input accepted");
}
}
int main() {
    try {
        using Info=std::optional<BnwDiagnosticBuildInfo>;
        using Record=BnwDiagnosticSavedBuildProgress;
        for(std::size_t i=0;i<source_fixture::names.size();++i)
            check(content::saved_info_type_hash(source_fixture::names[i])==source_fixture::hashes[i],
                  "independently source-derived Build identity changed");
        const auto farm=source_fixture::hashes[0],mine=source_fixture::hashes[1];
        const auto road=source_fixture::hashes[2],rail=source_fixture::hashes[3];
        // Fixture slot order is explicitly controlled, not base XML/native GC
        // order. Improvement IDs 77/88 are controlled transport identities.
        const std::array<Info,5> slots{BnwDiagnosticBuildInfo{farm,77,-1},std::nullopt,
            BnwDiagnosticBuildInfo{mine,88,-1},BnwDiagnosticBuildInfo{road,-1,0},
            BnwDiagnosticBuildInfo{rail,-1,1}};
        const std::array<Record,7> records{Record{mine,9},Record{rail,7},Record{0,{}},
            Record{farm,3},Record{mine,-4},Record{road,2},Record{farm,12}};
        auto restored=restore_bnw_plot_build_progress_diagnostic(1,slots,records);
        check(restored.allocated && restored.builds.size()==5,"dense selected Build extent");
        check(restored.builds[0].progress==12 && restored.builds[2].progress==-4,
              "later duplicate source writes must win, including negative values");
        check(restored.builds[1].improvement==-1 && restored.builds[1].route==-1 &&
              restored.builds[1].progress==0,"unavailable GC Build slot retained");
        check(!restored.can_publish_live(),"diagnostic transform promoted to live owner");
        terrain::PlotGraphicsInputs input;
        input.any_build_progress=restored.allocated;
        input.builds=restored.builds;
        check(terrain::derive_plot_graphics_state(input)==terrain::PlotGraphicsState{-1,77,1,0,4},
              "existing producer must scan GC slots, not wire record order");
        input.fog=terrain::PlotFogMode::explored;
        check(terrain::derive_plot_graphics_state(input)==terrain::PlotGraphicsState{-1,-1,2,-1,8},
              "hidden construction leaked");
        input.fog=terrain::PlotFogMode::visible;
        input.improvement_pillaged=true;
        check(terrain::derive_plot_graphics_state(input).improvement_state==3,"source pillage priority");
        auto empty=restore_bnw_plot_build_progress_diagnostic(8,slots,{});
        check(empty.allocated && empty.builds[0].progress==0,"allocated empty nested record array");
        auto absent=restore_bnw_plot_build_progress_diagnostic(0,slots,{});
        check(!absent.allocated && absent.builds.size()==5,"null versus zeroed allocated array");
        check(!restore_bnw_plot_build_progress_diagnostic(-1,slots,{}).allocated,
              "source nonpositive allocation gate");
        const std::array<Record,2> limits{Record{farm,-32768},Record{mine,32767}};
        auto bounded=restore_bnw_plot_build_progress_diagnostic(3,slots,limits);
        check(bounded.builds[0].progress==-32768 && bounded.builds[2].progress==32767,
              "signed short endpoints changed");
        auto alternative=slots;
        std::swap(alternative[0],alternative[2]);
        auto reordered=restore_bnw_plot_build_progress_diagnostic(1,alternative,records);
        check(reordered.builds[0].improvement==88 && reordered.builds[0].progress==-4 &&
              reordered.builds[2].progress==12,"selected-slot hash remapping lost");
        restored.builds[0].progress=999;
        check(restore_bnw_plot_build_progress_diagnostic(1,slots,records).builds[0].progress==12,
              "output aliases original inputs");
        rejects([&]{restore_bnw_plot_build_progress_diagnostic(0,slots,records);},"unallocated");
        for(const auto bad:std::array<Record,5>{Record{0,5},Record{farm,{}},Record{farm,32768},
                Record{farm,-32769},Record{0x12345678,1}}) {
            const std::array one{bad};
            rejects([&]{restore_bnw_plot_build_progress_diagnostic(1,slots,one);},
                bad.type_hash==0?"null Build":bad.type_hash==0x12345678?"unknown Build":"source short");
        }
        auto collision=slots;collision[2]=collision[0];
        rejects([&]{restore_bnw_plot_build_progress_diagnostic(1,collision,{});},"ambiguous");
        auto null_info=slots;null_info[0]=BnwDiagnosticBuildInfo{0,77,-1};
        rejects([&]{restore_bnw_plot_build_progress_diagnostic(1,null_info,{});},"invalid");
        auto bad_kind=slots;bad_kind[0]=BnwDiagnosticBuildInfo{farm,-2,-1};
        rejects([&]{restore_bnw_plot_build_progress_diagnostic(1,bad_kind,{});},"invalid");
        rejects([&]{restore_bnw_plot_build_progress_diagnostic(INT64_MAX,slots,{});},"int range");
        check(records[0].value==9 && slots[0]->improvement==77,"input fixture mutated");
        std::cout << "PASS " << checks << " diagnostic Build restore checks; live publication=false\n";
        return 0;
    } catch(const std::exception &e) {std::cerr << "FAIL " << e.what() << '\n';return 1;}
}
