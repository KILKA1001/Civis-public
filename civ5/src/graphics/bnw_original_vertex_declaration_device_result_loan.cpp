#include "civ5/ui/detail/bnw_original_vertex_declaration_device_result_loan.hpp"
#include "civ5/ui/detail/bnw_original_nonfont_device_initializer.hpp"
#include <thread>
#include <stdexcept>
#include <utility>
namespace civ5::ui::detail {
struct BnwOriginalVertexDeclarationDeviceResultLoan::Impl final {
 using Channel=graphics::detail::BnwOwnedGlRetirementNode::VertexDeclarationChannel;
 enum class Outcome {actual_vao_created};
 const std::weak_ptr<const BnwOriginalNonFontDeviceInitializer> initializer;
 const std::weak_ptr<const BnwOriginalVertexDeclarationCreateLoan> creating_operand;
 const std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
 const Channel::Owner node;
 const std::uint64_t epoch;
 const std::uint32_t slot;
 const std::vector<BnwOriginalVertexDeclarationElement> schema;
 const std::array<GLenum,2> actual_post; // Actual creation reset/error observation, not currentness.
 const graphics::GlApi physical_dispatch; // Loaded actual creating device snapshot, no proc reload.
 const Outcome outcome=Outcome::actual_vao_created;
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan> canonical;
 Impl(const std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>& i,
  const std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan>& c,
  std::shared_ptr<const graphics::detail::BnwRendererContextPin> p,Channel::Owner n,
  std::uint64_t e,std::uint32_t s,std::vector<BnwOriginalVertexDeclarationElement> v,std::array<GLenum,2> j,graphics::GlApi a)
  :initializer(i),creating_operand(c),pin(std::move(p)),node(std::move(n)),epoch(e),slot(s),schema(std::move(v)),actual_post(j),physical_dispatch(a){}
 std::shared_ptr<const BnwOriginalNonFontDeviceInitializer> first()const{
  if(creator!=std::this_thread::get_id()||!pin||!node||!epoch||slot>=15||schema.size()<2||
     actual_post[0]!=GL_NO_ERROR||actual_post[1]!=GL_NO_ERROR||
     !physical_dispatch.GetError||!physical_dispatch.GetGraphicsResetStatus)
   throw std::runtime_error("Declaration actual device result retired or failed");
  const auto init=initializer.lock();if(!init)throw std::runtime_error("Declaration original Init retired");
  init->require_physical_without_callbacks();const auto same=init->retain_owner_owned_without_callbacks();
  if(same.get()!=init.get()||same.owner_before(init)||init.owner_before(same)||init->initialization_epoch_without_callbacks()!=epoch)
   throw std::runtime_error("Declaration original Init epoch/control differs");
  Channel::ReadAccess::require_same_pin_without_callbacks(*node,pin);
  (void)Channel::ReadAccess::name_without_callbacks(*node);return init;
 }
 void same(const BnwOriginalNonFontDeviceInitializer& i,std::uint64_t e,std::uint32_t s,
  std::span<const BnwOriginalVertexDeclarationElement> v)const{
  const auto init=first();
  if(init.get()!=&i||e!=epoch||s!=slot||v.size()!=schema.size())
   throw std::runtime_error("Declaration result Init/epoch/slot/schema differs");
  for(std::size_t n=0;n<v.size();++n){const auto&a=v[n];const auto&b=schema[n];
   if(a.stream!=b.stream||a.offset!=b.offset||a.type!=b.type||a.method!=b.method||a.usage!=b.usage||a.usage_index!=b.usage_index)
    throw std::runtime_error("Declaration immutable accepted element differs");}
  (void)first();
 }
};
BnwOriginalVertexDeclarationDeviceResultLoan::BnwOriginalVertexDeclarationDeviceResultLoan(std::shared_ptr<const Impl> p)
 :impl_(std::move(p)){}
BnwOriginalVertexDeclarationDeviceResultLoan::~BnwOriginalVertexDeclarationDeviceResultLoan()=default;
void BnwOriginalVertexDeclarationDeviceResultLoan::require_physical_without_callbacks()const{
 if(!impl_)throw std::runtime_error("Declaration result control absent");
 (void)impl_->first();const auto original=impl_->canonical.lock();
 if(!original||original.get()!=this)throw std::runtime_error("Declaration canonical result retired");
 (void)impl_->first();
}
std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
BnwOriginalVertexDeclarationDeviceResultLoan::retain_owner_owned_without_callbacks()const{
 require_physical_without_callbacks();const auto original=impl_->canonical.lock();require_physical_without_callbacks();return original;
}
std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
BnwOriginalVertexDeclarationDeviceResultLoan::seal_created_owned(
 std::shared_ptr<const BnwOriginalNonFontDeviceInitializer> init,
 std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan> create,
 std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin,
 graphics::detail::BnwOwnedGlRetirementNode::VertexDeclarationChannel::Owner node,
 std::uint64_t epoch,std::uint32_t slot,std::vector<BnwOriginalVertexDeclarationElement> schema,std::array<GLenum,2> actual_post,graphics::GlApi actual_physical_dispatch){
 if(!init||!create||!pin||!node)throw std::runtime_error("Declaration seal lacks authentic owners");
 const auto i=init->retain_owner_owned_without_callbacks();const auto c=create->retain_owner_owned_without_callbacks();
 if(i.get()!=init.get()||i.owner_before(init)||init.owner_before(i)||c.get()!=create.get()||c.owner_before(create)||create.owner_before(c))
  throw std::runtime_error("Declaration seal offered canonical owners differ");
 create->require_physical_without_callbacks();
 const auto body=std::make_shared<Impl>(init,create,pin,std::move(node),epoch,slot,std::move(schema),actual_post,actual_physical_dispatch);
 create->require_physical_without_callbacks();(void)body->first();
 std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan> result(new BnwOriginalVertexDeclarationDeviceResultLoan(body));
 create->require_physical_without_callbacks();(void)body->first();body->canonical=result;
 result->require_physical_without_callbacks();return result;
}
std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>
BnwOriginalVertexDeclarationDeviceResultLoan::initializer_owner_owned_without_callbacks()const{
 const auto self=retain_owner_owned_without_callbacks();const auto init=self->impl_->first();
 self->require_physical_without_callbacks();return init;
}
std::shared_ptr<const graphics::detail::BnwRendererContextPin>
BnwOriginalVertexDeclarationDeviceResultLoan::context_pin_owned_without_callbacks()const{
 const auto self=retain_owner_owned_without_callbacks();const auto pin=self->impl_->pin;
 self->require_physical_without_callbacks();return pin;
}
graphics::GlApi BnwOriginalVertexDeclarationDeviceResultLoan::physical_dispatch_without_callbacks()const{
 const auto self=retain_owner_owned_without_callbacks();const auto api=self->impl_->physical_dispatch;
 self->require_physical_without_callbacks();return api;
}
void BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_creation_without_callbacks(
 const BnwOriginalVertexDeclarationDeviceResultLoan& offered,const BnwOriginalNonFontDeviceInitializer& init,
 std::uint64_t epoch,const BnwOriginalVertexDeclarationCreateLoan& actual,std::uint32_t slot,
 std::span<const BnwOriginalVertexDeclarationElement> schema){
 const auto result=offered.retain_owner_owned_without_callbacks();
 const auto create=actual.retain_owner_owned_without_callbacks();
 const auto origin=result->impl_->creating_operand.lock();
 if(!origin||origin.get()!=create.get()||origin.owner_before(create)||create.owner_before(origin))
  throw std::runtime_error("Declaration result genuine creating operand differs");
 result->impl_->same(init,epoch,slot,schema);actual.require_physical_without_callbacks();result->require_physical_without_callbacks();
}
void BnwOriginalVertexDeclarationDeviceResultLoan::RootAccess::require_current_without_callbacks(
 const BnwOriginalVertexDeclarationDeviceResultLoan& offered,const BnwOriginalNonFontDeviceInitializer& init,
 std::uint64_t epoch,std::uint32_t slot,std::span<const BnwOriginalVertexDeclarationElement> schema){
 const auto result=offered.retain_owner_owned_without_callbacks();result->impl_->same(init,epoch,slot,schema);
 // No old lexical CreateLoan/current slot is read in retained completed use.
 result->require_physical_without_callbacks();
}
}
