#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <math.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>

#include <list>
#include <string>
#include <iostream>

#include "btfsstat.h"

#ifndef ENOATTR
#define ENOATTR ENODATA
#endif

using namespace btfs;

static bool
string_compare(const std::string& a, const std::string& b) {
	return a.compare(b) == 0;
}

static std::list<std::string>
list(std::string path) {
	std::list<std::string> files;

	DIR *dp = opendir(path.c_str());

	if (!dp)
		return files;

	for (struct dirent *ep = readdir(dp); ep; ep = readdir(dp)) {
		std::string f(ep->d_name);

		if (f != "." && f != "..")
			files.push_back(f);
	}

	closedir(dp);

	files.sort(string_compare);

	return files;
}

static void
scan(std::string indent, std::string d, std::string f) {
	struct stat s;
	memset(&s, 0, sizeof (s));

	std::string p = d + "/" + f;

	if (lstat(p.c_str(), &s) < 0)
		return;

	if (S_ISDIR(s.st_mode)) {
		printf("%s%s/\n", indent.c_str(), f.c_str());

		std::list<std::string> l = list(p);

		for (auto i = l.begin(); i != l.end(); ++i) {
			scan(indent + "    ", p, *i);
		}
	} else if (S_ISREG(s.st_mode)) {
		// Download progress for this file (in percent)
		long progress;

		if (s.st_size > 0)
			progress = lround((100.0 * 512.0 * (double) s.st_blocks) /
				(double) s.st_size);
		else
			progress = 100;

		printf("%s%s (%3ld%%)\n", indent.c_str(), f.c_str(), progress);
	}
}

int
main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: %s MOUNT_POINTS...\n", argv[0]);
		return 1;
	}

	for (int i = 1; i < argc; i++) {
#ifdef __APPLE__
		if (getxattr(argv[i], XATTR_IS_BTFS, NULL, 0, 0, 0) < 0) {
#else
		if (getxattr(argv[i], XATTR_IS_BTFS, NULL, 0) < 0) {
#endif
			printf("%s: %s is not a btfs mount: %s\n", argv[0], argv[i],
				strerror(errno));
			return 2;
		}

		char *root = realpath(argv[i], NULL);

		if (!root) {
			perror("failed to canonicalize path");
			return 3;
		}

		char *dir = strdup(root);
		char *base = strdup(root);

		scan("", dirname(root), basename(root));

		free(base);
		free(dir);

		free(root);
	}

	return 0;
}
