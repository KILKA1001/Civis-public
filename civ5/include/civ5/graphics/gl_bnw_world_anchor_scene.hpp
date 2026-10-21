#pragma once

#include "civ5/graphics/gl_context.hpp"
#include "civ5/runtime/bnw_world_anchor_graphics_adapter.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>

namespace civ5::graphics {

// A world renderer can implement this only after it owns the actual drawable
// anchor objects and their GL resources. A saved plot alone is not a drawable.
class GlBnwWorldAnchorDrawBackend {
  public:
    virtual ~GlBnwWorldAnchorDrawBackend() = default;
    virtual const GlContext *context() const noexcept = 0;
    virtual runtime::BnwWorldAnchorGraphicsIdentity world_identity() const = 0;
    virtual bool has_drawable(std::uint64_t plot, std::int32_t anchor,
                              std::int32_t x, std::int32_t y) const = 0;
    virtual void remove_drawable(std::uint64_t plot, std::int32_t anchor,
                                 std::int32_t x, std::int32_t y,
                                 std::int32_t event_data) = 0;
};

// Retains the BNW world identity and source anchor roster through game-turn
// publication. It becomes a GameplayWorldAnchor graphics owner only when a
// matching live GL backend owns *every* source anchor drawable. The current
// LAND/water/FOW renderers do not implement that backend.
class GlBnwWorldAnchorScene final : public runtime::BnwWorldAnchorGraphicsOwner {
  public:
    GlBnwWorldAnchorScene(const runtime::LiveSessionState &state,
                          std::shared_ptr<GlContext> context,
                          std::shared_ptr<GlBnwWorldAnchorDrawBackend> backend = {});
    bool supports_gameplay_world_anchor() const override;
    runtime::BnwWorldAnchorGraphicsIdentity current_world() const override;
    bool publication_context_current() const override;
    void gameplay_world_anchor(std::int32_t anchor, bool add,
                               std::int32_t x, std::int32_t y,
                               std::int32_t data) override;
    std::size_t retained_anchor_count() const noexcept { return anchors_.size(); }
    bool publication_uncertain() const noexcept { return uncertain_; }

  private:
    struct Anchor { std::int32_t type, x, y; };
    runtime::BnwWorldAnchorGraphicsIdentity identity_;
    std::shared_ptr<GlContext> context_;
    std::shared_ptr<GlBnwWorldAnchorDrawBackend> backend_;
    std::thread::id render_thread_;
    std::map<std::uint64_t, Anchor> anchors_;
    bool uncertain_ = false;
};

} // namespace civ5::graphics
