// Include once inside civ5::ui::detail, after the actual Model source header
// and this child's complete header. Defines a real caller, not a tuple adapter.
std::shared_ptr<const CivisSavedWorldLoadingOwner>
CivisFrontendPartyWorldSource::capture_loading_inputs_owned()const {
    return CivisSavedWorldLoadingOwner::capture_owned(*this);
}
