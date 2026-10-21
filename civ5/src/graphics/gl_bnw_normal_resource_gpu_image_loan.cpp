#include "civ5/graphics/detail/bnw_normal_resource_gpu_image_loan.hpp"
#include "civ5/ui/detail/bnw_original_nonfont_device_initializer.hpp"
#include "civ5/ui/detail/bnw_original_generated_resource_image_loan.hpp"
#include <thread>
#include <stdexcept>
#include <utility>
namespace civ5::graphics::detail {
struct BnwNormalResourceGpuImageLoan::Impl final {
 using Channel=BnwOwnedGlRetirementNode::ResourceImageChannel;
 const std::shared_ptr<const ui::detail::BnwOriginalNonFontDeviceInitializer> init;
 const std::shared_ptr<const ui::detail::BnwOriginalGeneratedResourceImageLoan> cpu;
 const std::shared_ptr<const BnwRendererContextPin> pin;
 const Channel::Owner texture;
 const std::uint64_t epoch;
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNormalResourceGpuImageLoan> canonical;
 Impl(std::shared_ptr<const ui::detail::BnwOriginalNonFontDeviceInitializer> i,
      std::shared_ptr<const ui::detail::BnwOriginalGeneratedResourceImageLoan> c,
      std::shared_ptr<const BnwRendererContextPin> p,Channel::Owner t,std::uint64_t e)
 :init(std::move(i)),cpu(std::move(c)),pin(std::move(p)),texture(std::move(t)),epoch(e){}
 void first()const{
  if(creator!=std::this_thread::get_id()||!init||!cpu||!pin||!texture||!epoch)
   throw std::runtime_error("Root GPU image physical control retired");
  init->require_physical_without_callbacks();cpu->require_physical_without_callbacks();
  const auto i=init->retain_owner_owned_without_callbacks();
  const auto c=cpu->retain_owner_owned_without_callbacks();
  if(i.get()!=init.get()||i.owner_before(init)||init.owner_before(i)||
     c.get()!=cpu.get()||c.owner_before(cpu)||cpu.owner_before(c)||
     init->initialization_epoch_without_callbacks()!=epoch)
   throw std::runtime_error("Root GPU image original Init/CPU/epoch differs");
  Channel::ReadAccess::require_same_pin_without_callbacks(*texture,pin);
  (void)Channel::ReadAccess::image_name_without_callbacks(*texture);
  init->require_physical_without_callbacks();cpu->require_physical_without_callbacks();
 }
 void source(const runtime::BnwWorldViewSourceOperationLoan& op)const{
  first();init->require_gpu_resource0_image_current_owned(op,cpu,epoch);first();
 }
};
BnwNormalResourceGpuImageLoan::BnwNormalResourceGpuImageLoan(std::shared_ptr<const Impl> body)
 :impl_(std::move(body)){}
BnwNormalResourceGpuImageLoan::~BnwNormalResourceGpuImageLoan()=default;
std::shared_ptr<const BnwNormalResourceGpuImageLoan>
BnwNormalResourceGpuImageLoan::seal_uploaded_owned(
 std::shared_ptr<const ui::detail::BnwOriginalNonFontDeviceInitializer> init,
 std::shared_ptr<const ui::detail::BnwOriginalGeneratedResourceImageLoan> cpu,
 std::shared_ptr<const BnwRendererContextPin> pin,
 BnwOwnedGlRetirementNode::ResourceImageChannel::Owner texture,std::uint64_t epoch,
 const runtime::BnwWorldViewSourceOperationLoan& op){
 if(!init||!cpu||!pin||!texture)throw std::runtime_error("GPU upload seal lacks owned controls");
 const auto original_init=init->retain_owner_owned_without_callbacks();
 const auto original_cpu=cpu->retain_owner_owned_without_callbacks();
 if(original_init.get()!=init.get()||original_init.owner_before(init)||init.owner_before(original_init)||
    original_cpu.get()!=cpu.get()||original_cpu.owner_before(cpu)||cpu.owner_before(original_cpu))
  throw std::runtime_error("GPU upload seal canonical controls differ");
 const auto full=[&]{init->require_gpu_resource0_creation_owned(op,cpu);
  if(init->initialization_epoch_without_callbacks()!=epoch)
   throw std::runtime_error("GPU upload seal epoch changed");};
 full();
 auto body=std::make_shared<Impl>(init,cpu,pin,std::move(texture),epoch);
 full();body->first();
 std::shared_ptr<const BnwNormalResourceGpuImageLoan> facade(new BnwNormalResourceGpuImageLoan(body));
 full();body->first();body->canonical=facade;
 facade->require_source_owned(op);return facade;
}
void BnwNormalResourceGpuImageLoan::require_physical_without_callbacks()const{
 if(!impl_)throw std::runtime_error("Root GPU image control absent");
 impl_->first();const auto owner=impl_->canonical.lock();
 if(!owner||owner.get()!=this)throw std::runtime_error("Root GPU image canonical facade retired");
 impl_->first();
}
std::shared_ptr<const BnwNormalResourceGpuImageLoan>
BnwNormalResourceGpuImageLoan::retain_owner_owned_without_callbacks()const{
 require_physical_without_callbacks();const auto owner=impl_->canonical.lock();
 require_physical_without_callbacks();return owner;
}
void BnwNormalResourceGpuImageLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
 const auto owner=retain_owner_owned_without_callbacks();owner->impl_->source(op);
 owner->require_physical_without_callbacks();
}
void BnwNormalResourceGpuImageLoan::require_same_context_without_callbacks(const BnwRendererContextPin& offered)const{
 require_physical_without_callbacks();if(impl_->pin.get()!=&offered)
  throw std::runtime_error("Root GPU image Pin differs");
 Impl::Channel::ReadAccess::require_same_pin_without_callbacks(*impl_->texture,impl_->pin);
 require_physical_without_callbacks();
}
void BnwNormalResourceGpuImageLoan::require_same_inputs_without_callbacks(
 const std::shared_ptr<const ui::detail::BnwOriginalNonFontDeviceInitializer>& i,
 const std::shared_ptr<const ui::detail::BnwOriginalGeneratedResourceImageLoan>& c,
 const std::shared_ptr<const BnwRendererContextPin>& p,std::uint64_t e)const{
 require_physical_without_callbacks();
 if(!i||!c||!p||i.get()!=impl_->init.get()||i.owner_before(impl_->init)||impl_->init.owner_before(i)||
    c.get()!=impl_->cpu.get()||c.owner_before(impl_->cpu)||impl_->cpu.owner_before(c)||
    p.get()!=impl_->pin.get()||p.owner_before(impl_->pin)||impl_->pin.owner_before(p)||e!=impl_->epoch)
  throw std::runtime_error("Root GPU image Init/CPU/Pin association differs");
 require_physical_without_callbacks();
}
std::shared_ptr<const ui::detail::BnwOriginalGeneratedResourceImageLoan>
BnwNormalResourceGpuImageLoan::original_cpu_image_owned_without_callbacks()const{
 require_physical_without_callbacks();return impl_->cpu;
}
GLuint BnwNormalResourceGpuImageLoan::texture_name_without_callbacks()const{
 require_physical_without_callbacks();return Impl::Channel::ReadAccess::image_name_without_callbacks(*impl_->texture);
}
}
