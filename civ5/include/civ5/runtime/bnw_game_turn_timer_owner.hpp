#pragma once
#include <cstdint>
#include <memory>
#include <utility>

namespace civ5::runtime {
class LiveSession;class BnwRetainedTurnCursor;
class BnwGameTurnTimerOwner;
enum class BnwGameTurnTimerReset { construction, engine_begin, current_player };

// Sealed source-use handle for an operation actually performed by this owner.
// Copying it permits reading the same operation, never acknowledging UI/DoTurn.
class BnwGameTurnTimerUse {
  public:
    BnwGameTurnTimerUse(const BnwGameTurnTimerUse&)=default;
    BnwGameTurnTimerUse& operator=(const BnwGameTurnTimerUse&)=default;
  private:
    struct Data;
    explicit BnwGameTurnTimerUse(std::shared_ptr<const Data> data):data_(std::move(data)){}
    std::shared_ptr<const Data> data_;
    friend class BnwGameTurnTimerOwner;
};
struct BnwGameTurnTimerSample {
    float current_elapsed=0,game_turn_elapsed=0,pause_delta=0;
    float current_stopped_elapsed=0,game_turn_stopped_elapsed=0;
    std::int64_t current_start=0,game_turn_start=0,frequency=0;
    std::uint64_t operation=0;
    BnwGameTurnTimerReset reset=BnwGameTurnTimerReset::construction;
};

struct BnwEndTurnTimerSample {
    std::int64_t start=0,frequency=0;
    std::int32_t semaphore=0;
    float stopped_elapsed=0,elapsed=0;
    std::uint64_t reset_occurrence=0;
    bool running=false;
};

// Own standalone Windows timers, bound to one actual session allocation/turn.
// No native engine authority, caller timestamps, save import of QPC marks,
// UI acknowledgement, scheduler handler or full-turn completion is supplied.
class BnwGameTurnTimerOwner final {
  public:
    static std::shared_ptr<BnwGameTurnTimerOwner> create(
        std::shared_ptr<const LiveSession> session);
    ~BnwGameTurnTimerOwner();
    BnwGameTurnTimerOwner(const BnwGameTurnTimerOwner&)=delete;
    BnwGameTurnTimerOwner& operator=(const BnwGameTurnTimerOwner&)=delete;
    BnwGameTurnTimerUse current_use() const;
    // CvGame::doTurn resetTurnTimer(true), once per bound turn/owner.
    BnwGameTurnTimerUse reset_for_engine_begin();
    // CvGame::resetTurnTimer(false): current timer + pause delta only.
    BnwGameTurnTimerUse reset_for_current_player();
    BnwGameTurnTimerSample read(const BnwGameTurnTimerUse&) const;
    BnwEndTurnTimerSample read_end_turn_timer()const;
    void require_current() const;
    void retire() noexcept;
  private:
    friend class BnwRetainedTurnCursor;
    void adopt_actual_game_turn_clock_return(const BnwRetainedTurnCursor&);
    BnwEndTurnTimerSample reset_actual_end_turn_timer(const BnwRetainedTurnCursor&);
    void require_actual_end_turn_timer_return(const BnwRetainedTurnCursor&,const BnwEndTurnTimerSample&)const;
    struct Impl;
    explicit BnwGameTurnTimerOwner(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
    BnwGameTurnTimerUse reset(BnwGameTurnTimerReset);
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
