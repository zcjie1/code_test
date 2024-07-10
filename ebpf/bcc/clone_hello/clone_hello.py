#!/bin/python3
from bcc import BPF

bpf_code = '''
int kprobe__sys_clone(void *ctx) {
    bpf_trace_printk("Hello world!\\n");
    return 0;
}
'''

b = BPF(text=bpf_code)
b.trace_print()
