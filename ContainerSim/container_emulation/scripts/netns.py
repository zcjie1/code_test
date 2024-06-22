
# ip netns list指令从 /run/netns目录中读取netns
# docker创建容器时，并不会在 /run/netns 目录下创建对应的netns
# /var/run 和 /run 挂载的是同一个文件系统
# /var/run/netns目录用以映射docker容器进程的netns

import os

if not (os.path.exists("/var/run/netns/")):
	os.mkdir("/var/run/netns")