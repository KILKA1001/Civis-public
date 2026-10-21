#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
namespace civ5::graphics::detail {class BnwFontTextureObservation;class BnwFontTextureBorrower;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {class NotificationContextGuiPort;class BnwFontTextureRequest;}
namespace civ5::runtime {
class BnwSelectedWorldViewContextOwner;class BnwCivisOwnedStockSourceBundle;class BnwWorldViewSourceOperationLoan;
namespace detail {
class BnwFontResourceLoan final {
 friend class ::civ5::runtime::BnwSelectedWorldViewContextOwner;
 friend class ::civ5::ui::detail::NotificationContextGuiPort;
 friend class ::civ5::ui::detail::BnwFontTextureRequest;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::detail::BnwFontTextureObservation;
 friend class ::civ5::graphics::detail::BnwFontTextureBorrower;
 struct Life final {
  const std::thread::id creator=std::this_thread::get_id();
  std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};std::atomic<std::uint64_t> epoch{1};
  void first(std::uint64_t expected)const{
   if(creator!=std::this_thread::get_id()||!alive.load()||failed.load()||epoch.load()!=expected){failed.store(true);throw std::runtime_error("Font resource scope retired/foreign/changed");}
  }
  void retire()noexcept{alive.store(false);epoch.fetch_add(1);}
 };
 const std::shared_ptr<Life> life_=std::make_shared<Life>();
 std::shared_ptr<const assets::CivisResourceBankOwner> bank_owner_owned(const BnwWorldViewSourceOperationLoan&)const;
 const assets::CivisResourceBankCapability& bank_capability_owned(const BnwWorldViewSourceOperationLoan&)const;
 assets::CivisLoadedResource original_texture_source_owned(const BnwWorldViewSourceOperationLoan&,
   const ::civ5::ui::detail::BnwFontTextureRequest&)const;

 const std::thread::id creator_=std::this_thread::get_id();
 const std::shared_ptr<const BnwCivisOwnedStockSourceBundle> bundle_;
 const std::weak_ptr<BnwSelectedWorldViewContextOwner> source_;
 const std::uint64_t serial_,generation_,scope_;
 const std::string request_;
 const assets::CivisLoadedResource loaded_;
 std::shared_ptr<const content::XmlNode> parser_document_;
 BnwFontResourceLoan(std::shared_ptr<const BnwCivisOwnedStockSourceBundle>,
  std::weak_ptr<BnwSelectedWorldViewContextOwner>,std::uint64_t,std::uint64_t,std::uint64_t,std::string,assets::CivisLoadedResource);
 void require_current_owned(const BnwWorldViewSourceOperationLoan&)const;
 const content::XmlNode& xml_owned(const BnwWorldViewSourceOperationLoan&)const;
 public:
 BnwFontResourceLoan(const BnwFontResourceLoan&)=delete;
 BnwFontResourceLoan&operator=(const BnwFontResourceLoan&)=delete;
 ~BnwFontResourceLoan(){life_->retire();parser_document_.reset();}
};
}}
