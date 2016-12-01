// libTorrent - BitTorrent library
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <cerrno>
#include <cstring>

#include "bind_manager.h"

#include "net/socket_fd.h"
#include "rak/socket_address.h"
#include "torrent/utils/log.h"

#define LT_LOG(log_fmt, ...)                                            \
  lt_log_print(LOG_CONNECTION_BIND, "bind: " log_fmt, __VA_ARGS__);
#define LT_LOG_SA(log_fmt, sa, ...)                                     \
  lt_log_print(LOG_CONNECTION_BIND, "bind->%s: " log_fmt, (sa)->pretty_address_str().c_str(), __VA_ARGS__);

namespace torrent {

bind_struct::bind_struct(const sockaddr* a, int f) :
  flags(f)
{
  auto addr = new rak::socket_address;
  addr->copy_sockaddr(a);

  address = std::unique_ptr<const sockaddr>(addr->c_sockaddr());
}

inline const rak::socket_address*
bind_struct_cast_address(const bind_struct& bs) {
  return rak::socket_address::cast_from(bs.address.get());
}

inline bool
bind_manager_is_bindable(const rak::socket_address* bind_address) {
  switch (bind_address->family()) {
  case AF_INET:
    return !bind_address->sa_inet()->is_address_any() && bind_address->sa_inet()->is_port_any();
  case AF_INET6:
    return !bind_address->sa_inet6()->is_address_any() && bind_address->sa_inet6()->is_port_any();
  case AF_UNSPEC:
  default:
    return false;
  };
}

bind_manager::bind_manager() {
  auto default_address = new rak::socket_address;
  default_address->clear();

  m_default_address = std::unique_ptr<const sockaddr>(default_address->c_sockaddr());
}

void
bind_manager::add_bind(const sockaddr* bind_addr, int flags) {
  auto bind_address = rak::socket_address::cast_from(bind_addr);

  if (bind_manager_is_bindable(bind_address)) {
    LT_LOG("add bind failed, address is not bindable (flags:%i address:%s)",
           flags, bind_address->pretty_address_str().c_str());

    // Throw here?
    return;
  }

  LT_LOG("added bind (flags:%i address:%s)",
         flags, bind_address->pretty_address_str().c_str());

  // TODO: Add a way to set the order.
  base_type::push_back(bind_struct(bind_addr, flags));
}

int
bind_manager::connect_socket(const sockaddr* connect_sockaddr, int flags, alloc_fd_ftor alloc_fd) const {
  int file_desc = alloc_fd();

  if (file_desc == -1) {
    LT_LOG("connect failed, could not allocate socket (errno:%i message:'%s')",
           errno, std::strerror(errno));

    return file_desc;
  }

  SocketFd socket_fd(file_desc);

  auto bind_address = rak::socket_address::cast_from(m_default_address.get());

  if (!empty()) {
    auto itr = begin();
    auto itr_address = bind_struct_cast_address(*itr);

    if (!socket_fd.bind(*itr_address)) {
      LT_LOG_SA("connect failed, bind unsuccessful (fd:%i errno:%i message:'%s')",
                itr_address, file_desc, errno, std::strerror(errno));

      return false;
    }

    bind_address = itr_address;
  }

  if (connect_sockaddr != NULL) {
    auto connect_socket_addr = rak::socket_address::cast_from(connect_sockaddr);

    if (!socket_fd.connect(*connect_socket_addr)) {
      LT_LOG_SA("connect failed (fd:%i address:%s errno:%i message:'%s')",
                bind_address, file_desc, connect_socket_addr->pretty_address_str().c_str(), errno, std::strerror(errno));

      errno = 0;
      return false;
    }

    LT_LOG_SA("connect success (fd:%i address:%s)",
              bind_address, file_desc, connect_socket_addr->pretty_address_str().c_str());
  }

  return true;
}

}