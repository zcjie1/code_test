<template>
  <div class="container" 
  :style="`left:${position.x}px;top:${position.y}px;`" 
  >
    <ul v-if="flag">
      <li>
        <el-button type="primary" @click="Visible = true,flag=false" >
          <i class="el-icon-s-promotion">查看属性</i>
        </el-button>
      </li>
      <li>
        <el-button type="primary" @click="addoptions">
          <i class="el-icon-circle-plus">挂载文件</i>
        </el-button>
      </li>
      <li>
        <el-button type="primary" @click="wirelessmode" v-show="nodedata[0].topo_type=='station'">
          <i class="el-icon-circle-plus">无线模式</i>
        </el-button>
      </li>
    </ul>
    
    
    <!--查看属性-->
    <el-dialog title="属性" center :visible.sync='Visible' :before-close='outClose' >
      <el-table :data='nodedata'>
        <el-table-column prop='topo_type' label='类型'></el-table-column>
        <el-table-column prop='topo_name' label='名称'></el-table-column>
        <el-table-column prop='topo_id' label='id'></el-table-column>
        <el-table-column label='操作'>
          <template>
            <el-button type='text' size='mini' @click='Visible1=true'>修改属性</el-button>
            <el-button type='text' size='mini' @click='Portvisible=true'>查看端口</el-button>
          </template>
        </el-table-column>
      </el-table>

      <el-dialog title='修改属性' center :visible.sync='Visible1' :before-close='outClose1' append-to-body>
        <el-form :model='nodedata[0]' label-width='80px'>
            <el-form-item label='类型'>
              <el-input disabled v-model='nodedata[0].topo_type'></el-input>
            </el-form-item>
            <el-form-item label='名称'>
              <el-input disabled v-model='nodedata[0].topo_name'></el-input>
            </el-form-item>
            <el-form-item label='id'>
              <el-input disabled v-model='nodedata[0].topo_id'></el-input>
            </el-form-item>
        </el-form>
        <span slot='footer'>
            <!-- <el-button @click='changenode()'>确定</el-button> -->
            <el-button @click='Visible1=false'>返回</el-button>
        </span>
      </el-dialog>

      <el-dialog title="端口信息" center :visible.sync='Portvisible' :before-close='outClose2' append-to-body>
        <el-table :data='portdata'>
          <el-table-column prop='topo_duankou' label='端口名称'></el-table-column>
          <el-table-column prop='topo_type' label='类型'></el-table-column>
          <el-table-column prop='topo_name' label='名称'></el-table-column>
          <el-table-column prop='topo_id' label='id'></el-table-column>
          <el-table-column prop='topo_ip' label='ip'></el-table-column>
          <el-table-column prop='topo_ipv6' label='ipv6'></el-table-column>
          <el-table-column label='操作'>
            <template slot-scope='scope'>
              <el-button type='warning' size='mini' @click='handleEdit(scope.row)'>修改信息</el-button>
            </template>
          </el-table-column>
        </el-table>
        
        <el-dialog title='修改信息' center :visible.sync='Portvisible2' :before-close='outClose3' append-to-body>
          <el-form :model='formData' label-width='80px'>
            <el-form-item label='端口名称'>
              <el-input disabled v-model='formData.topo_duankou'></el-input>
            </el-form-item>
            <el-form-item label='类型'>
              <el-input disabled v-model='formData.topo_type'></el-input>
            </el-form-item>
            <el-form-item label='名称'>
              <el-input v-model='formData.topo_name'></el-input>
            </el-form-item>
            <el-form-item label='id'>
              <el-input disabled v-model='formData.topo_id'></el-input>
            </el-form-item>
            <el-form-item label='ip'>
              <el-input v-model='formData.topo_ip'></el-input>
            </el-form-item>
            <el-form-item label='ipv6'>
              <el-input v-model='formData.topo_ipv6'></el-input>
            </el-form-item>
          </el-form>
          <span slot='footer'>
            <el-button @click='Portvisible2=false'>确定</el-button>
            <el-button @click='Portvisible2=false'>返回</el-button>
          </span>
        </el-dialog>
        
        <span slot='footer'>
          <el-button @click='changelink()'>确定</el-button>
          <el-button @click='Portvisible=false'>返回</el-button>
        </span>
      </el-dialog>

      <span slot='footer'>
        <el-button @click='Visible=false'>返回</el-button>
      </span>
    </el-dialog>


  </div>
</template>

<script>
import { MessageBox } from 'element-ui'
export default {

    props: ['position','topoNodes','indexOfMenu','addfile'],

    data() {
      return {
        flag:true,
        Visible: false,
        Visible1: false,
        Portvisible:false,
        Portvisible2:false,
        formData: { topo_type: '', topo_id: '', topo_name: '',topo_ip: '',topo_ipv6:'' },
      }
    },

    methods:{
      wirelessmode(){
        alert('choose wirelessmode')
      },
      addoptions(){
        this.flag=false;
        MessageBox.prompt('请输入“挂载文件的当前路径(绝对路径)：挂载到容器内部的目录“', '挂载文件', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          inputPattern: /^(?:(?:[\w/.-]+):(?:[\w/.-]+))$/,
          inputErrorMessage: '输入格式不正确'
        }).then(({ value }) => {
          this.addfile(this.indexOfMenu,value)
          this.$message({
            type: 'success',
            message: `${this.topoNodes[this.indexOfMenu].name}节点挂载文件${value}配置成功`
          });
        }).catch(() => {
          this.$message({
            type: 'info',
            message: '取消输入'
          });       
        });
      },
      outClose() {
          this.$confirm('确认关闭', '提示框').then(() => {
            this.Visible=false
          }).catch(() => {
          })
      },
      outClose1() {
          this.$confirm('确认关闭', '提示框').then(() => {
            this.Visible1=false
          }).catch(() => {
          })
      },
      outClose2() {
          this.$confirm('确认关闭', '提示框').then(() => {
            this.Portvisible=false
          }).catch(() => {
          })
      },
      outClose3() {
          this.$confirm('确认关闭', '提示框').then(() => {
            this.Portvisible2=false
          }).catch(() => {
          })
      },
      handleEdit(row) {
          this.formData = row
          this.Portvisible2 = true
      },
      changenode() {
        // 子组件修改父组件的数据
        /* eslint-disable */
        // console.log('gt',this.gridData)
        /* eslint-disable */
        this.$emit('changenode', this.nodedata)
        this.Visible1=false
      },
      changelink() {
        // 子组件修改父组件的数据
        /* eslint-disable */
        // console.log('gt',this.gridData)
        /* eslint-disable */
        this.$emit('changelink', this.portdata)
        this.Portvisible=false
      },
    },

    computed:{
      nodedata(){
        return [{
          topo_type: this.topoNodes[this.indexOfMenu].type,
          topo_id: this.topoNodes[this.indexOfMenu].id,
          topo_name: this.topoNodes[this.indexOfMenu].name,
        }]
      },

      portdata(){
        let a=[]
        let b=this.topoNodes[this.indexOfMenu].ports
        for (const key in b) {
          a.push({
            topo_duankou: key,
            topo_type: this.topoNodes[b[key].id].type,
            topo_id: b[key].id,
            topo_name: this.topoNodes[b[key].id].name,
            topo_ip: b[key].ip,
            topo_ipv6: b[key].ipv6,
          })
        }
        return a
      },

    }
}

</script>

<style scoped>
.container {
  position: absolute;
  background-color: #FFF;
  box-shadow: 1px 2px 6px 1px rgba(0,0,0,0.2);
}
ul {
  list-style: none;
  margin: 0;
  padding: 0;
}
li {
  cursor: pointer;
  color: #409EFF;
}
li i {
  margin-right: 10px;
}
li:hover {
  background-color: #D9ECFF;
}
</style>