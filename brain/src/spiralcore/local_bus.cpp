// Copyright (C) 2026 Sungam
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// Built instead of OSC_server.cpp when SAMPLEBRAIN_LOCAL_BUS is defined.

#include <map>
#include <mutex>
#include "OSC_server.h"
#include "local_bus.h"

using namespace std;

static mutex &registry_mutex() { static mutex m; return m; }
static map<string,OSC_server*> &registry() { static map<string,OSC_server*> r; return r; }

void local_bus::register_server(const string &port, OSC_server *server) {
  lock_guard<mutex> lock(registry_mutex());
  registry()[port]=server;
}

void local_bus::unregister_server(const string &port) {
  lock_guard<mutex> lock(registry_mutex());
  registry().erase(port);
}

bool local_bus::deliver(const string &port, const command_ring_buffer::command &cmd) {
  lock_guard<mutex> lock(registry_mutex());
  auto i=registry().find(port);
  if (i==registry().end()) return false;
  return i->second->deliver(cmd);
}

OSC_server::OSC_server(const string &port) :
  m_port(port),
  m_exit(false),
  m_command_ring_buffer(262144)
{
}

OSC_server::~OSC_server() {
  m_exit=true;
  local_bus::unregister_server(m_port);
}

void OSC_server::run() {
  local_bus::register_server(m_port,this);
}

bool OSC_server::deliver(const command_ring_buffer::command &command) {
  lock_guard<mutex> lock(m_send_mutex);
  return m_command_ring_buffer.send(command);
}
