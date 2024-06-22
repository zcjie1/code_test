# 容器仿真平台(Linux)

## 环境搭建

```shell
& python3 -m venv .venv

& pip3 install -r requirements.txt

& source ./.venv/bin/actiavte
```

## 运行

```shell
& sudo python3 ./scripts/netns.py

& ./.venv/bin/python main.py
```

## TODO

- main.py中的路由划分模块，精简main.py的代码(通过flask的Blueprint实现)

- *.py划分模块，放入src文件夹

- xterm终端启动取消，改成shell命令执行

- 整合mac80211无线功能

- 容器组合(Pod), 使用Docker的contanier网络模式
