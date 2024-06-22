<template>
    <el-dialog title="属性" center :visible.sync='Visible' :before-close='outClose'>
        <div>
        <!-- 隐藏的input用于文件选择 -->
        <input
            type="file"
            ref="fileInput"
            @change="onFileChange"
            style="display: none;"
        />

        <!-- 点击触发文件选择 -->
        <!-- <button @click="openFileSelector">选择文件</button> -->

        <el-input placeholder="文件夹目录" class="input-with-select">
            <el-button slot="append" icon="el-icon-folder" type="success" @click="openFileSelector"></el-button>
        </el-input>
        <!-- 上传按钮 -->
        <el-button v-if="selectedFiles.length > 0" @click="uploadFiles">上传文件</el-button>

        <!-- 显示已选中的文件名 -->
        <ul v-if="selectedFiles.length > 0">
            <li v-for="(file, index) in selectedFiles" :key="index">
            {{ file.name }}
            </li>
        </ul>
        </div>
        <el-button @click="updatefilename_option">更新</el-button>
        <el-select v-model="filename_value" placeholder="请选择" >
          <el-option
            v-for="item in filename_options"
            :key="item.value"
            :label="item.label"
            :value="item.value">
          </el-option>
        </el-select>
        <span slot="footer" class="dialog-footer">
          <el-button @click="Visible = false">取 消</el-button>
          <el-button type="primary" @click="importimagetar">确 定</el-button>
        </span>
    </el-dialog>
</template>

<script>
import axios from 'axios';

export default {
  props:['changefileinput'],
  data() {
    return {
        Visible: true,
        selectedFiles: [],
        filename_options: [],
        filename_value: '',
    };
  },
  methods: {
    updatefilename_option(){
      // window.console.log('dianji')
      axios.get('http://localhost:5000/getimagefileoption')
      .then(response=>{
        // window.console.log(response)
        this.filename_options = response.data.map((filename)=> ({value:filename,label:filename}))
      })
      .catch(error=>{
        window.console.log('fileinput',error)
      })
    },
    importimagetar(){
      axios.post('http://localhost:5000/loadimage',{filename:this.filename_value})
      .then(response=>{
        window.console.log(response)
        this.Visible=false
      })
      .catch(error=>{
        window.console.log('fileinput',error)
      })

    },
    outClose() {
          this.$confirm('确认关闭', '提示框').then(() => {
            this.Visible=false,
            this.changefileinput()
          }).catch(() => {
          })
      },
    openFileSelector() {
      // 触发文件选择对话框
      this.$refs.fileInput.click();
    },
    onFileChange(event) {
      // 当文件选择发生改变时
      this.selectedFiles = Array.from(event.target.files);
    },
    async uploadFiles() {
      if (this.selectedFiles.length === 0) return;

      const formData = new FormData();

      // 将所有已选文件添加到FormData对象
      for (let i = 0; i < this.selectedFiles.length; i++) {
        formData.append('files[]', this.selectedFiles[i]);
        // 如果有其他额外字段需要一起发送，例如用户ID等
        // formData.append('userId', this.userId);
      }

      try {
        // 发送POST请求
        const response = await axios.post('http://localhost:5000/upload', formData, {
          headers: {
            // 如果后端需要的话，设置Content-Type
            'Content-Type': 'multipart/form-data',
          },
        });

        // 处理成功响应
        window.console.log('Upload successful:', response.data);
        this.selectedFiles = [];
        this.Visible=false;
        this.changefileinput(); 
      } catch (error) {
        // 处理错误响应
        window.console.error('Upload failed:', error);
        this.Visible=false;
        this.changefileinput(); 
      }
    },

  },
};
</script>