/*
 * main3.cpp
 *
 *  Created on: Apr 18, 2015
 *      Author: dmitry
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <iostream>
#include <signal.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <sys/time.h>
#include <iostream>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <stack>
#include <map>
#include <dirent.h>
#include <sstream>
#include <cstdio>
#include <fstream>
#include <vector>
using namespace std;

bool is_run = true;
bool flag_reup = true;
string b_dir;
stack<string> stack_dir;
map<int, string> map_wd;
int inotify_fd;

void signal_handler(int sig) {
	switch (sig) {
	case SIGHUP:
		syslog(LOG_INFO, "Reload conf file\n");
		flag_reup = true;
		break;
	case SIGTERM:
		syslog(LOG_INFO, "Close daemon...\n");
		is_run = false;
		break;
	}
}

void _inotify_add_watch(int __fd, const char *__name, uint32_t __mask) {
	int wd = inotify_add_watch(__fd, __name, __mask);
	if (wd < 0) {
		syslog(LOG_ERR, "inotify_add_watch(...)\n");
	} else {
		map_wd[wd] = __name;
		syslog(LOG_INFO, "watching: ");
		syslog(LOG_INFO, __name);
	}
}

void print_event(struct inotify_event *event) {
	string name;
	string type;

	if (event->len) {
		name = event->name;
	}
	if (event->mask & IN_ISDIR) {
		type = "Dir";
	} else {
		type = "File";
	}

	std::stringstream stream;
	std::stringstream abs_pwd;
	abs_pwd << map_wd[event->wd] << "/" << name;

	stream << " name:" << abs_pwd.str() << "; type:" << type << "; event:";

	switch (event->mask) {
	case IN_ACCESS:
		stream << "IN_ACCESS" << endl;
		return;
		break;
	case IN_MODIFY:
		stream << "IN_MODIFY" << endl;
		break;
	case IN_ATTRIB:
		stream << "IN_ATTRIB" << endl;
		break;
	case IN_CLOSE_WRITE:
		stream << "IN_CLOSE_WRITE" << endl;
		return;
		break;
	case IN_CLOSE_NOWRITE:
		stream << "IN_CLOSE_NOWRITE" << endl;
		return;
		break;
	case IN_CLOSE:
		stream << "IN_CLOSE" << endl;
		return;
		break;
	case IN_OPEN:
		stream << "IN_OPEN" << endl;
		return;
		break;
	case IN_MOVED_FROM:
		stream << "IN_MOVED_FROM" << endl;
		return;
		break;
	case IN_MOVED_TO:
		stream << "IN_MOVED_TO" << endl;
		return;
		break;
	case IN_MOVE:
		stream << "IN_MOVE" << endl;
		return;
		break;
	case IN_CREATE:
		stream << "IN_CREATE" << endl;
		break;
	case IN_DELETE:
		stream << "IN_DELETE" << endl;
		break;
	case IN_DELETE_SELF:
		stream << "IN_DELETE_SELF" << endl;
		break;
	case IN_MOVE_SELF:
		stream << "IN_MOVE_SELF" << endl;
		return;
		break;
	case IN_CREATE | IN_ISDIR:
		stream << "IN_CREATE | IN_ISDIR" << endl;
		_inotify_add_watch(inotify_fd, abs_pwd.str().c_str(), IN_ALL_EVENTS);
		break;
	case IN_DELETE | IN_ISDIR:
		stream << "IN_DELETE | IN_ISDIR" << endl;
		break;
	default:
		stream << "ENOTHER_EVENT" << endl;
		return;
		break;
	}

	syslog(LOG_INFO, stream.str().c_str());
}

void sub_dir(const char *dir) {
	DIR* dirp = opendir(dir);
	struct dirent *dp = readdir(dirp);
	stack_dir.push(dir);
	while (dp != NULL) {
		string name = dp->d_name;

		if ((name != "." && name != "..") && dp->d_type == DT_DIR) {
			name.insert(0, "/");
			name.insert(0, dir);
			stack_dir.push(name);
		}

		dp = readdir(dirp);
	}

}

void erase_all() {
	close(inotify_fd);
	map_wd.clear();
	b_dir.clear();
}

void read_conf() {
	vector<string> f_data;
	string line;
	ifstream myfile("./src/daemon_files.conf");
	if (myfile.is_open()) {
		while (getline(myfile, line)) {
			f_data.push_back(line);
			line.append("\n");
//			syslog(LOG_INFO, line.c_str());
		}
		b_dir = f_data[0];
		myfile.close();
	} else {
		syslog(LOG_INFO, "error open file\n");
	}
}

void init() {
	read_conf();
	sub_dir(b_dir.c_str());
	inotify_fd = inotify_init();
	if (inotify_fd < 0) {
		syslog(LOG_ERR, "inotify_init() error\n");
	}

	// Watching ...
	while (!stack_dir.empty()) {
		string pwd = stack_dir.top();
		stack_dir.pop();
		syslog(LOG_INFO, "notify add watch to ");
		syslog(LOG_INFO, pwd.c_str());
		syslog(LOG_INFO, "\n");
		_inotify_add_watch(inotify_fd, pwd.c_str(), IN_ALL_EVENTS);
	}
}

void work() {
	init();
	while (is_run) {
		if (flag_reup) {
			erase_all();
			init();
			flag_reup = false;
		}
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(inotify_fd, &rfds);
		if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) > 0) {
			char buffer[16384];
			read(inotify_fd, buffer, 16384);
			struct inotify_event *event =
					(struct inotify_event *) &buffer[(size_t) 0];

			print_event(event);
		}
	}
	// Watching end
	syslog(LOG_INFO, "inotify close");
	if (close(inotify_fd) < 0) {
		syslog(LOG_ERR, "inotify close error");
	}
}

int main(int argc, char **argv) {
	int pid;

//	return 0;

// создаем потомка
	pid = fork();

	if (pid == -1) // если не удалось запустить потомка
			{
		printf("Error: Start Daemon failed (%s)\n", strerror(errno));

		return -1;
	} else if (!pid) { // если это потомок
		umask(0);
		setsid();
//		chdir("/");

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		signal(SIGHUP, signal_handler);
		signal(SIGTERM, signal_handler);

		openlog("daemon_files", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

		work();

		return 0;
	} else {
		return 0;
	}

}
