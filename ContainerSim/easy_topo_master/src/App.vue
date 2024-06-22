<template>
  <div id="app">
    <Topo/>
  </div>
</template>

<script>
import Topo from './components/Topo'

export default {
  name: 'app',
  components: {
    Topo
  }
}
</script>

<style>
* {
  font-family:sans-serif;
  font-weight: 700;
  font-style: italic;
}
body {
  margin: 0;
}
.el-menu--collapse .el-menu .el-submenu, .el-menu--popup{
  min-width: 90px!important;
}
.el-submenu .el-menu-item {
  min-width: 90px!important;
}
</style>
