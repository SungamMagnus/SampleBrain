// copyright (C) 2004 david griffiths <dave@pawfal.org>
//
// this program is free software; you can redistribute it and/or modify
// it under the terms of the GNU general public license as published by
// the free software foundation; either version 2 of the license, or
// (at your option) any later version.
//
// this program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  see the
// GNU general public license for more details.
//
// you should have received a copy of the GNU general public license
// along with this program; if not, write to the free software
// foundation, inc., 59 temple place - suite 330, boston, MA 02111-1307, USA.

#ifndef OSC_SERVER
#define OSC_SERVER

#include <string>
#include "command_ring_buffer.h"

#ifdef SAMPLEBRAIN_LOCAL_BUS
// In-process build: the same named ports and message names, but messages go
// straight into the receiving server's ring buffer instead of over UDP, so the
// app needs no liblo. See local_bus.h for the sending side.
#include <mutex>
#else
#include <lo/lo.h>
#endif

class OSC_server
{
public:
	OSC_server(const std::string &port);
	~OSC_server();

	void run();
	bool get(command_ring_buffer::command& command) { return m_command_ring_buffer.get(command);}

#ifdef SAMPLEBRAIN_LOCAL_BUS
	// any thread may send; get() stays single-consumer
	bool deliver(const command_ring_buffer::command& command);

private:
	std::mutex m_send_mutex;
#else
private:
	static int default_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
	static void error_handler(int num, const char *m, const char *path);

	lo_server_thread m_server;
#endif
    std::string m_port;
	bool m_exit;
	command_ring_buffer m_command_ring_buffer;
};

#endif
