/*
 * header.h
 *
 *  Created on: Apr 19, 2015
 *      Author: dmitry
 */

#ifndef HEADER_H_
#define HEADER_H_


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



#endif /* HEADER_H_ */
