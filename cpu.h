/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Holger Eitzenberger <holger@eitzenberger.org>, Sophos, 2011.
 */
#ifndef CPU_H
#define CPU_H

#include <glib-2.0/glib.h>

#define HT_PER_CPU		2
#define HT_MASK			((1 << HT_PER_CPU) - 1)

#define OLD		0
#define NEW		1

struct proc_stat_cpu {
	unsigned long long psc_user;
	unsigned long long psc_nice;
	unsigned long long psc_system;
	unsigned long long psc_idle;
	unsigned long long psc_iowait;
	unsigned long long psc_irq;
	unsigned long long psc_softirq;
	unsigned long long psc_steal;
	unsigned long long psc_guest;
	unsigned long long psc_softirq_ctr;
};

/* /proc/stat */
struct proc_stat {
	size_t ps_len;
	unsigned long long ps_ctxt;
	unsigned long ps_btime;
	unsigned long ps_procs;
	unsigned long ps_procs_running;
	unsigned long ps_procs_blocked;
	struct proc_stat_cpu ps_cpu_total;
};

struct if_queue_info;

struct cpu_info {
	unsigned ci_num;
	GSList *ci_queues;
	unsigned ci_num_queues;
	unsigned ci_si_load;		/* softirq load (in percent) */

	/* /proc/net/softnet_stat */
	struct softnet_stat {
		unsigned total;
		unsigned dropped;
		unsigned time_squeeze;
		unsigned cpu_collision;
	} ci_ss[2];
	struct proc_stat_cpu ci_psc;
	struct proc_stat_cpu ci_psc_old;
};

#define CPU_SS_DIFF(ci, var) ((ci)->ci_ss[NEW].var - (ci)->ci_ss[OLD].var)

extern GSList *cpu_lru_list;
extern GSList *cpu_si_load_lru_list;

int cpu_init(void);
void cpu_fini(void);
unsigned cpu_count(void);
struct cpu_info *cpu_add_queue(int, struct interface *, int);
struct cpu_info *cpu_add_queue_lru(struct interface *, int);
int cpu_del_queue(int, struct if_queue_info *qi);
struct cpu_info *cpu_nth(int);

int cpu_read_stat(void);
int cpu_do_stat(void);
void cpu_dump_map(void);

/* CPU sets */
struct cpuset {
	size_t len;
	int ncpus;
	uint8_t data[];
};

struct cpuset *cpuset_new(void);
void cpuset_free(struct cpuset *);
int cpuset_set(struct cpuset *, unsigned) __WARN_UNUSED_RESULT;
int cpuset_clear(struct cpuset *, unsigned) __WARN_UNUSED_RESULT;
bool cpuset_is_set(const struct cpuset *, unsigned);
int cpuset_ffs(const struct cpuset *);
uint64_t cpuset_mask64(const struct cpuset *);

static inline bool cpuset_is_empty(const struct cpuset *set)
{
	return set->ncpus == 0;
}

static inline int cpuset_ncpus(const struct cpuset *set)
{
	return set->ncpus;
}

#endif /* CPU_H */
