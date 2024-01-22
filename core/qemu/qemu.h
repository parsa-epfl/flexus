#ifndef QEMU_H
#define QEMU_H
// not defined in api.c, defined in external binaries (exec.o, libqemuutil.a)
struct CPUState;
typedef struct CPUState CPUState;

struct QemuOptsList;
struct QemuOpts;
typedef struct QemuOptsList QemuOptsList;
typedef struct QemuOpts QemuOpts;

typedef uint64_t hwaddr;

void cpu_physical_memory_rw(hwaddr addr, uint8_t *buf, int len, int is_write);

CPUState *qemu_get_cpu(int index);
QemuOpts *qemu_opts_find(QemuOptsList *list, const char *id);
QemuOptsList *qemu_find_opts(const char *group);
uint64_t qemu_opt_get_number(QemuOpts *opts, const char *name, uint64_t defval);

extern int smp_cpus;
#endif
