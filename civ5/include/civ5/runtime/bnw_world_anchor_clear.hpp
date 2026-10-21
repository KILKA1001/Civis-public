#pragma once

#include "civ5/runtime/live_action_transaction.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwWorldAnchorRemovalEvent {
    std::uint64_t plot=0;
    std::int64_t old_anchor=-1,old_data=-1;
    std::int64_t x=0,y=0;
    bool add=false;
    std::int64_t data=-1; // CvPlot::SetWorldAnchor default iData1, NOT old_data.
    std::array<std::uint8_t,16> map_guid{};
    std::uint64_t map_width=0,map_height=0;
    bool operator==(const BnwWorldAnchorRemovalEvent &) const = default;
};
struct BnwWorldAnchorClearPlan {
    bool valid=false;
    std::optional<BnwWorldAnchorRemovalEvent> removal;
    std::string reason;
};

// The emitter is the host's equivalent of gDLL->GameplayWorldAnchor. The
// current Civis graphics bridge does not provide it; callers must supply an
// actual live graphics implementation before staging an occupied anchor.
using BnwWorldAnchorEmitter=std::function<void(const BnwWorldAnchorRemovalEvent &)>;

enum class BnwWorldAnchorPublicationStatus {complete,rejected,uncertain};
struct BnwWorldAnchorPublication {
    BnwWorldAnchorPublicationStatus status=BnwWorldAnchorPublicationStatus::rejected;
    std::size_t delivered=0;
    std::string reason;
};

class BnwWorldAnchorJournal {
  public:
    explicit BnwWorldAnchorJournal(BnwWorldAnchorEmitter emitter={});
    bool has_emitter() const noexcept {return static_cast<bool>(emitter_);}
    const std::vector<BnwWorldAnchorRemovalEvent> &pending() const noexcept {return events_;}
    std::size_t delivered() const noexcept {return delivered_;}
    bool uncertain() const noexcept {return uncertain_;}
    // Invoke only after the enclosing detached game turn commits. A throwing
    // engine callback might have performed its effect, so an uncertain event
    // is never replayed. Retain this journal until reconciliation.
    BnwWorldAnchorPublication publish(const LiveSessionState &committed);
  private:
    friend LiveActionResult stage_bnw_world_anchor_clear(
        LiveSessionState &,std::uint64_t,BnwWorldAnchorJournal &);
    BnwWorldAnchorEmitter emitter_;
    std::vector<BnwWorldAnchorRemovalEvent> events_;
    std::size_t delivered_=0;
    bool uncertain_=false;
};

// Read-only source-range and raw/projection identity check for one plot.
BnwWorldAnchorClearPlan plan_bnw_world_anchor_clear(
    const LiveSessionState &state,std::uint64_t plot);

// CvPlot::SetWorldAnchor(NO_WORLD_ANCHOR) after verifyUnitValidPlot. The
// caller owns a detached LiveSessionState and journal, discarding both on an
// enclosing turn rejection. No external callback runs during this stage.
LiveActionResult stage_bnw_world_anchor_clear(
    LiveSessionState &draft,std::uint64_t plot,BnwWorldAnchorJournal &journal);

} // namespace civ5::runtime
