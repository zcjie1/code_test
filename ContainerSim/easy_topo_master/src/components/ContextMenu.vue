<template>
  <div 
  class="container" 
  :style="`left:${position.x}px;top:${position.y}px;`"
  oncontextmenu="return false" 
   >
    <ul>
      <li @click="menuClick('link')">
        <i class="el-icon-share"></i>
        <span>连接</span>
      </li>
      <!-- <li @click="menuClick('rename')">
        <i class="el-icon-edit"></i>
        <span>重命名</span>
      </li> -->
      <li @click="menuClick('delete')">
        <i class="el-icon-delete"></i>
        <span>删除节点</span>
      </li>
      <li @click="menuClick('setting')">
        <i class="el-icon-setting"></i>
        <span>属性</span>
      </li>
      <li @click="menuClick('dialog')">
        <i class="el-icon-chat-round"></i>
        <span>打开容器终端</span>
      </li>
      <li @click="menuClick('stats')">
        <i class="el-icon-info"></i>
        <span>容器状态信息</span>
      </li>
      <li @click="menuClick('logs')">
        <i class="el-icon-document"></i>
        <span>容器日志信息</span>
      </li>
      <li @click="menuClick('server1')">
        <i class="el-icon-document-copy"></i>
        <span>访问服务器资源1</span>
      </li>
      <li @click="menuClick('server2')">
        <i class="el-icon-video-camera"></i>
        <span>访问服务器资源2</span>
      </li>
      <li @click="menuClick('makeimagefromcontainer')">
        <i class="el-icon-printer"></i>
        <span>生成容器镜像</span>
      </li>
    </ul>
  </div>
</template>

<script>
export default {
  name: 'Context-Menu',
  props: ['position'],
  methods: {
    menuClick (option) {
      this.$emit('menuClick', option)
    }
  }
  }
</script>

<style scoped>
ul {
  list-style: none;
  margin: 0;
  padding: 0;
}
.container {
  position: absolute;
  background-color: #FFF;
  box-shadow: 1px 2px 6px 1px rgba(0,0,0,0.2);
}
li {
  cursor: pointer;
  padding: 10px 20px;
  color: #409EFF;
}
li i {
  margin-right: 10px;
}
li:hover {
  background-color: #D9ECFF;
}
</style>
