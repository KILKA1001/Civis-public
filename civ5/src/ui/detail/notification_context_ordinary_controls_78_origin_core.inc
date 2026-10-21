// A child origin is earned by the ACTUAL completed parent common factory and
// its ordered78 member. It is not another root factory or a cold XML locator.
struct BnwWorldViewControlTree::RecursiveOrdinaryOrigin final {
 using Factory=detail::NotificationContextRootOrdinaryFactoryReturn;
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<RootOrdinaryControlsFrame> parent;
 const std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> parent_common,common;
 const std::shared_ptr<const Core::FullXmlOccurrence> parent_xml,xml;
 const std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const void> typed_state;
 const std::shared_ptr<const detail::NotificationLayoutContextRealmRef> inherited_3c;
 const detail::NotificationLayoutBaseline::Base::Context3CObservation inherited_observation;
 const Factory::Subtype subtype;
 const std::uint64_t id;
 const std::size_t ordinal;
 RecursiveOrdinaryOrigin(std::shared_ptr<RootOrdinaryControlsFrame> p,
  std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> pc,
  std::shared_ptr<const detail::NotificationLayoutCommonChildFactoryReturn> c,
  std::shared_ptr<const Core::FullXmlOccurrence> px,std::shared_ptr<const Core::FullXmlOccurrence> x,
  std::shared_ptr<const Core::LayoutReceiverOccurrence> o,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> r,
  std::shared_ptr<const void> concrete,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> realm,
  detail::NotificationLayoutBaseline::Base::Context3CObservation observation,
  Factory::Subtype type,std::uint64_t control,std::size_t index)
  :parent(std::move(p)),parent_common(std::move(pc)),common(std::move(c)),parent_xml(std::move(px)),
   xml(std::move(x)),occurrence(std::move(o)),receiver(std::move(r)),typed_state(std::move(concrete)),inherited_3c(std::move(realm)),
   inherited_observation(observation),subtype(type),id(control),ordinal(index){}
 void first()const {BnwWorldViewControlTree::require_recursive_root_ordinary_origin_without_callbacks(*this);}
};
