// Private owned XML cursor/Name/Instance effects. No visible control is
// synthesized for Instance: actual A0 receives its first child XML cursor.
struct BnwWorldViewControlTree::ContextRootNodeReturn final {
 enum class Kind {styles,instance,distinct_context,ordinary_control};
 const std::weak_ptr<ContextRootChildrenEvidence> actual_cursor;
 const std::shared_ptr<ContextRootXml84> document;
 const content::XmlNode*const declaration;
 const std::size_t ordinal;
 const Kind kind;
 const std::shared_ptr<const ContextRootStyleCursorValue> style;
 const std::optional<std::uint32_t> instance_key;
 const std::shared_ptr<const content::XmlNode> instance_selected;
 const std::weak_ptr<const detail::NotificationContextRootChildAttachReturn> attached;
 const std::weak_ptr<Core> child;
 const std::weak_ptr<const detail::NotificationLayoutNativeChildControlsReturn> ordinary_controls;
 ContextRootNodeReturn(std::weak_ptr<ContextRootChildrenEvidence> e,std::shared_ptr<ContextRootXml84> xml,
  const content::XmlNode* node,std::size_t i,Kind k,std::shared_ptr<const ContextRootStyleCursorValue> s={},
  std::optional<std::uint32_t> key={},std::shared_ptr<const content::XmlNode> selected={},
  std::weak_ptr<const detail::NotificationContextRootChildAttachReturn> a={},std::weak_ptr<Core> c={},
  std::weak_ptr<const detail::NotificationLayoutNativeChildControlsReturn> v64={})
  :actual_cursor(std::move(e)),document(std::move(xml)),declaration(node),ordinal(i),kind(k),style(std::move(s)),
   instance_key(key),instance_selected(std::move(selected)),attached(std::move(a)),child(std::move(c)),
   ordinary_controls(std::move(v64)){}
};
struct BnwWorldViewControlTree::ContextRootInstanceMutation final {
 enum class Stage {before_lookup,null_inserted,cursor_stored};
 const std::weak_ptr<ContextRootChildrenEvidence> actual_cursor;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorPrefix> constructor;
 context_constructor_prefix_detail::EmptyInstanceMapA0*const map;
 const std::shared_ptr<ContextRootXml84> document;
 const content::XmlNode*const declaration;
 const std::size_t ordinal,before_count;
 const std::uint64_t before_revision;
 const std::uint32_t key;
 const std::shared_ptr<const content::XmlNode> prior,selected;
 const bool key_absent;
 Stage stage=Stage::before_lookup;
 ContextRootInstanceMutation(std::weak_ptr<ContextRootChildrenEvidence> e,
  std::shared_ptr<const detail::NotificationContextNativeConstructorPrefix> ctor,
  context_constructor_prefix_detail::EmptyInstanceMapA0*m,std::shared_ptr<ContextRootXml84> xml,
  const content::XmlNode*node,std::size_t i,std::size_t count,std::uint64_t revision,std::uint32_t hash,
  std::shared_ptr<const content::XmlNode> old,std::shared_ptr<const content::XmlNode> actual,bool absent)
  :actual_cursor(std::move(e)),constructor(std::move(ctor)),map(m),document(std::move(xml)),declaration(node),ordinal(i),
   before_count(count),before_revision(revision),key(hash),prior(std::move(old)),selected(std::move(actual)),key_absent(absent){}
 void first()const;
};
namespace context_root_read_temporary_detail {
enum class CursorPhase {document,selected_child,exhausted};
struct XmlCursor final {
 enum class Type {cursor_835540,retired_base_b76284};
 Type type=Type::cursor_835540;
 const content::XmlNode* selected=nullptr;
 explicit XmlCursor(const content::XmlNode* node=nullptr):selected(node){}
 void retire_8353f0_without_callbacks(){
  require(type==Type::cursor_835540,"actual RootRead cursor DTOR replay");type=Type::retired_base_b76284;
  //8353F0 writes subtype only; it does not clear selected4.
 }
};
struct NameString final {
 enum class Life {query_value,retired_canonical_empty};
 Life life=Life::query_value;
 std::string value; // qualified own UTF8 FString storage; no x86 pool addresses
 void retire_to_empty_owned(){
  require(life==Life::query_value,"actual RootRead String18 retirement replay");
  {std::string retired;value.swap(retired);} // actual owned value allocation DTOR before return
  life=Life::retired_canonical_empty;
 }
};
}
