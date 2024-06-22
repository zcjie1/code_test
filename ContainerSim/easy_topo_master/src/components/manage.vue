<template>

    <el-dialog
        title="容器与镜像管理"
        :visible.sync="dialogVisible"
        width="30%"
        :before-close="handleClose">

        <div>
            <span style="display: flex;margin: 0 auto;justify-content: space-evenly"> 
                <el-button @click="getContainers">查看所有容器</el-button>
                <el-button @click="getImages">查看所有镜像</el-button>
            </span>
            
            <el-dialog title='查看所有容器' :visible.sync='Visible' :before-close='outClose' style="width: 1800px;" append-to-body>
                <el-button @click='Visible2=true;getImages();Visible1=false'>新建容器</el-button>
                <!-- <el-button @click='conbine_container'>组合容器</el-button> -->
                <el-button @click='delete_selected_containers' :disabled="deleteButtonDisabled">删除所选容器</el-button>


                <el-table :data='containers' height="300" @selection-change="handleSelectionChange">

                <el-table-column type="selection" width="55"></el-table-column>  

                <el-table-column prop='id' label='CONTAINER ID'>
                    <template slot-scope="scope">
                        {{ limitText(scope.row.id, 10) }}
                    </template>
                </el-table-column>

                <el-table-column prop='name' label='NAMES'></el-table-column>

                <el-table-column prop='image' label='IMAGE'></el-table-column>

                <el-table-column prop='status' label='STATUS'></el-table-column>

                <el-table-column prop="created" label="Created">
                    <template slot-scope="scope">
                        {{ formatCreated(scope.row.created) }}
                    </template>
                </el-table-column>
                
                <el-table-column prop='ports' label='PORTS'>
                     <template slot-scope='scope'>
                        <div v-for="key in Object.keys(scope.row.ports)" :key="key">
                            {{ key }}
                        </div>
                    </template>
                </el-table-column>

                <el-table-column label='操作'>
                    <template slot-scope='scope'>
                        <div style="display: flex; justify-content: flex-start;">
                            <el-button style="margin-right: 1px;" type='warning' size='mini' @click='startContainer(scope.row.id)' :disabled="scope.row.status === 'running'">启动</el-button>
                            <el-button style="margin-left: 1px;" type='warning' size='mini' @click='stopContainer(scope.row.id)' :disabled="scope.row.status !== 'running'">停止</el-button>
                        </div>
                    </template>
                </el-table-column>

                </el-table>
                <span slot='footer'>
                    <el-button @click='Visible=false'>返回</el-button>
                </span>
            </el-dialog>

            <el-dialog title='查看所有镜像' center :visible.sync='Visible1' :before-close='outClose1' append-to-body>
                <div class="container">
                    <el-table :data='images' height="300" :show-checkbox="true" @selection-change="handleSelectionChange2">

                    <el-table-column type="selection" width="55" ></el-table-column>

                    <el-table-column prop='id' label='CONTAINER ID'>
                        <template slot-scope="scope">
                            {{ limitText(scope.row.id.slice(7), 10) }}
                        </template>
                    </el-table-column>

                    <el-table-column prop='tags[0]' label='NAMES'></el-table-column>

                    <el-table-column prop="created" label="Created">
                        <template slot-scope="scope">
                            {{ formatCreated(scope.row.created) }}
                        </template>
                    </el-table-column>

                    <el-table-column prop="size" label="Size">
                        <template slot-scope="scope">
                            {{ formatSize(scope.row.size) }}
                        </template>
                    </el-table-column>

                    <el-table-column label='操作'>
                        <template slot-scope='scope'>
                            <el-button type='warning' size='mini' @click='removeImage(scope.row.id)'>删除镜像</el-button>
                        </template>
                    </el-table-column>

                    </el-table>

                    <!-- 添加镜像按钮 -->
                    <div class="add-image-button-container">
                        <div class="add-image-button" @click="Visible3=true" title="添加镜像">
                            <i class="el-icon-plus"></i>
                        </div>
                    </div>

                </div>
                <span slot='footer'>
                    <el-button type='warning' @click='removeSelectedImages'>删除所选镜像</el-button>
                    <el-button @click='Visible1=false'>返回</el-button>
                </span>
            </el-dialog>

            <!-- 新建容器对话框界面 -->
            <el-dialog title='新建容器'  center :visible.sync='Visible2' :before-close='outClose2' append-to-body>
                <el-form ref="new_container_data" :model="new_container_data" label-width="80px" >
                    <el-form-item label="容器名称">
                        <el-input v-model="new_container_data['new_container_name']" placeholder="请输入容器名称" ></el-input>
                    </el-form-item>
                    <el-form-item label="容器镜像">
                        <el-select v-model="new_container_data['new_container_image']" placeholder="请选择容器镜像">
                            <el-option
                            v-for="item in images"
                            :key="item.index"
                            :label="item.tags[0]"
                            :value="item.tags[0]"
                            >
                            </el-option>
                        </el-select>
                    </el-form-item>
                    <el-form-item>
                        <el-button type="primary" @click="create_container">立即创建</el-button>
                        <el-button @click="outClose2">取消</el-button>
                    </el-form-item>
                </el-form>
            </el-dialog>


            <el-dialog title='添加镜像' center :visible.sync='Visible3' :before-close='outClose3' append-to-body >
                <el-form ref="new_image_data" :model="new_image_data" label-width="120px" label-position="left" >
                <div v-if="isAdvancedMode">
                    <el-form-item label="Registry">
                        <el-select v-model="new_image_data.new_image_reg" placeholder="Docker Hub (anonymous)">
                            <el-option label="Docker Hub (anonymous)" value="Docker Hub (anonymous)"></el-option>
                        </el-select>
                    </el-form-item>
                    <el-form-item label="Image" style="margin-bottom: 0;">
                        <div style="display: flex;">
                            <div style="padding-left: 14px;padding-right: 14px;background-color: #dcdfe6;border-radius: 4px 0 0 4px;border: #ccc solid 1px;border-right: 0;color:55555">docker.io</div>
                            <input class="input" v-model="new_image_data.new_image_name" placeholder="e.g. my-image:my-tag">
                        </div>
                    </el-form-item>
                    <el-form-item label="" style="margin-top: 0;margin-bottom: 0;" v-if="!new_image_data.new_image_name">
                        <i class="el-icon-warning-outline" style="color:red;">Image name is required.</i>
                    </el-form-item>
                </div>
                <div v-if="!isAdvancedMode">
                    <el-form-item label="Image" style="margin-bottom: 0;">
                        <el-input class="input2" v-model="new_image_data.new_image_name" placeholder="e.g. registry/my-image:my-tag"></el-input>
                    </el-form-item>
                    <el-form-item label="" style="margin-top: 0;margin-bottom: 0;" v-if="!new_image_data.new_image_name">
                        <i class="el-icon-warning-outline" style="color:red;">Image name is required.</i>
                    </el-form-item>
                </div>
                </el-form>
                <button class="mode" @click="isAdvancedMode=!isAdvancedMode" >
                    {{ isAdvancedMode ? '高级模式' : '普通模式' }}  
                </button>
                <br>
                <el-button  type="primary" @click="pullImage" v-loading="loading">获取镜像</el-button>
            </el-dialog>

        </div>

        <span slot="footer" class="dialog-footer">
            <el-button type="primary" @click="handleClose">关闭</el-button>
        </span>
    </el-dialog>
    
</template>

<script>
import axios from 'axios';

export default {
    name:'manage',
    props:['showmanage','getimages'],
    data() {
        return {
            Visible:false,
            Visible1:false,
            Visible2:false,
            Visible3:false,
            dialogVisible:this.showmanage,
            isAdvancedMode:true,
            containers: [],
            images:[],
            selectedContainerIndexes: [],//选中的容器索引
            selectedImageIndexes: [],//选中的要删除的镜像索引
            conbine_container_number:0,
            new_container_data:{
                new_container_name:'',
                new_container_image:'',
            },
            deleteButtonDisabled: false,
            new_image_data:{
                new_image_reg:'',
                new_image_name:'',
            },
            loading:false,
            // 其他需要的数据
        };
    },
    methods: {
        // 时间处理函数：从后段获取的时间格式不对
        formatCreated(created) {
            const date = new Date(created);
            return new Intl.DateTimeFormat('en-US', {
                year: 'numeric',
                month: 'numeric',
                day: 'numeric',
                hour: 'numeric',
                minute: 'numeric',
                second: 'numeric',
                timeZoneName: 'short'
            }).format(date);
        },

        // size大小处理为带单位的值
        formatSize(size) {
            const units = ['B', 'KB', 'MB', 'GB', 'TB'];
            let i = 0;
            while (size >= 1024 && i < units.length - 1) {
                size /= 1024;
                i++;
            }
            return size.toFixed(2) + ' ' + units[i];
        },
        // 限制显示内容长度函数
        limitText(text, maxLength) {
            if (text.length > maxLength) {
                return text.substring(0, maxLength) + '...';
            } 
            else {
                return text;
            }
        },
        outClose() {
            this.$confirm('确认关闭', '提示框').then(() => {
                this.Visible=false
            })
        },
        outClose1() {
            this.$confirm('确认关闭', '提示框').then(() => {
                this.Visible1=false
            })
        },
        outClose2() {
            this.$confirm('确认关闭', '提示框').then(() => {
                this.new_container_data = {
                    new_container_name:'',
                    new_container_image:'',
                };
                this.Visible2 = false;
            })
        },
        outClose3() {
            this.$confirm('确认关闭', '提示框').then(() => {
                this.new_image_data = {
                    new_image_name:'',
                    new_image_reg:'',
                };
                this.Visible3=false
            })
        },
        handleClose() {
            this.$confirm('确认关闭？')
            .then(() => {
                this.$emit('changeshowmanage');
            })
            .catch(() => {});
        },
        getContainers() {
            this.Visible=true
            axios.get('http://localhost:5000/containers')
            .then(response => {
                /* eslint-disable */
                // console.log("gt3", response.data)
                /* eslint-disable */
                this.containers = response.data;
            })
            .catch(() => {});
        },
        getImages() {
            this.Visible1=true
            axios.get('http://localhost:5000/images')
            .then(response => {
                /* eslint-disable */
                // console.log("gt3", response.data)
                /* eslint-disable */
                this.images = response.data;
                this.getimages(response.data);
            })
            .catch(() => {});
        },
        startContainer(id) {
            axios.post('http://localhost:5000/containers/start', { id })
            .then(() => {
                // 更新前端状态，例如将容器状态改为 "运行中"
                this.updateContainerStatus(id, 'running');
                // 更新容器信息界面
                this.$nextTick(() => {
                    this.getContainers();
                });
            })
            .catch(error => {
                console.error('启动容器失败：', error);
                // 可以添加错误处理逻辑，例如提示用户启动失败
            });
        },
        stopContainer(id) {
            axios.post('http://localhost:5000/containers/stop', { id })
            .then(() => {
                // 更新前端状态，例如将容器状态改为 "已停止"
                this.updateContainerStatus(id, 'exited');
                // 更新容器信息界面
                this.$nextTick(() => {
                    this.getContainers();
                });
            })
            .catch(error => {
                console.error('停止容器失败：', error);
                // 可以添加错误处理逻辑，例如提示用户停止失败
            });
        },

        // 更新容器状态的方法
        updateContainerStatus(id, status) {
            const containerIndex = this.containers.findIndex(container => container.id === id);
            if (containerIndex !== -1) {
                this.$set(this.containers, containerIndex, { ...this.containers[containerIndex], status });
            }
        },
        //创建容器
        create_container(){
            /* eslint-disable */
            // console.log(this.new_container_data)
            /* eslint-disable */
            if((this.new_container_data.new_container_image==='')||(this.new_container_data.new_container_name===''))
            {
                this.$message.error('容器信息未输入完全，请重新创建');
                return;
            }
            else {
                if(this.containers.some(container=>container.name===this.new_container_data.new_container_name)){
                    this.$message.error('容器名称已存在，请选择一个不同的名称。');
                    return;
                }
                else {
                    axios.post('http://localhost:5000/containers/create',{data:this.new_container_data}).then(()=>{
                        this.new_container_data = {
                            new_container_name:'',
                            new_container_image:'',
                        };
                        this.$nextTick(() => {
                            this.getContainers();
                            this.Visible2 = false;
                        });
                    })
                }
            }
            
        },
        // 组合容器
        conbine_container(){
            if (this.selectedContainerIndexes.length === 0) {
                this.$message.warning('请先选择要组合的容器');
                return;
            }
            this.conbine_container_number++
            let a=[]
            this.selectedContainerIndexes.forEach(index => {
                a.push({image:this.containers[index].name})
            });
            axios.post('http://localhost:5000/containers/conbine',{a,num:this.conbine_container_number},{
                headers: {
                'Content-Type': 'application/json'
                }
            }).then(() => {
                // 更新容器信息界面
                this.$nextTick(() => {
                    this.getContainers();
                });
            })
        },
        //删除所选容器
        delete_selected_containers(){
            if (this.selectedContainerIndexes.length === 0) {
                this.$message.warning('请先选择要删除的容器');
                return;
            }
            let a=[]
            this.selectedContainerIndexes.forEach(index => {
            const container = this.containers[index];
            a.push(container.id); // 调用 removeImage 方法删除镜像
            });
            /* eslint-disable */
            // console.log(a)
            /* eslint-disable */
            axios.post('http://localhost:5000/containers/delete',{a}).then(() => {
                // 更新容器信息界面
                this.$nextTick(() => {
                    this.getContainers();
                });
            })
            this.selectedContainerIndexes = []; // 清空选中的行索引数组
            this.$message.success('成功删除所选容器');
        },
        ///////////////////////////////////////////////////////为完成
        removeImage(id){
            //此处需用axios向后端发送请求来删除镜像，然后再重新获取镜像信息
            // console.log(id)

            axios.post('http://localhost:5000/images/delete',{id}).then(response => {
                // 更新容器信息界面
                console.log(response.data)
                this.$nextTick(() => {
                    this.getImages();
                });
            })
        },
        handleSelectionChange(selection) {
            this.selectedContainerIndexes = selection.map(item => this.containers.indexOf(item));
        },
         // 处理行选中状态变化
        handleSelectionChange2(selection) {
            this.selectedImageIndexes = selection.map(item => this.images.indexOf(item));
            /* eslint-disable */
            // console.log(this.selectedImageIndexes);
            /* eslint-disable */
        },
        // 删除所选镜像
        removeSelectedImages() {
            if (this.selectedImageIndexes.length === 0) {
                this.$message.warning('请先选择要删除的镜像');
                return;
            }

            this.selectedImageIndexes.sort((a, b) => b - a); // 从后往前删除，防止删除后数组长度变化导致索引错位

            this.selectedImageIndexes.forEach(index => {
            const image = this.images[index];
            this.removeImage(image.id); // 调用 removeImage 方法删除镜像
            });

            this.selectedImageIndexes = []; // 清空选中的行索引数组
            this.$message.success('成功删除所选镜像');
        },
        
        // 拉取镜像
        pullImage(){
            if(this.new_image_data.new_image_name)
            {
                this.loading=true;
                axios.post('http://localhost:5000/images/pull',{data:this.new_image_data,staus:this.isAdvancedMode}) .then(response => {
                    this.loading=false;
                    // 处理成功响应
                    console.log(response.data);
                    this.$message.success('成功获取所需镜像');
                    this.Visible3=false;
                    this.new_image_data = {
                        new_image_name:'',
                        new_image_reg:'',
                    };
                    // 更新容器信息界面
                    this.$nextTick(() => {
                        this.getImages();
                    });
                })
                .catch(error => {
                    // 处理错误响应
                    // console.error('gt',error.response.data);
                    this.$message.warning('未获取到所需镜像,请重试!')
                    this.loading=false;
                });
                
            }
            else {
                this.isAdvancedMode ? this.$message.warning('请输入要获取的镜像名称!') : this.$message.warning('请输入要获取的镜像信息!')
                return;
            }
        },
    },
};
</script>

<style scoped>
.container {
  position: relative;
}

.input {
    flex:1;
    border: #dcdfe6 solid 1px;
    border-radius: 0 4px 4px 0;
    padding-left: 14px;
    color:#606266
}
.input2 {
    background-color: #f9fafb;
    border: #dcdfe6 solid 1px;
    border-radius: 4px;
    color:#606266;
}

.input2:focus {
    border:#409EFF solid 1px; 
    outline: none;
}
.input:focus {
    border:#409EFF solid 1px; 
    outline: none;
}
.input2:hover:not(:focus) {
    border:#ccc solid 1px; 
}
.input:hover:not(:focus) {
    border:#ccc solid 1px; 
}
.input::placeholder {
    color: #999;
}
.input2::placeholder {
    color: #abccee;
}
.mode {
    margin-top: 10px;
    margin-bottom: 10px;
    background-color: #fff;
    color: #409EFF;
    border: 0;
}
.mode:hover {
    cursor:pointer;
    color:black;
}
.add-image-button-container {
  position: absolute;
  bottom: 0;
  left: 50%;
  transform: translateX(-50%) translateY(100%);
}

.add-image-button {
  background-color: #409EFF;
  color: #fff;
  font-size: 24px;
  width: 50px;
  height: 25px; /* 控制按钮高度，使其为半圆形状 */
  border-bottom-left-radius: 25px; /* 左下角设置为半圆 */
  border-bottom-right-radius: 25px; /* 右下角设置为半圆 */
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
}

.add-image-button:hover {
  background-color: #66b1ff;
}
</style>
