#pragma once

#include "civ5/runtime/bnw_world_anchor_clear.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace civ5::runtime {

struct BnwWorldAnchorGraphicsIdentity {
    std::array<std::uint8_t,16> map_guid{};
    std::uint64_t width=0,height=0;
    bool operator==(const BnwWorldAnchorGraphicsIdentity &) const = default;
};

// Implement on the retained world-scene owner when it supports the SDK's
// GameplayWorldAnchor call. The current GlMaterialWorldRenderer draws LAND
// and has no world-anchor object owner; the 2D WorldView XML WorldAnchor is
// a different UI element. The owner must remain alive through publication.
class BnwWorldAnchorGraphicsOwner {
  public:
    virtual ~BnwWorldAnchorGraphicsOwner()=default;
    virtual bool supports_gameplay_world_anchor() const=0;
    virtual BnwWorldAnchorGraphicsIdentity current_world() const=0;
    virtual bool publication_context_current() const=0;
    virtual void gameplay_world_anchor(std::int32_t anchor,bool add,
        std::int32_t x,std::int32_t y,std::int32_t data)=0;
};

struct BnwWorldAnchorGraphicsBinding {
    BnwWorldAnchorEmitter emitter;
    std::string reason;
    bool ready() const noexcept {return static_cast<bool>(emitter);}
};

// Bind during map-turn preflight, then give emitter to BnwWorldAnchorJournal.
// The owner identity and context are checked again immediately before each
// after-commit engine call. Missing graphics capability rejects before the
// map state changes; a stale owner after commit becomes journal uncertainty.
BnwWorldAnchorGraphicsBinding bind_bnw_world_anchor_graphics(
    const LiveSessionState &state,
    std::shared_ptr<BnwWorldAnchorGraphicsOwner> owner);

} // namespace civ5::runtime
