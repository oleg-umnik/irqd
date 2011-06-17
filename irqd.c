/*
 * irqd.c
 *
 * Holger Eitzenberger <heitzenberger@astaro.com>, 2011.
 */
#include <getopt.h>

#include "irqd.h"
#include "event.h"
#include "cpu.h"
#include "interface.h"

#define PID_FILE		"irqd.pid"


/* if set allows to access files below /sys and /proc below a subdirectory */
char *irqd_prefix;
bool no_daemon;
int verbose;


void
die(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(1);
}

static int
check_opts(int argc, char *argv[])
{
	static struct option lopts[] = {
		{ "verbose", 0, NULL, 'v' },
		{ "version", 0, NULL, 0 },
		{ 0 }
	};
	int c, idx = 0;

	while ((c = getopt_long(argc, argv, "dv", lopts, &idx)) != -1) {
		if (!c) {				/* long-only option */
			switch (idx) {
			case 1:				/* version */
				break;

			default:
				return -1;
			}
			continue;
		}

		switch (c) {
		case 'd':
			no_daemon = true;
			break;

		case 'v':				/* verbose */
			verbose++;
			break;

		case '?':
			return -1;
		}
	}

	return 0;
}

/* returned string needs to be freed by caller */
char *
id_path(const char *path)
{
	char *buf = malloc(PATH_MAX);

	BUG_ON(*path != '/');
	if (buf) {
		snprintf(buf, PATH_MAX, "%s%s", irqd_prefix, path);
		buf[PATH_MAX - 1] = '\0';
	} else
		OOM();

	return buf;
}

int
irq_set_affinity(int irq, uint64_t mask)
{
	char path[PATH_MAX], buf[16];
	int fd, len, nwritten;

	snprintf(path, sizeof(path), "/proc/irq/%d/smp_affinity", irq);
	if ((fd = open(path, O_WRONLY | O_CLOEXEC)) < 0) {
		err("%s: %m", path);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "%" PRIx64 "\n", mask);
	nwritten = write(fd, buf, len);
	BUG_ON(nwritten != len);

	close(fd);

	return 0;
}

char *
xstrncpy(char *dst, const char *src, size_t n)
{
	strncpy(dst, src, n);
	if (dst[n - 1])
		dst[n - 1] = '\0';
	return dst;
}

static int
daemonize(void)
{
    int fd;

    switch (fork()) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(0);
    }

    if (setsid() == -1)
        return (-1);

	chdir("/");

    if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2)
            close (fd);
    }

    return 0;
}

static void
irqd_at_exit(void)
{
	char pidfile[PATH_MAX];

	snprintf(pidfile, sizeof(pidfile), "%s%s", _PATH_VARRUN, PID_FILE);
	unlink(pidfile);
}

/* write PID file unless already running */
static int
write_pid(void)
{
	char path[PATH_MAX];
	FILE *fp = NULL;
	int fd;

	snprintf(path, sizeof(path), "%s%s", _PATH_VARRUN, PID_FILE);
	if ((fd = open(path, O_RDWR | O_CREAT, 0644)) < 0) {
		err("already running");
		return -1;
	}

	if ((fp = fdopen(fd, "r+")) == NULL) {
		err("%s: %m", PID_FILE);
		close(fd);

		return -1;
	}

	fprintf(fp, "%u\n", getpid());

	fclose(fp);

	atexit(irqd_at_exit);

	return 0;
}

int
main(int argc, char *argv[])
{
	log_init();

	if (check_opts(argc, argv) < 0)
		exit(EXIT_FAILURE);

	if (!no_daemon)
		openlog("irqd", LOG_PID | LOG_NDELAY, LOG_DAEMON);

	if ((irqd_prefix = getenv("IRQD_PREFIX")) == NULL)
		irqd_prefix = "";

	setlocale(LC_ALL, "");

	if (geteuid()) {
		err("root required");
		exit(1);
	}

	ev_init();
	cpu_init();
	if_init();

	if(cpu_count() == 1) {
		log("terminating because single CPU");
		exit(0);
	}

	if (!no_daemon)
		daemonize();
	if (write_pid() < 0)
		exit(1);

	ev_dispatch();

	if_fini();
	cpu_fini();
	ev_fini();

	return 0;
}
