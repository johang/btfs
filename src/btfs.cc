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

#define FUSE_USE_VERSION 26

#include <cstdlib>

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fuse.h>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/peer_request.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>

#include <curl/curl.h>

#include "btfs.h"

#define RETV(s, v) { s; return v; };

using namespace btfs;

libtorrent::session *session = NULL;

libtorrent::torrent_handle handle;

pthread_t alert_thread;

std::list<Read*> reads;

// First piece index of the current sliding window
int cursor;

std::map<std::string,int> files;
std::map<std::string,std::set<std::string> > dirs;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;

static struct btfs_params params;

static bool
move_to_next_unfinished(int& piece) {
	for (; piece < handle.get_torrent_info().num_pieces(); piece++) {
		if (!handle.have_piece(piece))
			return true;
	}

	return false;
}

static void
jump(int piece, int size) {
	int tail = piece;

	if (!move_to_next_unfinished(tail))
		return;

	cursor = tail;

	int pl = handle.get_torrent_info().piece_length();

	for (int b = 0; b < 16 * pl; b += pl) {
		handle.piece_priority(tail++, 7);
	}

	for (int o = (tail - piece) * pl; o < size + pl - 1; o += pl) {
		handle.piece_priority(tail++, 1);
	}
}

static void
advance() {
	jump(cursor, 0);
}

Read::Read(char *buf, int index, int offset, int size) {
	libtorrent::torrent_info metadata = handle.get_torrent_info();

	libtorrent::file_entry file = metadata.file_at(index);

	while (size > 0 && offset < file.size) {
		libtorrent::peer_request part = metadata.map_file(index,
			offset, size);

		part.length = std::min(
			metadata.piece_size(part.piece) - part.start,
			part.length);

		parts.push_back(Part(part, buf));

		size -= part.length;
		offset += part.length;
		buf += part.length;
	}
}

void Read::copy(int piece, char *buffer, int size) {
	for (parts_iter i = parts.begin(); i != parts.end(); ++i) {
		if (i->part.piece == piece && !i->filled)
			i->filled = (memcpy(i->buf, buffer + i->part.start,
				i->part.length)) != NULL;
	}
}

void Read::trigger() {
	for (parts_iter i = parts.begin(); i != parts.end(); ++i) {
		if (handle.have_piece(i->part.piece))
			handle.read_piece(i->part.piece);
	}
}

bool Read::finished() {
	for (parts_iter i = parts.begin(); i != parts.end(); ++i) {
		if (!i->filled)
			return false;
	}

	return true;
}

int Read::size() {
	int s = 0;

	for (parts_iter i = parts.begin(); i != parts.end(); ++i) {
		s += i->part.length;
	}

	return s;
}

int Read::read() {
	if (size() <= 0)
		return 0;

	// Trigger reads of finished pieces
	trigger();

	// Move sliding window to first piece to serve this request
	jump(parts.front().part.piece, size());

	while (!finished())
		// Wait for any piece to downloaded
		pthread_cond_wait(&signal_cond, &lock);

	return size();
}

static void
setup() {
	printf("Got metadata. Now ready to start downloading.\n");

	libtorrent::torrent_info ti = handle.get_torrent_info();

	if (params.browse_only)
		handle.pause();

	for (int i = 0; i < ti.num_files(); ++i) {
		// Initially, don't download anything
		handle.file_priority(i, 0);

		std::string parent("");

		char *p = strdup(ti.file_at(i).path.c_str());

		for (char *x = strtok(p, "/"); x; x = strtok(NULL, "/")) {
			if (strlen(x) <= 0)
				continue;

			if (parent.length() <= 0)
				// Root dir <-> children mapping
				dirs["/"].insert(x);
			else
				// Non-root dir <-> children mapping
		 		dirs[parent].insert(x);

			parent += "/";
			parent += x;
		}

		free(p);

		// Path <-> file index mapping
		files["/" + ti.file_at(i).path] = i;
	}
}

static void
handle_read_piece_alert(libtorrent::read_piece_alert *a) {
	printf("%s: piece %d size %d\n", __func__, a->piece, a->size);

	pthread_mutex_lock(&lock);

	for (reads_iter i = reads.begin(); i != reads.end(); ++i) {
		(*i)->copy(a->piece, a->buffer.get(), a->size);
	}

	pthread_mutex_unlock(&lock);

	// Wake up all threads waiting for download
	pthread_cond_broadcast(&signal_cond);
}

static void
handle_piece_finished_alert(libtorrent::piece_finished_alert *a) {
	printf("%s: %d\n", __func__, a->piece_index);

	pthread_mutex_lock(&lock);

	for (reads_iter i = reads.begin(); i != reads.end(); ++i) {
		(*i)->trigger();
	}

	// Advance sliding window
	advance();

	pthread_mutex_unlock(&lock);
}

static void
handle_metadata_failed_alert(libtorrent::metadata_failed_alert *a) {
	//printf("%s\n", __func__);
}

static void
handle_torrent_added_alert(libtorrent::torrent_added_alert *a) {
	//printf("%s()\n", __func__);

	pthread_mutex_lock(&lock);

	handle = a->handle;

	if (a->handle.status(0).has_metadata)
		setup();

	pthread_mutex_unlock(&lock);
}

static void
handle_metadata_received_alert(libtorrent::metadata_received_alert *a) {
	//printf("%s\n", __func__);

	pthread_mutex_lock(&lock);

	handle = a->handle;

	setup();

	pthread_mutex_unlock(&lock);
}

static void
handle_alert(libtorrent::alert *a) {
	switch (a->type()) {
	case libtorrent::read_piece_alert::alert_type:
		handle_read_piece_alert(
			(libtorrent::read_piece_alert *) a);
		break;
	case libtorrent::piece_finished_alert::alert_type:
		handle_piece_finished_alert(
			(libtorrent::piece_finished_alert *) a);
		break;
	case libtorrent::metadata_failed_alert::alert_type:
		handle_metadata_failed_alert(
			(libtorrent::metadata_failed_alert *) a);
		break;
	case libtorrent::metadata_received_alert::alert_type:
		handle_metadata_received_alert(
			(libtorrent::metadata_received_alert *) a);
		break;
	case libtorrent::torrent_added_alert::alert_type:
		handle_torrent_added_alert(
			(libtorrent::torrent_added_alert *) a);
		break;
	default:
		//printf("unknown event %d\n", a->type());
		break;
	}

	delete a;
}

static void*
alert_queue_loop(void *data) {
	int oldstate, oldtype;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	while (1) {
		if (!session->wait_for_alert(libtorrent::seconds(1)))
			continue;

		std::deque<libtorrent::alert*> alerts;

		session->pop_alerts(&alerts);

		std::for_each(alerts.begin(), alerts.end(), handle_alert);
	}

	return NULL;
}

static bool
is_dir(const char *path) {
	return dirs.find(path) != dirs.end();
}

static bool
is_file(const char *path) {
	return files.find(path) != files.end();
}

static int
btfs_getattr(const char *path, struct stat *stbuf) {
	if (!is_dir(path) && !is_file(path) && strcmp(path, "/") != 0)
		return -ENOENT;

	pthread_mutex_lock(&lock);

	memset(stbuf, 0, sizeof (*stbuf));

	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();

	if (strcmp(path, "/") == 0 || is_dir(path)) {
		stbuf->st_mode = S_IFDIR | 0755;
	} else {
		libtorrent::file_entry file =
			handle.get_torrent_info().file_at(files[path]);

		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_size = file.size;
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

static int
btfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	if (!is_dir(path) && !is_file(path) && strcmp(path, "/") != 0)
		return -ENOENT;

	if (is_file(path))
		return -ENOTDIR;

	pthread_mutex_lock(&lock);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (std::set<std::string>::iterator i = dirs[path].begin();
			i != dirs[path].end(); ++i) {
		filler(buf, i->c_str(), NULL, 0);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

static int
btfs_open(const char *path, struct fuse_file_info *fi) {
	if (!is_dir(path) && !is_file(path))
		return -ENOENT;

	if (is_dir(path))
		return -EISDIR;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int
btfs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	//printf("%s: %s %lu %ld\n", __func__, path, size, offset);

	if (!is_dir(path) && !is_file(path))
		return -ENOENT;

	if (is_dir(path))
		return -EISDIR;

	if (params.browse_only)
		return -EACCES;

	pthread_mutex_lock(&lock);

	Read *r = new Read(buf, files[path], offset, size);

	reads.push_back(r);

	// Wait for read to finish
	int s = r->read();

	reads.remove(r);

	delete r;

	pthread_mutex_unlock(&lock);

	return s;
}

static void *
btfs_init(struct fuse_conn_info *conn) {
	pthread_mutex_lock(&lock);

	libtorrent::add_torrent_params *p = (libtorrent::add_torrent_params *)
		fuse_get_context()->private_data;

	int alerts =
		//libtorrent::alert::all_categories |
		libtorrent::alert::storage_notification |
		libtorrent::alert::progress_notification |
		libtorrent::alert::status_notification |
		libtorrent::alert::error_notification;

	session = new libtorrent::session(
		libtorrent::fingerprint(
			"LT",
			LIBTORRENT_VERSION_MAJOR,
			LIBTORRENT_VERSION_MINOR,
			0,
			0),
		std::make_pair(6881, 6889),
		"0.0.0.0",
		libtorrent::session::add_default_plugins,
		alerts);

	pthread_create(&alert_thread, NULL, alert_queue_loop, NULL);

#ifndef __APPLE__
	pthread_setname_np(alert_thread, "alert");
#endif

	libtorrent::session_settings se = session->settings();

	se.strict_end_game_mode = false;
	se.announce_to_all_trackers = true;
	se.announce_to_all_tiers = true;

	session->set_settings(se);
	session->async_add_torrent(*p);

	pthread_mutex_unlock(&lock);

	return NULL;
}

static void
btfs_destroy(void *user_data) {
	pthread_mutex_lock(&lock);

	pthread_cancel(alert_thread);
	pthread_join(alert_thread, NULL);

	std::string path = handle.save_path();

	session->remove_torrent(handle,
		params.keep ? 0 : libtorrent::session::delete_files);

	delete session;

	rmdir(path.c_str());

	pthread_mutex_unlock(&lock);
}

static bool
populate_target(libtorrent::add_torrent_params& p, char *arg) {
	std::string templ;

	if (arg) {
		templ += arg;
	} else if (getenv("HOME")) {
		templ += getenv("HOME");
		templ += "/btfs";
	} else {
		templ += "/tmp/btfs";
	}

	if (mkdir(templ.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
		if (errno != EEXIST)
			RETV(fprintf(stderr, "Failed to create target: %m\n"),
				false);
	}

	templ += "/btfs-XXXXXX";

	char *s = strdup(templ.c_str());

	if (mkdtemp(s) != NULL) {
		char *x = realpath(s, NULL);

		if (x)
			p.save_path = x;
		else
			perror("Failed to expand target");

		free(x);		
	} else {
		perror("Failed to generate target");
	}

	free(s);

	return p.save_path.length() > 0;
}

static size_t
handle_http(void *contents, size_t size, size_t nmemb, void *userp) {
	Array *output = (Array *) userp;

	// Offset into buffer to write to
	size_t off = output->size;

	output->expand(nmemb * size);

	memcpy(output->buf + off, contents, nmemb * size);

	// Must return number of bytes copied
	return nmemb * size;
}

static bool
populate_metadata(libtorrent::add_torrent_params& p, const char *arg) {
	std::string uri(arg);

	if (uri.find("http:") == 0 || uri.find("https:") == 0) {
		Array output;

		CURL *ch = curl_easy_init();

		curl_easy_setopt(ch, CURLOPT_URL, uri.c_str());
		curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, handle_http); 
		curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) &output); 
		curl_easy_setopt(ch, CURLOPT_USERAGENT, "btfs/" VERSION);
		curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

		CURLcode res = curl_easy_perform(ch);

		if(res != CURLE_OK)
			RETV(fprintf(stderr, "curl failed: %s\n",
				curl_easy_strerror(res)), false);

		curl_easy_cleanup(ch);

		libtorrent::error_code ec;

		p.ti = new libtorrent::torrent_info((const char *) output.buf,
			output.size, ec);

		if (ec)
			RETV(fprintf(stderr, "Can't load metadata: %s\n",
				ec.message().c_str()), false);

		if (params.browse_only)
			p.flags |= libtorrent::add_torrent_params::flag_paused;
	} else if (uri.find("magnet:") == 0) {
		libtorrent::error_code ec;

		parse_magnet_uri(uri, p, ec);

		if (ec)
			RETV(fprintf(stderr, "Can't load magnet: %s\n",
				ec.message().c_str()), false);
	} else {
		char *r = realpath(uri.c_str(), NULL);

		if (!r)
			RETV(fprintf(stderr, "Can't find metadata: %m\n"),
				false);

		libtorrent::error_code ec;

		p.ti = new libtorrent::torrent_info(r, ec);

		free(r);

		if (ec)
			RETV(fprintf(stderr, "Can't load metadata: %s\n",
				ec.message().c_str()), false);

		if (params.browse_only)
			p.flags |= libtorrent::add_torrent_params::flag_paused;
	}

	return true;
}

#define BTFS_OPT(t, p, v) { t, offsetof(struct btfs_params, p), v }

static const struct fuse_opt btfs_opts[] = {
	BTFS_OPT("-v",            version,     1),
	BTFS_OPT("--version",     version,     1),
	BTFS_OPT("-h",            help,        1),
	BTFS_OPT("--help",        help,        1),
	BTFS_OPT("-b",            browse_only, 1),
	BTFS_OPT("--browse-only", browse_only, 1),
	BTFS_OPT("-k",            keep,        1),
	BTFS_OPT("--keep",        keep,        1),
	FUSE_OPT_END
};

static int
btfs_process_arg(void *data, const char *arg, int key,
		struct fuse_args *outargs) {
	// Number of NONOPT options so far
	static int n = 0;

	struct btfs_params *params = (struct btfs_params *) data;

	if (key == FUSE_OPT_KEY_NONOPT) {
		if (n++ == 0)
			params->metadata = arg;

		return n <= 1 ? 0 : 1;
	}

	return 1;
}

int
main(int argc, char *argv[]) {
	struct fuse_operations btfs_ops;
	memset(&btfs_ops, 0, sizeof (btfs_ops));

	btfs_ops.getattr = btfs_getattr;
	btfs_ops.readdir = btfs_readdir;
	btfs_ops.open = btfs_open;
	btfs_ops.read = btfs_read;
	btfs_ops.init = btfs_init;
	btfs_ops.destroy = btfs_destroy;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &params, btfs_opts, btfs_process_arg))
		RETV(fprintf(stderr, "Failed to parse options\n"), -1);

	if (!params.metadata)
		params.help = 1;

	if (params.version) {
		// Print version
		printf(PACKAGE " version: " VERSION "\n");

		// Let FUSE print more versions
		fuse_opt_add_arg(&args, "--version");
		fuse_main(args.argc, args.argv, &btfs_ops, NULL);

		return 0;
	}

	if (params.help) {
		// Print usage
		printf("usage: " PACKAGE " [options] metadata mountpoint\n");
		printf("\n");
		printf("btfs options:\n");
		printf("    --version -v           show version information\n");
		printf("    --help -h              show this message\n");
		printf("    --browse-only -b       download metadata only\n");
		printf("    --keep -k              keep files after unmount\n");
		printf("\n");

		// Let FUSE print more help
		fuse_opt_add_arg(&args, "-ho");
		fuse_main(args.argc, args.argv, &btfs_ops, NULL);

		return 0;
	}

	libtorrent::add_torrent_params p;

	p.flags &= ~libtorrent::add_torrent_params::flag_auto_managed;
	p.flags &= ~libtorrent::add_torrent_params::flag_paused;

	if (!populate_target(p, NULL))
		return -1;

	curl_global_init(CURL_GLOBAL_ALL);

	if (!populate_metadata(p, params.metadata))
		return -1;

	fuse_main(args.argc, args.argv, &btfs_ops, (void *) &p);

	curl_global_cleanup();

	return 0;
}
