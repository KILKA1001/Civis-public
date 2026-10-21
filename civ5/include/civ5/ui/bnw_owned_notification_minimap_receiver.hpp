#pragma once

#include <memory>
#include <thread>

namespace civ5::ui {
class BnwClientSceneFrame;
class BnwNotificationPublicationLender;
class BnwNotificationMinimapReturn;

// Closed Scene-owned prerequisite. Native type-0x60 matrix/drain admission is
// still required: no physical queue append or successful opaque return exists.
class BnwOwnedNotificationMinimapReceiver final {
 public:
  ~BnwOwnedNotificationMinimapReceiver();
  BnwOwnedNotificationMinimapReceiver(const BnwOwnedNotificationMinimapReceiver&)=delete;
  BnwOwnedNotificationMinimapReceiver& operator=(const BnwOwnedNotificationMinimapReceiver&)=delete;
  BnwOwnedNotificationMinimapReceiver(BnwOwnedNotificationMinimapReceiver&&)=delete;
  BnwOwnedNotificationMinimapReceiver& operator=(BnwOwnedNotificationMinimapReceiver&&)=delete;
 private:
  friend class BnwClientSceneFrame;
  friend class BnwNotificationPublicationLender;
  struct State;
  const std::thread::id creator_=std::this_thread::get_id();
  const std::shared_ptr<State> state_;
  explicit BnwOwnedNotificationMinimapReceiver(std::weak_ptr<const BnwClientSceneFrame>);
  static void pure(const std::shared_ptr<State>&);
  static std::shared_ptr<BnwOwnedNotificationMinimapReceiver>
      create_for_actual_scene(const BnwNotificationPublicationLender&);
  // Only the genuine lender may reach this endpoint. It cannot mint a
  // BnwNotificationMinimapReturn from a public result, sequence or bool.
  [[noreturn]] void dispatch_current_owned(const BnwNotificationPublicationLender&)const;
};
}
