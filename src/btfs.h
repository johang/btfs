/*
Copyright 2015 Johan Gunnarsson <johan.gunnarsson@gmail.com>

This file is part of BTFS.

BTFS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BTFS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BTFS.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BTFS_H
#define BTFS_H

#include <libtorrent/peer_request.hpp>
#include <libtorrent/session_settings.hpp>

namespace btfs
{

class Part;
class Read;

typedef std::vector<Part>::iterator parts_iter;
typedef std::list<Read*>::iterator reads_iter;

class Part
{
	friend class Read;

public:
	Part(libtorrent::peer_request p, char *b) : part(p), buf(b),
			filled(false) {
	}

private:
	libtorrent::peer_request part;

	char *buf;

	bool filled;
};

class Read
{
public:
	Read(char *buf, int index, int offset, int size);

	void copy(int piece, char *buffer, int size);

	void trigger();

	bool finished();

	int size();

	int read();

private:
	std::vector<Part> parts;
};

class Array
{
public:
	Array() : buf(0), size(0) {
	}

	~Array() {
		free(buf);
	}

	bool expand(int n) {
		return (buf = (char *) realloc((void *) buf, size += n)) != NULL;
	}

	char *buf;

	size_t size;
};

class Log : public std::ofstream
{
public:
	Log(std::string p) : std::ofstream(p.c_str()), path(p) {
		if (!is_open())
			// If open log file fails, write to a dummy file
			open("/dev/null");
	}

	~Log() {
		close();

		if (remove(path.c_str()))
			perror("Failed to remove log");
	}

private:
	std::string path;
};

enum {
	KEY_VERSION,
	KEY_HELP,
};

std::map<std::string, libtorrent::proxy_settings::proxy_type> libtorrent_proxy_types = {
	{"socks4", libtorrent::proxy_settings::socks4},
	{"socks5h", libtorrent::proxy_settings::socks5},
	{"socks5", libtorrent::proxy_settings::socks5},
	{"http", libtorrent::proxy_settings::http}
};

std::map<libtorrent::proxy_settings::proxy_type, libtorrent::proxy_settings::proxy_type> libtorrent_authed_proxy_types = {
	{libtorrent::proxy_settings::socks5, libtorrent::proxy_settings::socks5_pw},
	{libtorrent::proxy_settings::http, libtorrent::proxy_settings::http_pw}
};

struct btfs_params {
	int version;
	int help;
	int browse_only;
	int keep;
	const char *proxy_hostname;
	int proxy_port;
	const char *proxy_type;
	const char *proxy_username;
	const char *proxy_password;
	const char *i2p_http_proxy;
	const char *metadata;
};

}

#endif
