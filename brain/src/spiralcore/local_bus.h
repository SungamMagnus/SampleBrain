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

// The sending half of the in-process OSC replacement. Every OSC_server
// registers itself under its port name ("8888" audio, "8889" process, "8890"
// feedback), and send() packs its arguments exactly the way liblo's handler
// used to, so the receiving code is untouched.

#pragma once

#include <string>
#include <cstring>
#include "command_ring_buffer.h"

class OSC_server;

namespace local_bus {

  void register_server(const std::string &port, OSC_server *server);
  void unregister_server(const std::string &port);
  bool deliver(const std::string &port, const command_ring_buffer::command &cmd);

  class packer {
  public:
    packer() : m_pos(0), m_ok(true) { m_types[0]=0; }

    void add(int v) { put('i', &v, 4); }
    void add(float v) { put('f', &v, 4); }
    void add(double v) { add((float)v); }
    void add(bool v) { add((int)v); }
    void add(const char *v) { put('s', v, strlen(v)+1); }
    void add(const std::string &v) { add(v.c_str()); }

    bool m_ok;
    char m_types[64];
    char m_data[COMMAND_DATA_SIZE];
    unsigned int m_pos;

  private:
    void put(char type, const void *src, unsigned int size) {
      size_t n=strlen(m_types);
      if (n+1>=sizeof(m_types) || m_pos+size>COMMAND_DATA_SIZE) { m_ok=false; return; }
      m_types[n]=type; m_types[n+1]=0;
      memcpy(m_data+m_pos,src,size);
      m_pos+=size;
    }
  };

  template <class... Args>
  bool send(const std::string &port, const char *path, Args... args) {
    packer p;
    (p.add(args), ...);
    if (!p.m_ok) return false;
    return deliver(port, command_ring_buffer::command(path,p.m_types,p.m_data,p.m_pos));
  }

}
