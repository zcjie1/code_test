// do_execve.c
#include <uapi/linux/limits.h>	// #define NAME_MAX		255
#include <linux/fs.h>			// struct filename;
#include <linux/sched.h>		// #define TASK_COMM_LEN	16


// 定义 Buffer 中的数据结构，用于内核态和用户态的数据交换
struct data_t {
	u32     pid;
	char    comm[TASK_COMM_LEN];
	char    fname[NAME_MAX];
};
BPF_PERF_OUTPUT(events);

// 自定义 hook 函数
int check_do_execve(struct pt_regs *ctx, struct filename *filename,
                                const char __user *const __user *__argv,
                                const char __user *const __user *__envp) {
	struct data_t data = { };
	
	data.pid = bpf_get_current_pid_tgid();
	bpf_get_current_comm(&data.comm, sizeof(data.comm));
	bpf_probe_read_kernel_str(&data.fname, sizeof(data.fname), (void *)filename->name);
	// 提交 buffer 数据
	events.perf_submit(ctx, &data, sizeof(data));
	return 0;
}
