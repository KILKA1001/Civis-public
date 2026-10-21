#pragma once
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/ui/bnw_audio_window_activation_loan.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
namespace civ5::ui {
struct BnwNewEraNativeXmlFile {
 BnwAudioXmlKind kind;
 std::string basename,sha256;
 std::size_t declaration_ordinal=0;
 std::uint64_t bytes=0;
};
struct BnwNewEraNativeXmlRow {
 content::XmlNode row;
 std::vector<content::XmlNode> authored_defaults;
 BnwNewEraNativeXmlFile file;
 std::size_t row_ordinal=0;
};
struct BnwNewEraNativeXmlOperands {
 BnwNewEraNativeXmlRow script,definition;
 std::vector<BnwNewEraNativeXmlFile> files;
 std::uint64_t actual_activation_sequence=0;
};
// Captures authored NewEra operands via a genuine completed selected-read leaf.
// Dependencies are read in declaration order, before local rows. Duplicate IDs
// refuse; defaults and empty Booleans are preserved without invented values.
// This owner does not issue PopupShown, consume RNG, initialize knobs or submit
// a voice. Actual category/gain/duplicate/device effects remain separate.
class BnwNewEraNativeXmlOwner final {
 public:
 static std::shared_ptr<const BnwNewEraNativeXmlOwner> from_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,
  const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwNewEraNativeXmlOwner();
 BnwNewEraNativeXmlOwner(const BnwNewEraNativeXmlOwner&)=delete;
 BnwNewEraNativeXmlOwner& operator=(const BnwNewEraNativeXmlOwner&)=delete;
 void require_current()const;
 BnwNewEraNativeXmlOperands operands()const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNewEraNativeXmlOwner(std::shared_ptr<Impl>);
};
}
#endif
