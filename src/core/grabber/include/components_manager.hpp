#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "receiver.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/session.hpp>

namespace krbn {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(std::weak_ptr<version_monitor> weak_version_monitor) : dispatcher_client(),
                                                                            weak_version_monitor_(weak_version_monitor) {
    session_monitor_ = std::make_unique<pqrs::osx::session::monitor>(weak_dispatcher_);

    session_monitor_->console_user_id_changed.connect([this](auto&& uid) {
      uid_t console_user_server_socket_uid = 0;

      if (uid) {
        logger::get_logger()->info("current_console_user_id: {0}", *uid);
        console_user_server_socket_uid = *uid;
      } else {
        logger::get_logger()->info("current_console_user_id: none");
      }

      if (auto m = weak_version_monitor_.lock()) {
        m->async_manual_check();
      }

      // Prepare console_user_server_socket_directory
      {
        auto socket_file_path = console_user_server_client::make_console_user_server_socket_directory(console_user_server_socket_uid);
        mkdir(socket_file_path.c_str(), 0700);
        chown(socket_file_path.c_str(), console_user_server_socket_uid, 0);
        chmod(socket_file_path.c_str(), 0700);
      }

      // receiver_

      receiver_ = nullptr;
      receiver_ = std::make_unique<receiver>();
    });
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      session_monitor_ = nullptr;
      receiver_ = nullptr;
    });
  }

  void async_start(void) const {
    enqueue_to_dispatcher([this] {
      session_monitor_->async_start(std::chrono::milliseconds(1000));
    });
  }

private:
  std::weak_ptr<version_monitor> weak_version_monitor_;
  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn
