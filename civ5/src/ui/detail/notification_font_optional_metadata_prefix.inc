// Live Source4 nonNULL branch, AFTER genuine base observation/current POST.
const auto original_source=request->source_owned(operation,*resource);
const auto optional_filename=detail::bnw_native_font_optional_filename(original_source);
pure();observation->require_current_owned(operation,*resource,*request);pure();
// Graphics owns this getter and its cache/renderer FIRST + actualBank POST.
// Exact source-view Source2+mandatorySource3 correction provide this API;
// real query/outcomes still dependency-HOLD until their body is qualified.
const auto source_payload=borrower->captured_source_owned(operation,*resource,*request);
const auto bank_payload=resource->original_texture_source_owned(operation,*request);
borrower->require_current_owned(operation,*resource,*request);pure();
require(source_payload.bytes&&bank_payload.bytes&&source_payload.bytes.get()==bank_payload.bytes.get()&&
 !source_payload.bytes.owner_before(bank_payload.bytes)&&!bank_payload.bytes.owner_before(source_payload.bytes)&&
 source_payload.payload_identity.sha256==bank_payload.payload_identity.sha256&&
 source_payload.payload_identity.bytes==bank_payload.payload_identity.bytes,
 "Font metadata quote is not actualBASE sameBank winning byte storage");
pure();observation->require_current_owned(operation,*resource,*request);pure();
const auto bc_marker=detail::bnw_native_font_bc_marker(*source_payload.bytes);
pure();resource->require_current_owned(operation);pure();
borrower->require_current_owned(operation,*resource,*request);pure();
observation->require_current_owned(operation,*resource,*request);pure();
actual_font.optional_metadata.emplace_back(optional_filename,bc_marker,source_payload.payload_identity);
// This is reached DATA only; family issuer and two-query custody remain absent.
// Zero/nonzero is not permission to bypass an unissued current row descendant.
throw std::runtime_error(bc_marker!=0?
 "Font exact BC marker/index filename reached; genuine base/index pending-row family issuer missing":
 "Font genuine BC zero reached; positive-base row cache/postcleanup custody admission missing");
