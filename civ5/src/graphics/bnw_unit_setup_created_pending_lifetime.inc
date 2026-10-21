#include <atomic>
namespace civ5::graphics {
struct BnwUnitSetupGraphicalQueue::PendingReadLifetime {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<BnwUnitSetupGraphicalQueue>queue;
 const CreatedOccurrenceLoan*const loan;
 const std::uint64_t drain_serial,append_serial;
 std::atomic<bool>alive{true};
 PendingReadLifetime(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&q,const CreatedOccurrenceLoan*l,
  std::uint64_t drain,std::uint64_t append):queue(q),loan(l),drain_serial(drain),append_serial(append){}
 void current()const{
  // Saved lifetime FIRST. Never touch a retired/reused loan address.
  setup_need(creator==std::this_thread::get_id(),"Created Unit reference foreign thread");
  setup_need(alive.load(),"Created Unit reference original FIFO scope retired");
  Pin pin(queue);const auto s=pin.owner->state_;
  setup_need(s->busy&&s->created_loan==loan&&loan&&s->created_drain_serial==drain_serial&&s->serial==append_serial,
   "Created Unit reference exact active canonical occurrence changed");
  loan->require_current();
  setup_need(alive.load(),"Created Unit reference retired during Source callback");pin.owner->pure();
  setup_need(s->busy&&s->created_loan==loan&&s->created_drain_serial==drain_serial&&s->serial==append_serial,
   "Created Unit reference FIRST POST occurrence changed");
 }
};
}
