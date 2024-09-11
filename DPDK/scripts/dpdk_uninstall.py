# 在目录 dpdk-xxx/build 中运行

import json
import os
import subprocess
import shutil

# 指定输出文件
json_file = 'tmp.json'

# 构建命令
command = ['meson', 'introspect', '--installed']

# 运行命令并将输出重定向到文件
with open(json_file, 'w') as file:
    subprocess.run(command, stdout=file, check=True)

def delete_files_from_json(json_file):
    with open(json_file, 'r') as file:
        data = json.load(file)
    
    # 遍历 JSON 对象的每一个键值对
    for _, value in data.items():
        # 检查文件是否存在
        if os.path.islink(value):
            print(f"Deleting symbolic link {value}")
            os.unlink(value)
        elif os.path.isfile(value):
            print(f"Deleting {value}")
            os.remove(value)
        elif os.path.isdir(value):
            print(f"Deleting directory {value}")
            shutil.rmtree(value)
        else:
            print(f"File {value} does not exist, skipping")

# 调用函数删除文件
delete_files_from_json(json_file)

os.remove('tmp.json')