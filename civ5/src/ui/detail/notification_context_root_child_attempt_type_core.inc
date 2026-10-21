struct BnwWorldViewControlTree::ContextRootChildAttempt final {
 using State=detail::NotificationLayoutContextOrdinaryState;
 const detail::NotificationContextRootReadChildrenLoan::Guard parent;
 const std::shared_ptr<ContextRootXml84> document;
 const content::XmlNode*const declaration;
 const std::shared_ptr<const detail::NotificationLayoutContextRealmRef> realm;
 const std::shared_ptr<const runtime::ScriptUiChildSource> descriptor;
 const runtime::detail::ContextLoaderResourceOperand operand;
 const std::string selected_context;
 std::shared_ptr<BnwWorldViewControlTree> child;
 std::shared_ptr<Core> child_core;
 std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
 std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> constructor;
 struct WrapperReturn final {
  const std::shared_ptr<State> state;
  const std::shared_ptr<const detail::NotificationLayoutContextRealmRef> realm;
  const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
  const std::uint64_t sequence,effects,b4_revision;
  WrapperReturn(std::shared_ptr<State> s,std::shared_ptr<const detail::NotificationLayoutContextRealmRef> r,
   std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> a,std::uint64_t q,std::uint64_t e,std::uint64_t b)
   :state(std::move(s)),realm(std::move(r)),children(std::move(a)),sequence(q),effects(e),b4_revision(b){}
 };
 std::shared_ptr<const WrapperReturn> pending_wrapper,wrapper;
 std::shared_ptr<const detail::NotificationContextRootChildFactoryReturn> factory;
 std::shared_ptr<const detail::NotificationContextChildControlsExportLoan> controls_export_operand;
 std::shared_ptr<const detail::NotificationContextChildControlsExportReturn> controls_export_return;
 std::shared_ptr<const ContextChildName588Registration> name588_registration;
 std::shared_ptr<const detail::NotificationLayoutNativeChildControlsReturn> pending_controls_v64_return,controls_v64_return;
 std::shared_ptr<ContextDistinctChildAttachProgress> attach_progress;
 ContextRootChildAttempt(detail::NotificationContextRootReadChildrenLoan::Guard p,
  std::shared_ptr<ContextRootXml84> xml,const content::XmlNode*decl,
  std::shared_ptr<const detail::NotificationLayoutContextRealmRef> r,
  std::shared_ptr<const runtime::ScriptUiChildSource> d,runtime::detail::ContextLoaderResourceOperand o,std::string name)
  :parent(std::move(p)),document(std::move(xml)),declaration(decl),realm(std::move(r)),descriptor(std::move(d)),
   operand(std::move(o)),selected_context(std::move(name)){}
};
