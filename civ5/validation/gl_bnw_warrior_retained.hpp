#pragma once
#include "bnw_warrior_world_binding.hpp"
#include "bnw_warrior_skin_diagnostic.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/terrain/camera_frame.hpp"
#include "gl_bnw_warrior_material_bridge.hpp"
#include <memory>
#include <thread>

namespace civ5::graphics {
// A native capture pin is an integration-owned trust input, not an inferred
// formation generator. Both files must come from an independently accepted
// capture of the exact original save; an empty pin never enables world drawing.
struct BnwWarriorPlacementPin {
    std::filesystem::path record,native_snapshot;
    std::string record_sha256,native_snapshot_sha256;
};
class BnwWarriorPlacementReceipt {
  public:
    const std::array<terrain::Matrix4,12> &member_world() const noexcept {return member_world_;}
    std::int64_t facing_direction() const noexcept {return facing_;}
    const std::string &save_sha256() const noexcept {return save_;}
    const std::string &record_sha256() const noexcept {return record_;}
  private:
    friend BnwWarriorPlacementReceipt load_bnw_warrior_placement_receipt(const BnwWarriorPlacementPin &);
    BnwWarriorPlacementReceipt()=default;
    std::array<terrain::Matrix4,12> member_world_{};
    std::int64_t facing_=-1;
    std::string save_,record_;
};
BnwWarriorPlacementReceipt load_bnw_warrior_placement_receipt(const BnwWarriorPlacementPin &);

struct GlBnwWarriorFrame {
    // The source camera uses row vectors. GL_FALSE upload interprets these
    // arrays as transposed column-vector matrices; source depth is 0..1.
    terrain::Matrix4 view{},projection{};
    std::int32_t width=0,height=0;
    GLenum depth_function=GL_LEQUAL;
};
struct GlBnwWarriorDrawResult {
    bool drawn=false,model_drawable=false,full_graphics_parity=false;
    std::size_t members=0,triangles=0;
    int sdk_sample_index=-1;
    std::string blocker;
};

// Retains the three original meshes, their textures and the exact SDK poses.
// Creation, pose changes, draw and destruction belong to the context owner.
// This object has no mutable live-world or RNG reference.
class GlBnwWarriorRetainedBackend {
  public:
    GlBnwWarriorRetainedBackend(std::shared_ptr<GlContext> context,
        const std::filesystem::path &assets,const std::filesystem::path &sdk_cache);
    ~GlBnwWarriorRetainedBackend();
    GlBnwWarriorRetainedBackend(const GlBnwWarriorRetainedBackend &)=delete;
    GlBnwWarriorRetainedBackend &operator=(const GlBnwWarriorRetainedBackend &)=delete;
    void select_sdk_sample(int index); // -1 bind; 0/1/2/3 SDK times 0/1/3/5.
    GlBnwWarriorDrawResult draw_selected(const BnwRendererSceneReceipt &scene,
        const runtime::LiveSessionState &live,
        const BnwWarriorPlacementReceipt *placement,const GlBnwWarriorFrame &frame);
    // Synthetic placement is isolated to this explicit diagnostic entry point.
    // It always returns model_drawable=false, even after a successful draw.
    GlBnwWarriorDrawResult draw_diagnostic(
        const std::array<terrain::Matrix4,12> &world,const GlBnwWarriorFrame &frame);
    // Retain ONCE against the same host's roster/rules and stable scene epoch.
    void retain_code_material(const std::filesystem::path &material_metadata,
        const std::filesystem::path &sampler_metadata,const runtime::PregameRoster &same_roster,
        const content::RulesDatabase &same_rules,std::int32_t player,std::int32_t unit,
        std::uint64_t scene_revision);
    // Explicit source-format4/three-input VS diagnostic. Reuses SAME code
    // resources/packets; SDK poses remain diagnostic, production flags false.
    // Validate all streams before publication. Existing raw4/ordinary buffers
    // are preserved; draw_code_diagnostic selects this opt-in route afterward.
    void retain_native_format4_diagnostic();
    bool native_format4_diagnostic_retained()const;
    void update_code_visibility(std::span<const std::uint8_t,12> states,float renderer_clock,
        std::int32_t unit,std::uint64_t scene_revision);
    GlBnwWarriorDrawResult draw_code_diagnostic(
        const std::array<terrain::Matrix4,12> &world,const GlBnwWarriorFrame &frame,
        const GlBnwWarriorMaterialView &material_view);
    GlBnwWarriorMaterialState code_material_state()const;
    std::size_t retained_meshes() const noexcept {return 3;}
    std::size_t buffer_uploads() const noexcept;
    // Model bytes/SDK poses remain unscaled. Generated world matrices apply
    // this source-validated native scalar once, before backend drawing.
    float native_model_scale(float compiled_scale) const;
    const std::string &metadata_sha256() const noexcept;
  private:
    struct Impl; std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
