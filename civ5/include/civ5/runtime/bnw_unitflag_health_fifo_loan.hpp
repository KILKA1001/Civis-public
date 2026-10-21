#pragma once
#include <cstdint>
#include <memory>
#include <utility>
namespace civ5::graphics {class BnwUnitDamageCommandOwner;}
namespace civ5::runtime {
class BnwSelectedStockLuaBootstrap;class BnwUnitFlagHealthInstanceLoan;
class BnwWorldViewSourceOperationLoan;class BnwWorldViewNativeContextIssuer;
class BnwUnitFlagLivingHealthRead;
// One actual retained FIFO head in a later genuine GUI event scope.
// No render/frame/layout capability and no public event or completion issuer.
class BnwUnitFlagHealthFifoLoan final {
 public:
  ~BnwUnitFlagHealthFifoLoan();
  BnwUnitFlagHealthFifoLoan(const BnwUnitFlagHealthFifoLoan&)=delete;
  BnwUnitFlagHealthFifoLoan&operator=(const BnwUnitFlagHealthFifoLoan&)=delete;
 private:
  friend class graphics::BnwUnitDamageCommandOwner;
  friend class LuaScriptHost;
  friend class BnwSelectedStockLuaBootstrap;
  friend class BnwUnitFlagHealthInstanceLoan;
  friend class BnwWorldViewSourceOperationLoan;
  friend class BnwWorldViewNativeContextIssuer;
  struct State;const std::shared_ptr<State>state_;
  explicit BnwUnitFlagHealthFifoLoan(std::shared_ptr<State>);
  std::shared_ptr<State>retain_storage_without_callbacks()const;
  static void require_origin_from_storage_without_callbacks(const std::shared_ptr<State>&);
  static void require_current_from_storage(const std::shared_ptr<State>&);
  static std::shared_ptr<const BnwUnitFlagLivingHealthRead>health_source_from_storage_without_callbacks(const std::shared_ptr<State>&);
  static std::pair<std::int32_t,std::int32_t>owner_full_id_from_storage_without_callbacks(const std::shared_ptr<State>&);
  static std::uint64_t occurrence_from_storage_without_callbacks(const std::shared_ptr<State>&);
  static std::pair<std::int32_t,std::int32_t>damage_pair_from_storage_without_callbacks(const std::shared_ptr<State>&);
  void require_origin_without_callbacks()const;
  void require_current()const;
  std::shared_ptr<const BnwUnitFlagLivingHealthRead>health_source_without_callbacks()const;
  std::pair<std::int32_t,std::int32_t>owner_full_id_without_callbacks()const;
  class DeliveryReturn final {
   public:
    DeliveryReturn(DeliveryReturn&&)noexcept;
    DeliveryReturn&operator=(DeliveryReturn&&)=delete;
    DeliveryReturn(const DeliveryReturn&)=delete;
    DeliveryReturn&operator=(const DeliveryReturn&)=delete;
    ~DeliveryReturn();
   private:
    friend class graphics::BnwUnitDamageCommandOwner;
    friend class LuaScriptHost;
    friend class BnwSelectedStockLuaBootstrap;
    // RootTurn supplies the genuine protected-original-event return body.
    // Includes actual early branches and Source/GUI callback teardown when entered.
    struct OriginalReturnedEvent;
    explicit DeliveryReturn(const BnwUnitFlagHealthFifoLoan&,const OriginalReturnedEvent&);
    std::shared_ptr<State>state_;std::uint64_t occurrence_=0;
  };
};
}
