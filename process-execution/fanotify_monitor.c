#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdarg.h>

static bool use_syslog = false;

/* Log output to syslog or stdout */
static void log_event(const char *format, ...) {
	va_list args;
	va_start(args, format);

	if (use_syslog) {
		vsyslog(LOG_INFO, format, args);
	} else {
		vprintf(format, args);
		fflush(stdout);
	}

	va_end(args);
}

/* Read /proc/<pid>/exe */
static int read_proc_exe(pid_t pid, char *buf, size_t bufsize) {
	char proc_path[64];
	ssize_t len;

	snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", pid);
	len = readlink(proc_path, buf, bufsize - 1);

	if (len == -1) {
		/* No exe link - likely a kernel thread, try comm instead */
		FILE *fp;
		snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pid);
		fp = fopen(proc_path, "r");
		if (fp) {
			if (fgets(buf, bufsize, fp)) {
				/* Remove trailing newline */
				len = strlen(buf);
				if (len > 0 && buf[len-1] == '\n') {
					buf[len-1] = '\0';
				}
				fclose(fp);
				return 0;
			}
			fclose(fp);
		}
		snprintf(buf, bufsize, "<kernel thread>");
		return -1;
	}

	buf[len] = '\0';
	return 0;
}

/* Read parent PID from /proc/<pid>/stat */
static pid_t read_ppid(pid_t pid) {
	char proc_path[64];
	char line[512];
	FILE *fp;
	pid_t ppid = -1;

	snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", pid);
	fp = fopen(proc_path, "r");
	if (!fp) {
		return -1;
	}

	if (fgets(line, sizeof(line), fp)) {
		/* Format: pid (comm) state ppid ... */
		char *p = strrchr(line, ')');
		if (p) {
			sscanf(p + 1, " %*c %d", &ppid);
		}
	}

	fclose(fp);
	return ppid;
}

/* Get path from file descriptor */
static int get_path_from_fd(int fd, char *buf, size_t bufsize) {
	char fd_path[64];
	ssize_t len;

	snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
	len = readlink(fd_path, buf, bufsize - 1);

	if (len == -1) {
		snprintf(buf, bufsize, "<unknown>");
		return -1;
	}

	buf[len] = '\0';
	return 0;
}

/* Convert mask to permission string */
static const char* mask_to_perm(uint64_t mask) {
	if (mask & FAN_OPEN_EXEC_PERM || mask & FAN_OPEN_EXEC) {
		return "execute";
	} else if (mask & FAN_OPEN_PERM || mask & FAN_OPEN) {
		return "open";
	} else if (mask & FAN_ACCESS_PERM || mask & FAN_ACCESS) {
		return "access";
	}
	return "unknown";
}

int main(int argc, char *argv[]) {
	int fan_fd;
	char buf[4096];
	ssize_t len;
	const struct fanotify_event_metadata *metadata;
	char exe_path[PATH_MAX];
	char file_path[PATH_MAX];
	const char *mount_point = "/";

	/* Parse arguments */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--syslog") == 0) {
			use_syslog = true;
			openlog("fanotify_monitor", LOG_PID | LOG_CONS, LOG_DAEMON);
		} else if (argv[i][0] != '-') {
			mount_point = argv[i];
		}
	}

	if (!use_syslog) {
		fprintf(stderr, "fanotify monitor - fapolicyd format output\n");
		fprintf(stderr, "Usage: %s [--syslog] [mount_point]\n", argv[0]);
		fprintf(stderr, "Monitoring: %s\n", mount_point);
		fprintf(stderr, "Output: %s\n\n", use_syslog ? "syslog" : "stdout");
	}

	/* Initialize fanotify */
	fan_fd = fanotify_init(FAN_CLASS_CONTENT | FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS,
			       O_RDONLY | O_LARGEFILE);

	if (fan_fd == -1) {
		perror("fanotify_init");
		exit(EXIT_FAILURE);
	}

	/* Mark filesystem for monitoring */
	uint64_t mask = FAN_OPEN_EXEC_PERM | FAN_OPEN_PERM;

	if (fanotify_mark(fan_fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			  mask, AT_FDCWD, mount_point) == -1) {
		perror("fanotify_mark");
		close(fan_fd);
		exit(EXIT_FAILURE);
	}

	if (use_syslog) {
		syslog(LOG_INFO, "fanotify_monitor started on %s", mount_point);
	}

	/* Event loop */
	while (1) {
		len = read(fan_fd, buf, sizeof(buf));

		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		if (len <= 0) {
			continue;
		}

		metadata = (struct fanotify_event_metadata *)buf;

		while (FAN_EVENT_OK(metadata, len)) {
			if (metadata->vers != FANOTIFY_METADATA_VERSION) {
				fprintf(stderr, "Mismatch of fanotify metadata version.\n");
				exit(EXIT_FAILURE);
			}

			if (metadata->fd >= 0) {
				pid_t pid = metadata->pid;
				pid_t ppid;
				const char *perm_type = mask_to_perm(metadata->mask);

				read_proc_exe(pid, exe_path, sizeof(exe_path));
				get_path_from_fd(metadata->fd, file_path, sizeof(file_path));
				ppid = read_ppid(pid);

				/* fapolicyd format output with ppid */
				log_event("perm=%s pid=%d ppid=%d exe=%s : path=%s\n",
					  perm_type, pid, ppid, exe_path, file_path);

				/* Allow operation */
				struct fanotify_response response = {
					.fd = metadata->fd,
					.response = FAN_ALLOW
				};

				if (write(fan_fd, &response, sizeof(response))){
					;
				}
				close(metadata->fd);
			}

			metadata = FAN_EVENT_NEXT(metadata, len);
		}
	}

	if (use_syslog) {
		closelog();
	}
	close(fan_fd);
	return 0;
}

