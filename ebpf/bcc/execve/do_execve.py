# do_execve.py
#!/bin/python3
from bcc import BPF
from bcc.utils import printb

# 指定 eBPF 源码文件
b = BPF(src_file="do_execve.c")
# 以内核函数的方式绑定 eBPF 探针
b.attach_kprobe(event="do_execve", fn_name="check_do_execve")

print("%-6s %-16s %-16s" % ("PID", "COMM", "FILE"))

# 自定义回调函数
def print_event(cpu, data, size):
	event = b["events"].event(data)
	printb(b"%-6d %-16s %-16s" % (event.pid, event.comm, event.fname))

# 指定 buffer 名称，为 buffer 的修改添加回调函数
b["events"].open_perf_buffer(print_event)
while 1:
	try:
		# 循环监听
		b.perf_buffer_poll()
	except KeyboardInterrupt:
		exit()
