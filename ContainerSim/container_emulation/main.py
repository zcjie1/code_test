from flask import Flask, request, jsonify, make_response
from flask_cors import CORS
from net import Net
from docker.errors import ImageNotFound
import docker
import json
import os
import pwd
import subprocess

app = Flask(__name__)
CORS(app)

# docker客户端，与docker daemon通信
client = docker.from_env()
net = Net()

@app.route('/savetopo', methods=['POST'])
def save_topo():
    try:
        data = request.get_json()
        filename = data.get('filename')
        if not filename:
            return jsonify({'error': 'Invalid filename'}), 400

        # 保存目录
        save_dir = '/home/gongtao/Desktop/topodata/'
        filepath = os.path.join(save_dir, f'{filename}.txt')

        # 字符串写入文件
        data_str = json.dumps(data, indent=4)
        with open(filepath, 'w') as f:
            f.write(data_str)

        return '', 204

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/create', methods=['POST'])
def create_topo():
    nodes = request.get_json()['nodes']
    node_number = len(nodes)
    for node in range(node_number):
        node_type = nodes[node]['type']
        node_id = nodes[node]['number']
        configpath = nodes[node]['addfile']
        net.add_node(node_type=node_type, node_id=node_id,configpath=configpath)
    links = request.get_json()['links']
    link_number = len(links)
    for link in range(link_number):
        node1_name = str(links[link]['startNodeName'])
        node2_name = str(links[link]['endNodeName'])
        net.add_link(node1_name, node2_name)
    print("共创建{}个容器".format(node_number))

    try:
        for index in range(node_number):
            node_name = str(nodes[index]['name'])
            node = net.get(node_name)
            ports = nodes[index]['ports']
            for key in ports:
                port_name = str(key).replace(" ", "")
                ipstr = ports[key]['ip']
                ipstr2 = ports[key]['ipv6']
                net.config_Node(node, port_name, ip=ipstr,ipv6=ipstr2)
    except Exception as e:
        print('ip初始化失败',e)


    net.run_docker_inspect_bridge()
    net.prometheus_config()
    yaml_file_relative_path = "../yamlData.yml"
    # 获取当前脚本所在的绝对路径
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # 合成yaml文件的绝对路径
    yaml_file_absolute_path = os.path.join(script_dir, yaml_file_relative_path)

    containers = client.containers.list(all=True)
    
    # 查找名为prometheus的容器
    prometheus_container = any(c for c in containers if c.name == 'prometheus')

    if prometheus_container:
        # 如果找到Prometheus容器，则先停止并删除它
        print("发现已存在的Prometheus容器，将停止并删除...")
        prometheus_container.stop()
        prometheus_container.remove()

    # 现在你可以将这个绝对路径用于Docker命令中
    command = [
        'docker', 'run', '-d',
        '--name', 'prometheus',
        '-p', '9090:9090',
        '-v', f'{yaml_file_absolute_path}:/etc/prometheus/prometheus.yml',
        'prom/prometheus'
    ]
    # 使用subprocess.Popen来执行命令
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    # 等待命令执行完成并获取输出和错误信息
    stdout, stderr = process.communicate()

    # 检查命令是否成功执行
    if process.returncode == 0:
        container_id = stdout.decode().strip()  # 获取容器ID（假设返回的是容器ID）
        print(f"Prometheus容器已启动，容器ID：{container_id}")
    else:
        print(f"启动Prometheus容器时出错：{stderr.decode().strip()}")

    return '共创建{}个容器'.format(node_number)

@app.route('/delete', methods=['POST'])
def delete_topo():
    net.del_topo()
    return '删除拓扑成功'

@app.route('/setport', methods=['POST'])
def node_setportIP():
    datas = request.get_json()['node']
    portname = request.get_json()['portname']
    ip_change = request.get_json()['ip_change']
    ipv6_change = request.get_json()['ipv6_change']
    node_name = str(datas['name'])
    ports = datas['ports']
    node = net.get(node_name)
    port_name = str(portname).replace(" ", "")
    ipstr=None
    ipstr2=None
    if ip_change:
        ipstr = ports[portname]['ip']
        # print(ipstr)
    if ipv6_change:
        ipstr2 = ports[portname]['ipv6']
        # print(ipstr2)
    result=net.config_Node(node, port_name, ip=ipstr,ipv6=ipstr2)
    # ports = datas['ports']
    # for key in ports:
    #     port_name = str(key).replace(" ", "")
    #     ipstr = ports[key]['ip']
    #     ipstr2 = ports[key]['ipv6']
    #     net.config_Node(node, port_name, ip=ipstr,ipv6=ipstr2)
    return result

@app.route('/xterm', methods=['POST'])
def open_node_xterm():
    node = request.get_json()['nodes']
    node_name = str(node['name'])
    net.open_node_xterm(node_name)
    return '已打开节点 {}的xterm'.format(node_name)

@app.route('/containers', methods=['GET'])
def get_containers():
    containers = client.containers.list(all=True)
    container_data = []
    for container in containers:
        container_info = {
            'id': container.id,
            'name': container.name,
            'status': container.status,
            'ports': container.attrs['NetworkSettings']['Ports'],
            'created': container.attrs['Created'],
            'image': container.image.tags[0] if container.image.tags else None,  # 获取容器关联的镜像名称
        }
        container_data.append(container_info)
    return jsonify(container_data)

@app.route('/containers/stats', methods=['POST'])
def containers_stats():
    name = request.json.get('name')
    try:
        # 构建 docker stats 命令
        docker_stats_command = ['docker', 'stats', '--no-stream', '--format', '{{json .}}', name]

        # 执行命令并获取输出
        result = subprocess.run(docker_stats_command, capture_output=True, text=True, check=True)

        # 解析 JSON 输出
        stats_info = json.loads(result.stdout)

        # 提取所需的信息
        memory_usage = stats_info['MemUsage']
        cpu_usage = stats_info['CPUPerc']
        network_usage = stats_info['NetIO']
        io_usage = stats_info['BlockIO']

        # 返回信息给调用者
        return jsonify({
            'MemoryUsage': memory_usage,
            'CPUUsage': cpu_usage,
            'NetworkUsage': network_usage,
            'IOUsage': io_usage
        })

    except Exception as e:
        print(f"Error: {e}")
        # 处理错误，如果有需要的话
        return {'error': str(e)}
        
@app.route('/containers/logs', methods=['POST'])
def containers_logs():
    name = request.json.get('name')
    try:
        # 构建 docker logs 命令
        docker_logs_command = ['docker', 'logs', name]

        # 执行命令并获取输出
        result = subprocess.run(docker_logs_command, capture_output=True, text=True, check=True)

        # 直接返回输出
        return {'logs': result.stdout}

    except Exception as e:
        print(f"Error: {e}")
        # 处理错误，如果有需要的话
        return {'error': str(e)}
	
@app.route('/images', methods=['GET'])
def get_images():
    images = client.images.list()
    image_data = []
    for image in images:
        image_info = {
            'id': image.id,
            'tags': image.tags,
            'created': image.attrs['Created'],
            'size': image.attrs['Size'],
        }
        image_data.append(image_info)
    return jsonify(image_data)

@app.route('/containers/start', methods=['POST'])
def start_container():
    container_id = request.json.get('id')
    container = client.containers.get(container_id)
    container.start()
    return jsonify({'status': 'success'})

@app.route('/containers/stop', methods=['POST'])
def stop_container():
    container_id = request.json.get('id')
    container = client.containers.get(container_id)
    container.stop()
    return jsonify({'status': 'success'})

@app.route('/containers/conbine',methods=['POST'])
def conbine_container():
    conbine_container_number = request.json.get('num')
    images = request.json.get('a')
    command = "docker run -d --name combine_container{} ubuntu".format(conbine_container_number) 
    # unique_images = []  
    # for image in images:  
    #     if image['image'] not in unique_images:  
    #         unique_images.append(image['image'])  
    # for unique_image in unique_images:
    #     command += ' --link ' + unique_image
    for image in images:
        command += ' --link ' + image['image']
    subprocess.run(command,shell=True)

    # conbine_container_number = request.json.get()['num']
    # containers = request.json.get('a')
    # dockerfile_content = '''
    # FROM ubuntu
    # '''
    # with open('Dockerfile', 'w') as f:
    #     f.write(dockerfile_content)
    # for container in containers:
    #     dockerfile_content = f'''
    #     ADD {container['container_id']} {container['container_name']}
    #     '''
    #     with open('Dockerfile', 'a') as f:
    #         f.write(dockerfile_content)
   

    # subprocess.run(['docker', 'build', '-t', 'combine_container{}'.format(conbine_container_number), '.']) 
    return jsonify({'status': 'success'})

@app.route('/containers/create',methods=['POST'])
def create_container():
    data = request.json.get('data')
    command = "docker run -d --name {} {}".format(data['new_container_name'],data['new_container_image'])
    subprocess.run(command,shell=True)
    return jsonify({'status': 'success'})

@app.route('/containers/delete',methods=['POST'])
def delete_container():
    container_id = request.json.get('a')
    for id in container_id:
        container = client.containers.get(id)
        inspect_result = subprocess.run(['docker', 'inspect', '--format', '{{.State.Status}}', id], capture_output=True, text=True, check=True)
        if inspect_result.stdout.strip() == 'running':
            container.stop()
        container.remove()
    
    return jsonify({'status': 'success'})

@app.route('/images/pull',methods=['POST'])
def pull_image():
    try:
        status = request.json.get('status')
        image_data = request.json.get('data')
        if status:
            if image_data.new_image_reg == '' or image_data.new_image_reg == 'Docker Hub (anonymous)':
                client.images.pull(f"{image_data['new_image_name']}")
            else :
                client.images.pull(f"{image_data['new_image_reg']}/{image_data['new_image_name']}")
            return jsonify({"message": "镜像拉取成功"})
        else :
            client.images.pull(f"{image_data['new_image_name']}")
        return jsonify({"message": "镜像拉取成功"})
    except ImageNotFound as e:
        return jsonify({"error": f"镜像未找到: {str(e)}"}), 404
    except Exception as e:
        return jsonify({"error": str(e)}), 500
        
@app.route('/images/delete',methods=['POST'])
def delete_image():
    id = request.json.get('id').replace('sha256:', '')
    client.images.remove(id)
    return jsonify({"message": "镜像删除成功"})

@app.route('/server/nginx/text',methods=['POST'])
def server_nginx():
    name = request.json.get('name')
    dest_ip = request.json.get('ip')
    filepath = request.json.get('filepath') or ''
    # print(name)
    # print(dest_ip)
    # print(filepath)
    node = net.get(name)
    output=node.get_instance().exec_run(['sh','-c','curl -s http://{}/{}'.format(dest_ip,filepath)])
    print(output)
    html_content = output.output.decode('utf-8')  # 解码字节串为字符串

    response = make_response(html_content)  # 创建一个Flask Response对象
    response.headers['Content-Type'] = 'text/html; charset=utf-8'  # 设置Content-Type头以表示返回的是HTML内容

    return response

@app.route('/server/nginx/video',methods=['POST'])
def server_nginx_video():
    name = request.json.get('name')
    dest_ip = request.json.get('ip')
    filepath = request.json.get('filepath') or ''
    # print(name)
    # print(dest_ip)
    # print(filepath)
    node = net.get(name)
    try:
        output=node.get_instance().exec_run(['sh','-c','curl http://{}/{} -o {}'.format(dest_ip,filepath,filepath)])
        print(output)
    except Exception as e:
        print(e)
        return '获取失败'

    return '获取成功'

@app.route('/makeimagefromcontainer',methods=['POST'])
def makeimagefromcontainer():
    name = request.json.get('name')
    imagename = request.json.get('imagename')
    try:
        # 尝试获取容器实例
        container_instance = net.get(name).get_instance()

        # 容器存在，则执行 docker commit 命令
        command = ['docker', 'commit', f'{name}', f'{imagename}']
        # print(command)

        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        if process.returncode == 0:
            print(f'{imagename}镜像创建成功！{stdout.decode()}')
        else:
            print(f"镜像创建时出错：{stderr.decode()}")
            return jsonify({"status": "error", "message": f"镜像创建时出错：{stderr.decode()}"}), 500

    except:
        print(f'{name}容器不存在')
        return jsonify({"status": "error", "message": f"{name}容器不存在"}), 404
    
    return 'gt'

@app.route('/topodatafilename',methods=['GET'])
def get_file_names_in_path():
    directory = "/home/gongtao/Desktop/topodata"
    return [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]

@app.route('/gettopodata', methods=['POST'])
def get_topo_data():
    data = request.get_json()
    filename = data.get('filename')
    # print(filename)
    filepath = f'/home/gongtao/Desktop/topodata/{filename}'
    with open(filepath, 'r') as f:
        data_str = f.read()
        
    # 如果您之前是以JSON格式保存的，这里需要反序列化为字典
    data_dict = json.loads(data_str)
    
    return jsonify(data_dict), 200

@app.route('/linkreset',methods=['POST'])
def linkreset():
    link = request.json.get('link')
    # print(link['startNodeName'],link['endNodeName'])
    try:
        node1=net.get(link['startNodeName'])
        node2=net.get(link['endNodeName'])
        node1.get_instance().exec_run(["sh","-c","tc filter del dev {}".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc filter del dev {}".format('to'+link['startNodeName'])])
        node1.get_instance().exec_run(["sh","-c","tc class del dev {} classid 1:1".format('to'+link['endNodeName'])])
        node1.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc class del dev {} classid 1:1".format('to'+link['startNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format('to'+link['startNodeName'])])
        node1.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format('to'+link['startNodeName'])])
    except:
        return '重置失败'
    return '重置成功'

@app.route('/linkupdate',methods=['POST'])
def linkupdate():
    link = request.json.get('link')
    # print(link)
    prop = ""
    if link["timedelay"] != '0':
        prop += f"delay {link['timedelay']}ms "
        if link["timedelay_fluct"] != '0':
            prop += f"{link['timedelay_fluct']}ms "
            if link["timedelay_fluct_percent"] != '0':
                prop += f"{link['timedelay_fluct_percent']}% "
            if link["timedelay_fluct_distribution"] in ["normal", "uniform", "pareto", "paretonormal"]:
                prop += f"distribution {link['timedelay_fluct_distribution']} "
            else:
                print('请输入正确的时延波动分布类型')
    if link["corrupt"] != '0':
        prop += f"corrupt {link['corrupt']}% "
    if link["duplicate"] != '0':
        prop += f"duplicate {link['duplicate']}% "
    if link["packetloss"] != '0':
        prop += f"loss {link['packetloss']}% "
        if link["packetloss_success"] != '0':
            prop += f"{link['packetloss_success']}% "
    # print(prop)
    
    try:
        node1=net.get(link['startNodeName'])
        node2=net.get(link['endNodeName'])
        node1.get_instance().exec_run(["sh","-c","tc filter del dev {}".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc filter del dev {}".format('to'+link['startNodeName'])])
        node1.get_instance().exec_run(["sh","-c","tc class del dev {} classid 1:1".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc class del dev {} classid 1:1".format('to'+link['startNodeName'])])
        if link['BandWidth']!='':
            node1.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate {}".format('to'+link['endNodeName'],link['BandWidth']+'mbit')])
            node2.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate {}".format('to'+link['startNodeName'],link['BandWidth']+'mbit')])
            # print(1)
        else:
            node1.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format('to'+link['endNodeName'])])
            node2.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format('to'+link['startNodeName'])])
            # print(2)
        if prop!='':
            node1.get_instance().exec_run(["sh","-c","tc qdisc add dev {} parent 1:1 handle 20: netem {}".format('to'+link['endNodeName'],prop)])
            node2.get_instance().exec_run(["sh","-c","tc qdisc add dev {} parent 1:1 handle 20: netem {}".format('to'+link['startNodeName'],prop)])
            # print(3)
        node1.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format('to'+link['endNodeName'])])
        node2.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format('to'+link['startNodeName'])])
        print('{}节点与{}节点之间的链路属性更新成功'.format(link['startNodeName'],link['endNodeName']))
    except:
        print('{}节点与{}节点之间的链路属性更新失败'.format(link['startNodeName'],link['endNodeName']))
        return '更新失败'
        
    return '更新成功'

@app.route('/upload', methods=['POST'])
def handle_file_upload():
    # 获取上传的文件
    file = request.files['files[]']
    # 桌面路径通常位于用户主目录下的“Desktop”目录
    # desktop_dir = os.path.join("Desktop", "t123")  # 替换为你的文件夹名
    # desktop_dir = os.environ['HOME'] + '/gongtao/Desktop/t123'

    # 获取普通用户的用户名
    target_user = 'gongtao'  # 替换为实际的非root用户名

    # 获取该用户的家目录
    user_info = pwd.getpwnam(target_user)
    user_home_dir = user_info.pw_dir
    desktop_dir = os.path.join(user_home_dir, 'Desktop', 't123')
    
    print(desktop_dir)
    # 确保目标文件夹存在
    if not os.path.exists(desktop_dir):
        os.makedirs(desktop_dir)
        print(520)
    

    # 检查是否有文件被上传
    if file:      
        # 保存文件到指定目录
        file.save(os.path.join(desktop_dir, file.filename))
        print('文件已保存')

        return {'status': 'success', 'message': f'File {file.filename} has been uploaded.'}, 200
    else:
        return {'status': 'error', 'message': 'No file was uploaded.'}, 400
    
@app.route('/loadimage', methods=['POST'])
def loadimage():
    imagefilename = request.json.get('filename')
    try:
        subprocess.run('docker load -i /home/gongtao/Desktop/t123/{}'.format(imagefilename),shell=True,stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return 'success'
    except Exception as e:
        print(e)
        return 'error'
    
@app.route('/getimagefileoption', methods=['GET'])
def getimagefileoption():
    directory = "/home/gongtao/Desktop/t123"
    return [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]

if __name__ == '__main__':
    app.run(port=5000,debug=True)
