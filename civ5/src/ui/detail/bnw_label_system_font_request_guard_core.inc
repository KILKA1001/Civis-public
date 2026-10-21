// Inside civ5::ui after complete Request::Impl and genuine Request bodies.
// Guard pins independent metadata only. Original Request DTOR flips alive,
// which every guard FIRST checks; retention never renews lexical authority.
detail::BnwLabelSystemFontRequest::Guard::Guard(std::shared_ptr<const Impl> saved):impl_(std::move(saved)){}
detail::BnwLabelSystemFontRequest::Guard
detail::BnwLabelSystemFontRequest::capture_guard_without_callbacks()const{
 const auto saved=impl_;saved->first();return Guard(saved);
}
void detail::BnwLabelSystemFontRequest::Guard::first_without_callbacks()const{
 const auto saved=impl_;saved->first();
}
void detail::BnwLabelSystemFontRequest::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;saved->first();operation.require_operation(saved->serial);saved->first();
 saved->source->require_current_owned();saved->first();operation.require_operation(saved->serial);saved->first();
}
std::shared_ptr<const detail::BnwLabelSystemFontSource>
detail::BnwLabelSystemFontRequest::Guard::source_without_callbacks()const{
 const auto saved=impl_;saved->first();auto result=saved->source;saved->first();return result;
}
std::shared_ptr<BnwSystemFontCacheOwner>
detail::BnwLabelSystemFontRequest::Guard::cache_without_callbacks()const{
 const auto saved=impl_;saved->first();auto result=saved->cache.lock();saved->first();return result;
}
std::int32_t detail::BnwLabelSystemFontRequest::Guard::height_without_callbacks()const{
 const auto saved=impl_;saved->first();return saved->height;
}
std::uint64_t detail::BnwLabelSystemFontRequest::Guard::serial_without_callbacks()const{
 const auto saved=impl_;saved->first();return saved->serial;
}
