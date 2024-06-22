import pty
import os
import subprocess
import time


def run_in_namespace(name):
    # 创建一个伪终端，并执行命令进入指定的命名空间
    master, slave = pty.openpty()
    cmd = "ip"
    args = ["netns", "exec", name, "bash", "-l"]

    # 使用subprocess.Popen执行命令
    popen = subprocess.Popen([cmd] + args, stdin=slave, stdout=slave, stderr=slave, close_fds=False, shell=True)
    pid = popen.pid
    return pid

# 测试
namespace_name = "Host1"
terminal_pid = run_in_namespace(namespace_name)
print(f"Terminal in namespace {namespace_name} is running with PID: {terminal_pid}")
while True:
    time.sleep(1)