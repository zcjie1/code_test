<template>
  <el-container style="height: 100vh;" v-loading="clearloading" element-loading-text="努力清除中"
    element-loading-spinner="el-icon-loading"
    element-loading-background="rgba(0, 0, 0, 0.8)">
    <!-- 页眉 -->
    <el-header class="header-container">
      <div class="background-link" @click="currentImageindex = (currentImageindex+1) % 15" :class="{ ['bg-image-' + currentImageindex]: true }">
        <h1 class="title" style="text-align: center;">容器化网络仿真平台</h1>
      </div>
    </el-header>

    <!-- 上方导航栏 -->
    <el-menu 
      ref="menu"
      class="el-menu-demo" 
      mode="horizontal"  
      background-color="#545c64"
      text-color="#fff"
      active-text-color="#ffd04b"
      style="height:30px;overflow:hidden;"
    >
      <el-menu-item index="1" class="centered" style="height:'30px';font-size: 10px;" @click="changefileinput">
        导入本地镜像
      </el-menu-item>
      <!-- <el-submenu index="2" style="height: 28px;">
        <template slot="title" >
          <div class="submenu-title-container" style="font-size: 10px;">
            功能二
          </div>
        </template>
        <el-menu-item index="2-1" style="font-size: 10px;">选项1</el-menu-item>
        <el-menu-item index="2-2" style="font-size: 10px;">选项2</el-menu-item>
        <el-menu-item index="2-3" style="font-size: 10px;">选项3</el-menu-item>
        <el-submenu index="2-4" style="width: 88px;">
          <template slot="title">
            <div style="font-size: 10px;">
              选项4
            </div>
          </template>
          <el-menu-item index="2-4-1" style="font-size: 10px;">选项1</el-menu-item>
          <el-menu-item index="2-4-2" style="font-size: 10px;">选项2</el-menu-item>
          <el-menu-item index="2-4-3" style="font-size: 10px;">选项3</el-menu-item>
        </el-submenu>
      </el-submenu> -->
      <el-menu-item index="2" class="centered" style="height: '30px';font-size: 10px;">
        <a href="http://localhost:9000/#!/2/docker/containers" target="_blank" style="text-decoration: none;">portainer</a>
      </el-menu-item>
      <el-menu-item index="3" class="centered" style="height: '30px';font-size: 10px;">
        <a href="http://localhost:9090" target="_blank" style="text-decoration: none;">prometheus</a>
      </el-menu-item>
      <el-menu-item index="4" class="centered" style="height: '30px';font-size: 10px;">
        <a href="http://localhost:3000/d/rYdddlPWk/node-exporter-full?orgId=1&refresh=1m&from=now-5m&to=now" target="_blank" style="text-decoration: none;">grafana</a>
      </el-menu-item>
      <el-menu-item index="5" class="centered" style="height: '30px';font-size: 10px;" @click="importtopo">导入topo文件</el-menu-item>
      <el-menu-item index="6" class="centered" style="height: '30px';font-size: 10px;" @click="fasttopo=true">快速布置topo</el-menu-item>
    </el-menu>

    <el-container style="height: 100%;">
      <!-- 侧边节点库 -->
      <el-aside class="aside" style="width:200px;position: relative;" :class="asideinit ? (asidetoggle ? 'asidetogg2' : 'asidetogg1') : ''">
        <!-- 侧栏菜单 -->
        <el-menu :unique-opened="true">
          <!-- 子菜单 -->
          <el-submenu v-for="(type, index) in typeList" :key="type" :index="String(index)">
            <template slot="title">
              <i class="el-icon-s-grid"></i>
              <span>{{type}}</span>
            </template>
            <el-menu-item-group>
              <el-menu-item v-for="item in libraryList[type]" :key="item.index" class="library-item">
                <div draggable="true" @dragstart="dragToBoardStart">
                  <img :src="item.pic" :alt="type" draggable="false">
                  <span>{{item.name}}</span>
                </div>
              </el-menu-item>
            </el-menu-item-group>
          </el-submenu>
        </el-menu>

        <div style="position: absolute;display: flex;justify-content: space-around;width: 100%;">
          <el-button @click="typelist('add')">添加</el-button>
          <el-button @click="typelist('del')">删除</el-button>
        </div>
        
        <div style="position: absolute;bottom: 10px;display: flex;width: 100%;flex-wrap: wrap;text-align: center;justify-content: space-evenly;align-items: center">
          <el-switch v-model="linestate" active-text="实线" inactive-text="虚线"></el-switch>
          <el-color-picker v-model="linecolor" size="mini" ></el-color-picker>
          <el-slider v-model="linewidth" style="width: 80%;" :max=5 :min=1 :marks="linemarks"></el-slider>
        </div>
      </el-aside>
      
      <div style="display: flex;flex-direction: column;justify-content: space-between;">
        <div class="asidetoggle" @click="asidetoggle=!asidetoggle,asideinit=true">
          <i :class="{[asidetoggle ? 'el-icon-caret-left':'el-icon-caret-right']:true}"></i>
        </div>
        <!-- <div class="asidetoggle">
          <i :class="{[asidetoggle ? 'el-icon-caret-left':'el-icon-caret-right']:true}"></i>
          <i class="el-icon-caret-left"></i>
        </div> -->
      </div>

      <!-- 拓扑画板 -->
      <el-main class="board-container">
        <!-- 画板 -->
        <svg 
        id="myid"
        @mousewheel="mousewheel"
        @mousedown="mousedown" 
        @mousemove="mousemove"
        class="board" 
        ondragover="return false" 
        @drop="dropToBoard" 
        oncontextmenu="return false"
        @click.left.stop.prevent="closeContextMenu">
          <!-- 已连接的线 -->
          <line
          v-for="(item, index) in lines"
          :key="index+'l'"
          :x1="item.x1 - item.offsetX1 + 6000*item.radio1" :y1="item.y1 - item.offsetY1 + 4000*item.radio1"
          :x2="item.x2 - item.offsetX2 + 6000*item.radio2" :y2="item.y2 - item.offsetY2 + 4000*item.radio2"
          :style="{stroke:item.linecolor , strokeWidth:item.linewidth+'px'}"
          :stroke-dasharray="item.linestate ? '' : '10,10' "
          @click.right.stop.prevent="linehello(index)"
          @dblclick.stop.prevent="linehello(index)"
          v-popover="'popover_' + index"
          />
          
          <!-- 正在连接的线 -->
          <line
          :x1="connecting.x1" :y1="connecting.y1"
          :x2="connecting.x2" :y2="connecting.y2"
          :style="{stroke:linecolor , strokeWidth:linewidth+'px'}"
          :stroke-dasharray="linestate ? '' : '10,10' "/>
          <!-- topo图上的节点 -->
          <g 
          v-for="(item, index) in topoNodes"
          :key="index"
          @mousedown.left.stop.prevent="moveAndLink(index, $event)"
          @click.right.stop.prevent="nodeMenu(index, $event)">
            <image :xlink:href="item.pic" :width="50*item.radio" :height="50*item.radio" :x="item.x - item.offsetX + 6000*item.radio" :y="item.y - item.offsetY + 4000*item.radio"></image>
            <text :x="item.x + 25*item.radio - item.offsetX + 6000*item.radio" :y="item.y + 66*item.radio - item.offsetY + 4000*item.radio" style="text-anchor: middle; user-select: none;">{{item.name}}</text>
          </g>
        </svg>

        <!-- topo中的节点的右键菜单 -->
        <Context-Menu :position="position" v-if="showMenu" @menuClick="clickMenuItem" style="z-index: 99;"/>
        <!-- 右键属性选项 -->
        <zujian :addfile="addfile" :position="position" :topoNodes="topoNodes" :topoLinks="topoLinks" v-if="showzujian" @changelink='dealChange2' @changenode='dealChange' :indexOfMenu="indexOfMenu" />
        <!-- 容器管理 -->
        <manage v-if="showmanage" @changeshowmanage="changeshowmanage" :showmanage="showmanage" :getimages="getimages"/>
        <!-- 导入本地镜像 -->
        <fileinput v-if="isshow_fileinput" :changefileinput="changefileinput"/>
        <!-- containerstatus -->
        <el-dialog
          :visible.sync="dialogVisible"
          :modal="false"
          :before-close="handleClose" style="overflow: hidden;" width="100%" class="container_stats">
          <!-- <span slot="title" style="height: 10px;">Container statistics</span> -->
          <div style="display: flex;flex-wrap: wrap;width:470px;height: 330px;justify-content: space-around;align-content: space-around;background-color: #e9e9eb;">
            <div class="chart-container">
              <div>Memory Usage</div>
              <canvas ref="Memory"></canvas>
            </div>
            <div class="chart-container">
              <div>CPU Usage</div>
              <canvas ref="CPU Usage"></canvas>
            </div>
            <div class="chart-container">
              <div>Network Usage</div>
              <canvas ref="RX"></canvas>
            </div>
            <div class="chart-container">
              <div>I/O Usage</div>
              <canvas ref="Read"></canvas>
            </div>
          </div>
        </el-dialog>
        <!-- 导入本地拓扑文件 -->
        <el-dialog
          title="请选择导入的拓扑文件"
          :visible.sync="centerDialogVisible"
          width="30%"
          center>
          <el-select v-model="filename_value" placeholder="请选择">
            <el-option
              v-for="item in filename_options"
              :key="item.value"
              :label="item.label"
              :value="item.value">
            </el-option>
          </el-select>
          <span slot="footer" class="dialog-footer">
            <el-button @click="centerDialogVisible = false">取 消</el-button>
            <el-button type="primary" @click="importtopo2">确 定</el-button>
          </span>
        </el-dialog>
        <!-- 修改typelist(添加删除按钮操控) -->
        <el-dialog
          title="修改typelist"
          :visible.sync="centerDialogVisible_typelist"
          width="30%"
          center>
          <div v-if="changetypelist_add">
            <el-select v-model="changetypelist_value" placeholder="请选择要添加的类型">
              <el-option
                v-for="(type, index) in typeList"
                :key="index"
                :label="type"
                :value="type">
              </el-option>
            </el-select>
          </div>
          <div v-else>
             <el-cascader
              v-model="changetypelist_value"
              :options="typelist_options"
              :props="{ multiple: true }"
              filterable></el-cascader>
          </div>
          
          <span slot="footer" class="dialog-footer">
            <el-button @click="centerDialogVisible_typelist = false">取 消</el-button>
            <el-button type="primary" @click="changetypelist">确 定</el-button>
          </span>
        </el-dialog>
        <!-- 快速布置topo -->
        <el-dialog
          title="快速布置topo"
          :visible.sync="fasttopo"
          width="50%"
          center>

          <el-tabs v-model="activeName" type="border-card">
            <el-tab-pane label="简单拓扑" name="first">
              <el-form :model="simpletopoform" ref="simpletopoform" label-width="100px" class="demo-ruleForm">
                <el-alert
                  title="此结构为一个交换机与多个主机相连的简单拓扑结构"
                  type="info"
                  show-icon
                  :closable="false">
                </el-alert>
                <el-divider></el-divider>
                <el-form-item
                  label="主机个数"
                  prop="hostnum"
                  :rules="[
                    { required: true, message: '主机个数不能为空',trigger:'blur'},
                    { type: 'number', message: '主机个数必须为数字值'}
                  ]"
                >
                  <el-input v-model.number="simpletopoform.hostnum" autocomplete="off"></el-input>
                </el-form-item>
                <el-form-item>
                  <el-button type="primary" @click="submitForm_fasttopo('simpletopoform')">提交</el-button>
                  <el-button @click="resetForm('simpletopoform')">重置</el-button>
                </el-form-item>
              </el-form>
            </el-tab-pane>
            <el-tab-pane label="线性拓扑" name="second">
              <el-form :model="lineartopoform" ref="lineartopoform" label-width="100px" class="demo-ruleForm">
                <el-alert
                  title="此结构为每个交换机与一个主机相连，各交换机按需线性连接在一起的线性拓扑结构"
                  type="info"
                  show-icon
                  :closable="false">
                </el-alert>
                <el-divider></el-divider>
                <el-form-item
                  label="主机个数"
                  prop="hostnum"
                  :rules="[
                    { required: true, message: '主机个数不能为空'},
                    { type: 'number', message: '主机个数必须为数字值'}
                  ]"
                >
                  <el-input v-model.number="lineartopoform.hostnum" autocomplete="off"></el-input>
                </el-form-item>
                <el-form-item>
                  <el-button type="primary" @click="submitForm_fasttopo('lineartopoform')">提交</el-button>
                  <el-button @click="resetForm('lineartopoform')">重置</el-button>
                </el-form-item>
              </el-form>
            </el-tab-pane>
            <el-tab-pane label="树状拓扑" name="third">
              <el-form :model="treetopoform" ref="treetopoform" label-width="100px" class="demo-ruleForm">
                <el-alert
                  title="此结构为深度为depth，每个节点有fanout个子结点的网状拓扑结构（最底层节点为host，其余节点为switch）"
                  type="info"
                  show-icon
                  :closable="false">
                </el-alert>
                <el-divider></el-divider>
                <el-form-item
                  label="深度"
                  prop="depth"
                  :rules="[
                    { required: true, message: '深度不能为空'},
                    { type: 'number', message: '深度必须为数字值'}
                  ]"
                >
                  <el-input v-model.number="treetopoform.depth" autocomplete="off"></el-input>
                </el-form-item>
                <el-form-item
                  label="扇出"
                  prop="fanout"
                  :rules="[
                    { required: true, message: '扇出不能为空'},
                    { type: 'number', message: '扇出必须为数字值'}
                  ]"
                >
                  <el-input v-model.number="treetopoform.fanout" autocomplete="off"></el-input>
                </el-form-item>
                <el-form-item>
                  <el-button type="primary" @click="submitForm_fasttopo('treetopoform')">提交</el-button>
                  <el-button @click="resetForm('treetopoform')">重置</el-button>
                </el-form-item>
              </el-form>
            </el-tab-pane>
            <el-tab-pane label="自定义" name="fourth">
              <el-form ref="customizedtopoform" :model="customizedtopoform" label-width="80px" style="max-height: 400px; overflow-y: auto;" label-position=top>
                <el-alert
                  title="自定义节点数量"
                  type="info"
                  show-icon
                  :closable="false">
                </el-alert>
                <el-divider></el-divider>

                <el-form-item
                  v-for="(items, category) in libraryList"
                  :key="category"
                  :label="category"
                  :prop="category"
                >
                  <span v-for="(part,index) in customizedtopoform[category]" :key="String(index)">
                    <el-col :span="10">
                      <el-select v-model="part.name" placeholder="请选择节点种类名称">
                        <el-option
                          v-for="item in items"
                          :key="item.name"
                          :label="item.name"
                          :value="item.name">
                        </el-option>
                      </el-select>
                    </el-col>
                    <el-col :span="10">
                      <el-input v-model.number="part.num" autocomplete="off" placeholder="请输入节点数量"></el-input>
                    </el-col>
                    <el-col :span="2">
                      <el-button @click.prevent="customizedtopoform[category].splice(index, 1)">删除</el-button>
                    </el-col>
                  </span>
                  <el-button @click.prevent="customizedtopoform[category].push({name:'',num:0})">添加</el-button>
                </el-form-item>

                <el-form-item>
                  <el-button type="primary" @click="submitForm_fasttopo('customizedtopoform')">提交</el-button>
                  <el-button @click="resetForm('customizedtopoform')">重置</el-button>
                </el-form-item>
              </el-form>
            </el-tab-pane>
          </el-tabs>
          
          
          <span slot="footer" class="dialog-footer">
            <el-button @click="fasttopo = false">退出</el-button>
          </span>
        </el-dialog>

        <el-dialog title="所有链路属性" center :visible.sync='linksVisible' :before-close='outClose' >
          <el-table :data='topoLinks' stripe max-height="300">
            <el-table-column prop="startNodeName" label="起点"></el-table-column>
            <el-table-column prop="endNodeName" label="终点"></el-table-column>
            <el-table-column prop="BandWidth" label="带宽(Mbps)" ></el-table-column>
            <el-table-column prop="timedelay" label="时延(ms)"></el-table-column>
            <el-table-column label='操作'>
              <template slot-scope="scope">
                <el-button type='text' size='mini' @click='showlinkdetail(scope.row)'>查看详细属性</el-button>
              </template>
            </el-table-column>
          </el-table>

          <el-dialog title='链路属性' center :visible.sync='linkVisible' :before-close='outClose1' append-to-body>
            <el-form :model='currentlinkdata' ref="currentlinkdata" label-width='150px' label-position="right" status-icon style="max-height: 300px; overflow-y: auto;">
                <el-form-item label='起点' prop="startNodeName">
                  <el-input disabled v-model='currentlinkdata.startNodeName'></el-input>
                </el-form-item>
                <el-form-item label='终点' prop="endNodeName">
                  <el-input disabled v-model='currentlinkdata.endNodeName'></el-input>
                </el-form-item>
                <el-form-item label='带宽(Mbps)' prop="BandWidth">
                  <el-input v-model.number='currentlinkdata.BandWidth'></el-input>
                </el-form-item>
                <el-form-item label='时延(ms)' prop="timedelay">
                  <el-input v-model.number='currentlinkdata.timedelay'></el-input>
                </el-form-item>
                <el-form-item label='时延波动(ms/%)' prop="timedelay_fluct">
                  <el-input v-model.number='currentlinkdata.timedelay_fluct'></el-input>
                </el-form-item>
                <el-form-item label='时延波动相关系数(%)' prop="timedelay_fluct_percent">
                  <el-input v-model.number='currentlinkdata.timedelay_fluct_percent'></el-input>
                </el-form-item>
                <el-form-item label='时延分布类型' prop="timedelay_fluct_distribution">
                  <el-select v-model="currentlinkdata.timedelay_fluct_distribution" placeholder="请选择活动区域">
                    <el-option label="normal" value="normal"></el-option>
                    <el-option label="uniform" value="uniform"></el-option>
                    <el-option label="pareto" value="pareto"></el-option>
                    <el-option label="paretonormal" value="paretonormal"></el-option>
                  </el-select>
                </el-form-item>
                <el-form-item label='丢包率(%)' prop="packetloss">
                  <el-input v-model.number='currentlinkdata.packetloss'></el-input>
                </el-form-item>
                <el-form-item label='丢包成功率(%)' prop="packetloss_success">
                  <el-input v-model.number='currentlinkdata.packetloss_success'></el-input>
                </el-form-item>
                <el-form-item label='重复率(%)' prop="duplicate">
                  <el-input v-model.number='currentlinkdata.duplicate'></el-input>
                </el-form-item>
                <el-form-item label='损坏率(%)' prop="corrupt">
                  <el-input v-model.number='currentlinkdata.corrupt'></el-input>
                </el-form-item>
            </el-form>
            <span slot="footer">
              <el-button type="primary" @click="submitForm_update(currentlinkdata)">更新</el-button>
              <el-button type="danger" @click="submitForm_reset(currentlinkdata)">重置</el-button>
              <el-button @click="resetForm('currentlinkdata')">复原</el-button>
              <el-button @click='linkVisible=false'>返回</el-button>
            </span>
          </el-dialog>
          <span slot='footer'>
            <el-button @click='linksVisible=false'>返回</el-button>
          </span>
        </el-dialog>

        <el-dialog title="所有节点属性" center :visible.sync='nodesVisible' :before-close='outClose2' >
          <el-table :data='topoNodes' stripe border max-height="300" :default-sort = "{prop: 'id'}">
            <el-table-column sortable prop='id' label='id'></el-table-column>
            <el-table-column sortable 
              :filters="[{text: 'host', value: 'host'}, {text: 'router', value: 'router'}, {text: 'switch', value: 'switch'}, {text: 'server', value: 'server'}, {text: 'controller', value: 'controller'}, {text: 'firewall', value: 'firewall'}]"
              :filter-method="filterHandler" prop='cls' label='种类'></el-table-column>
            <el-table-column sortable prop='type' label='类型'></el-table-column>
            <el-table-column prop='name' label='名称'></el-table-column>
            <el-table-column label='操作'>
              <template slot-scope="scope">
                <el-button type='text' size='mini' @click='shownodedetail(scope.row)'>查看详细属性</el-button>
              </template>
            </el-table-column>
          </el-table>

          <el-dialog title='节点属性' center :visible.sync='nodeVisible' :before-close='outClose3' append-to-body>
            <el-form :model='currentnodedata' label-width='150px' label-position="left" status-icon style="max-height: 300px; overflow-y: auto;">
                <el-form-item label='id' prop="id">
                  <el-input disabled v-model='currentnodedata.id'></el-input>
                </el-form-item> 
                <el-form-item label='种类' prop="cls">
                  <el-input disabled v-model='currentnodedata.cls'></el-input>
                </el-form-item>
                <el-form-item label='类型' prop="type">
                  <el-input disabled v-model='currentnodedata.type'></el-input>
                </el-form-item>
                <el-form-item label='名称' prop="name">
                  <el-input disabled v-model='currentnodedata.name'></el-input>
                </el-form-item>
                <el-form-item label='类型内编号' prop="number">
                  <el-input disabled v-model='currentnodedata.number'></el-input>
                </el-form-item>
                <el-form-item label='横坐标' prop="x">
                  <el-input disabled v-model='currentnodedata.x'></el-input>
                </el-form-item>
                <el-form-item label='纵坐标' prop="y">
                  <el-input disabled v-model='currentnodedata.y'></el-input>
                </el-form-item>
                <el-form-item label="端口">
                  <el-select v-model="currentnodedata_portselected" placeholder="请选择端口">
                    <el-option  v-for="(value, key) in currentnodedata.ports"
                      :key="key"
                      :label="key"
                      :value="key"></el-option>
                  </el-select>
                </el-form-item>
            </el-form>
            <span slot="footer">
              <el-button @click='nodeVisible=false'>返回</el-button>
            </span>
          </el-dialog>
          <span slot='footer'>
            <el-button @click='nodesVisible=false'>返回</el-button>
          </span>
        </el-dialog>

        <!-- 功能按键 -->
        <div class="button-container">
          <el-button @click="openmanage">打开容器镜像管理</el-button>
          <el-button @click="saveTopo">保存拓扑</el-button>
          <el-button @click="createTopo" v-loading="createtopoloading" >生成拓扑</el-button>
          <el-button @click="clearTopo" type="danger">清空拓扑</el-button>
        </div>

        
        <el-popover v-for="(item, index) in lines" 
          :key="index"
          :ref="'popover_' + index"
          placement="bottom"
          title="链路属性(0表示未设置)"
          width="300"
          trigger="manual"
          v-model="item.popvisible">
          <el-table :data="linedata(index)" stripe style="width: 100%" border empty-text="暂无数据" @cell-dblclick="linkdatachange">
            <!-- <el-table-column prop="startNodeName" label="起点"></el-table-column>
            <el-table-column prop="endNodeName" label="终点"></el-table-column> -->
            <el-table-column prop="BandWidth" label="带宽(Mbps)" ></el-table-column>
            <el-table-column prop="timedelay" label="时延(ms)"></el-table-column>
            <el-table-column prop="timedelay_fluct" label="时延波动(ms/%)"></el-table-column>
            <el-table-column prop="timedelay_fluct_percent" label="时延波动相关系数(%)"></el-table-column>
            <el-table-column prop="timedelay_fluct_distribution" label="时延分布类型(normal/uniform/pareto/paretonormal)"></el-table-column>
            <el-table-column prop="packetloss" label="丢包率(%)"></el-table-column>
            <el-table-column prop="packetloss_success" label="丢包成功率(%)"></el-table-column>
            <el-table-column prop="duplicate" label="重复率(%)"></el-table-column>
            <el-table-column prop="corrupt" label="损坏率(%)"></el-table-column>
            <!-- <el-table-column label="操作" fixed="right">
              <template slot-scope="scope">
                <el-button
                  size="mini"
                  @click="handleEdit(scope.$index, scope.row)">修改</el-button>
              </template>
            </el-table-column> -->
          </el-table>
          <div style="text-align: center;display:flex;justify-content: space-around;">
            <el-button size="mini" type="danger" @click="linkreset(index)">重置链路</el-button>
            <el-badge :is-dot="islinkdatachange">
              <el-button size="mini" type="primary" @click="linkupdate(index)">更新链路</el-button>
            </el-badge>
            <el-button size="mini" @click="closePopover(index)">关闭</el-button>
          </div>
        </el-popover>
        
        <!-- <div class="svg-left">
          <div class="svg-left-title">
            title
            <i class="el-icon-close"></i>
          </div>
          <div class="svg-left-main">
           
          </div>
        </div> -->

        <!-- 右侧侧边栏 -->
        <div style="position: fixed;right:0px;top:20%;text-align: center;z-index:99;">
          <el-menu class="el-menu-vertical-demo" @open="handleOpen" @close="handleClose" :collapse="isCollapse" active-text-color="#909399">
            <el-menu-item index="1" @click="linksVisible=true">
              <i class="el-icon-share"></i>
              <el-badge :value="topoLinks.length" type="primary" :max="9"></el-badge>
              <span slot="title" >查看所有链路信息</span>
            </el-menu-item>
            <el-menu-item index="2" @click="nodesVisible=true">
              <i class="el-icon-menu"></i>
              <el-badge :value="topoNodes.length" type="primary" :max="9"></el-badge>
              <span slot="title" >查看所有节点信息</span>
            </el-menu-item>
            <el-menu-item index="3" @click="svgdrag=!svgdrag"  :style="svgdrag ? {'color': '#409EFF'} : {}">
              <i class="el-icon-switch-button"></i>
              <span slot="title">切换移动模式</span>
            </el-menu-item>
            <!-- <el-menu-item index="4">
              <i class="el-icon-setting"></i>
              <span slot="title">功能四</span>
            </el-menu-item> -->
            <el-menu-item index="5"  @click="isCollapse=!isCollapse">
              <i class="el-icon-d-arrow-left" v-if="isCollapse" ></i>
              <i class="el-icon-d-arrow-right" v-else></i>
            </el-menu-item>
          </el-menu>
        </div>
      </el-main>
    </el-container>

    <el-footer height="40px" style="border-top: 1px solid #CCC;">
      <el-link href="https://github.com/gtdbdxgt" :underline="false" target="_blank">查看项目地址<i class="el-icon-view el-icon--right address" ></i> </el-link>
    </el-footer>
  </el-container>
</template>

<script>
import ContextMenu from './ContextMenu'
import manage from './manage'
import zujian from './zujian'
import fileinput from './fileinput'
import { MessageBox , Notification} from 'element-ui'
import nodeData from '../data/nodeData'
import axios from 'axios'
import { Chart,Filler,LineElement,LineController,BarElement, BarController, LinearScale, PointElement, TimeScale, CategoryScale, Tooltip, Legend } from 'chart.js';


export default {
  name: 'Topo',
  components: {
    ContextMenu,zujian,manage,fileinput
  },
  data () {
    return {
      customizedtopoform:{},
      treetopoform:{
        depth:'',
        fanout:''
      },
      lineartopoform:{
        hostnum:''
      },
      simpletopoform:{
        hostnum:''
      },
      activeName:'first',
      fasttopo:false,
      ipnum:1,
      clearloading:false,
      svgdrag:false,
      asideinit:false,
      asidetoggle:true,
      mousePos:{x:0,y:0},
      offsetX:0,
      offsetY:0,
      radio:1,
      dragging:false,
      currentImageindex:2,
      currentnodedata_portselected:'',
      libraryList: {}, // 左侧节点库的节点数据
      typeList: [], // 节点分类
      topoNodes: [], // topo图中的节点
      topoNodes_copy: [], // topo图中的节点信息副本
      topoLinks: [], // topo图中的连线
      topoLinks_copy: [], // topo图中的连线信息副本
      Nodes_type_number: [],// 每种类型节点的数量
      node_id: 0,
      connecting:{ // 显示正在连接的线条
        x1: 0,
        y1: 0,
        x2: 0,
        y2: 0
      },
      move: true, // 操作模式，默认为移动。可切换为连接模式
      position: {x: 0, y: 0}, // 右键菜单的位置
      showMenu: false, // 控制右键菜单的显示与否
      showzujian: false, // 控制组件的显示与否
      showmanage: false,
      dialogVisible: false, // 控制对话框的显示与否
      indexOfMenu: null, // 表示在哪个节点上点击了右键菜单
      indexOfMenu_copy:null,
      isCollapse: true,
      isshow_fileinput:false,
      islinkdatachange:false,
      centerDialogVisible: false,
      centerDialogVisible_typelist: false,
      linksVisible:false,
      linkVisible:false,
      nodesVisible:false,
      nodeVisible:false,
      currentlinkdata:{},
      currentnodedata:{},
      images:[],//容器管理界面获取到的容器镜像数据
      cpuChartData: {
        labels: [],
        datasets: [
          {
            label: 'CPU Usage',
            data: [],
             fill: true,
              backgroundColor: 'rgba(151,187,205,0.4)',
              borderColor: 'rgba(151,187,205,0.6)',
              pointBackgroundColor: 'rgba(151,187,205,1)',
              pointBorderColor: 'rgba(151,187,205,1)',
              pointRadius: 2,
              borderWidth: 2,
          },
        ],
      },
      ioChartData: {
        labels: [],
        datasets: [
          {
            label: 'Read',
            data: [],
            fill: true,
            backgroundColor: 'rgba(255,180,174,0.4)',
            borderColor: 'rgba(255,180,174,0.6)',
            tension: 0.1,
            pointBackgroundColor: 'rgba(255,180,174,1)',
            pointBorderColor: 'rgba(255,180,174,1)',
            pointRadius: 2,
            borderWidth: 2,
          },
          {
            label: 'Write',
            data: [],
             fill: true,
              backgroundColor: 'rgba(151,187,205,0.4)',
              borderColor: 'rgb(75, 192, 192)',
              tension: 0.1,
              pointBackgroundColor: 'rgba(151,187,205,1)',
              pointBorderColor: 'rgba(151,187,205,1)',
              pointRadius: 2,
              borderWidth: 2,
          },
        ],
      },
      memoryChartData: {
        labels: [],
        datasets: [
          {
            label: 'Memory',
            data: [/* Extract memory usage data from your API response */],
            fill:true,
            backgroundColor: 'rgba(255,180,174,0.4)',
            borderColor: 'rgba(255,180,174,0.6)',
            tension: 0.1,
            pointBackgroundColor: 'rgba(255,180,174,1)',
            pointBorderColor: 'rgba(255,180,174,1)',
            pointRadius: 2,
            borderWidth: 2,
          },
          {
            label: 'Cache',
            data: [/* Extract memory usage data from your API response */],
             fill: true,
              backgroundColor: 'rgba(151,187,205,0.4)',
              borderColor: 'rgb(75, 192, 192)',
              tension: 0.1,
              pointBackgroundColor: 'rgba(151,187,205,1)',
              pointBorderColor: 'rgba(151,187,205,1)',
              pointRadius: 2,
              borderWidth: 2,
          },
        ],
      },
      networkChartData: {
        labels: [],
        datasets: [
          {
            label: 'RX',
            data: [/* Extract network usage data from your API response */],
            //  fill: true,
            backgroundColor: 'rgba(255,180,174,0.4)',
            borderColor: 'rgba(255,180,174,0.6)',
            tension: 0.1,
            pointBackgroundColor: 'rgba(255,180,174,1)',
            pointBorderColor: 'rgba(255,180,174,1)',
            pointRadius: 2,
            borderWidth: 2,
          },
          {
            label: 'TX',
            data: [/* Extract network usage data from your API response */],
            //  fill: true,
              backgroundColor: 'rgba(151,187,205,0.4)',
              borderColor: 'rgb(75, 192, 192)',
              tension: 0.1,
              pointBackgroundColor: 'rgba(151,187,205,1)',
              pointBorderColor: 'rgba(151,187,205,1)',
              pointRadius: 2,
              borderWidth: 2,
          },
        ],
      },
      filename_options: [],
      filename_value: '',
      typelist_options:[],
      changetypelist_add:'',
      changetypelist_value:'',
      linestate:true,
      linecolor:'#000000',
      linewidth:3,
      linemarks:{
        1:'1px',
        2:'2px',
        3:'3px',
        4:'4px',
        5:'5px',
      },
      createtopoloading:false,
    }
  },
  methods: {
    addnode(name0,type,x,y,pic){
      const name = this.Nodes_type_number.find(obj => obj.name === name0)
      if (name) {
        name.number += 1
      }
      else{
        this.Nodes_type_number.push({name: name0, number: 1,type:type})
      }
      const newname = this.Nodes_type_number.find(obj => obj.name === name0)
      let node = {
        id: this.node_id, //每个节点的唯一id
        number: newname.number, // 每个节点的类型id
        pic: pic, // 图片
        name: name0 + newname.number, // 默认显示名称，可修改
        type: name0 , 
        x: x, // 横坐标
        y: y, // 纵坐标
        offsetX:this.offsetX,
        offsetY:this.offsetY,
        radio:this.radio,
        ports: {},
        cls: type,
        addfile:'',
      }
      if(node.type=='station'){
        // window.console.log(123)
        node.ports['wlan0']={
          ip: null,
          ipv6:null,
          id: node.id,
        }
      }
      this.node_id += 1
      this.topoNodes.push(node)
      return node.id
    },
    addlink(startNodeId,endNodeId){
      this.topoLinks.push({
        startNodeId: this.topoNodes[startNodeId].id,
        startNodeName: this.topoNodes[startNodeId].name,
        endNodeId: this.topoNodes[endNodeId].id,
        endNodeName: this.topoNodes[endNodeId].name,
        startInterface: 'fa0/1',
        endInterface: 'fa0/1',
        linestate:this.linestate,
        linecolor:this.linecolor,
        linewidth:this.linewidth,
        BandWidth:'50',
        timedelay:'0',
        timedelay_fluct:'0',
        timedelay_fluct_percent:'0',
        timedelay_fluct_distribution:'normal',
        corrupt:'0',
        duplicate:'0',
        packetloss:'0',
        packetloss_success:'0',
      })
      if(this.topoNodes_copy.some(node => node.name === this.topoNodes[startNodeId].name)){
        this.topoNodes[startNodeId].ports[`to ${this.topoNodes[endNodeId].name}`] = {
          ip: null,
          ipv6:null,
          id: this.topoNodes[endNodeId].id
        }
        window.console.log('创建了容器的节点',this.topoNodes[startNodeId])
      }else{
        this.topoNodes[startNodeId].ports[`to ${this.topoNodes[endNodeId].name}`] = {
          ip: '192.168.1.' + this.ipnum + '/24',
          ipv6:null,
          id: this.topoNodes[endNodeId].id
        }
        this.ipnum++;
        window.console.log('没创建容器的节点',this.topoNodes[startNodeId])
      }

      if(this.topoNodes_copy.some(node => node.name === this.topoNodes[endNodeId].name)){
        this.topoNodes[endNodeId].ports[`to ${this.topoNodes[startNodeId].name}`] = {
          ip: null,
          ipv6:null,
          id: this.topoNodes[startNodeId].id
        }
        window.console.log('创建了容器的节点',this.topoNodes[endNodeId])
      }else{
        this.topoNodes[endNodeId].ports[`to ${this.topoNodes[startNodeId].name}`] = {
          ip: '192.168.1.' + this.ipnum + '/24',
          ipv6:null,
          id: this.topoNodes[startNodeId].id
        }
        this.ipnum++;
        window.console.log('没创建容器的节点',this.topoNodes[endNodeId])
      }
    },
    submitForm_fasttopo(formName) {
      this.$refs[formName].validate((valid) => {
        if (valid) {
          this.$message({
            type:'success',
            message:'成功'
          })
          if(formName=='simpletopoform'){
            // window.console.log(this.simpletopoform)
            const pic=this.libraryList['switch'].find(obj=>obj.name=='ovsswitch').pic
            const pic1=this.libraryList['host'].find(obj=>obj.name=='ubuntu').pic

            let x=100
            let y=100
            if(this.simpletopoform.hostnum>=16){
              x=800
            }else{
              x=100*(1+this.simpletopoform.hostnum)/2
            }
            const ovsid=this.addnode('ovsswitch','switch',x,y,pic)

            x=100
            y=200
            for(let i=1;i<=this.simpletopoform.hostnum;i++){
              const nodeid=this.addnode('ubuntu','host',x,y,pic1)
              this.addlink(ovsid,nodeid)
              if(x>=1500){
                x=0
                y+=100
              }
              x+=80
            }
          }else if(formName == 'lineartopoform'){
            // window.console.log(this.lineartopoform)
            const pic=this.libraryList['switch'].find(obj=>obj.name=='ovsswitch').pic
            const pic1=this.libraryList['host'].find(obj=>obj.name=='ubuntu').pic
            let x=100
            let y=100
            let ovsid_pre = 0
            for(let i=1;i<=this.lineartopoform.hostnum;i++){
              const ovsid=this.addnode('ovsswitch','switch',x,y,pic)
              const hostid=this.addnode('ubuntu','host',x,y+100,pic1)
              this.addlink(ovsid,hostid)
              if(i!=1){
                this.addlink(ovsid,ovsid_pre)
              }
              x+=80
              ovsid_pre=ovsid
            }
          }else if(formName == 'treetopoform'){
            // window.console.log(this.treetopoform)
            let depth = this.treetopoform.depth
            let fanout = this.treetopoform.fanout
            let y=80
            const pic=this.libraryList['switch'].find(obj=>obj.name=='ovsswitch').pic
            const pic1=this.libraryList['host'].find(obj=>obj.name=='ubuntu').pic
            let nodeid_pre=[]
            for(let i=depth-1;i>=0;i--){
              let nodecount=Math.pow(fanout,i)
              y=80*(i+1)
              let startx=800-(nodecount-1)*80*(depth-i)/2
              if(i==depth-1){
                for(let j=1;j<=nodecount;j++){
                  nodeid_pre.push(this.addnode('ubuntu','host',startx+80*(j-1),y,pic1))
                }
              }else {
                let nodeid=[]
                for(let j=1;j<=nodecount;j++){
                  let node_id=this.addnode('ovsswitch','switch',startx+80*(j-1)*(depth-i),y,pic)
                  nodeid.push(node_id)
                  for(let m=1;m<=fanout;m++){
                    this.addlink(node_id,nodeid_pre[(j-1)*fanout+m-1])
                  }
                }
                nodeid_pre=nodeid.slice()
              }
            }
          }else {
            // window.console.log(this.customizedtopoform)
            let x=100
            let y=100
            for (const key in this.customizedtopoform) {
              this.customizedtopoform[key].forEach(item=>{
                if(item.name!=''){
                  const pic=this.libraryList[key].find(obj=>obj.name==item.name).pic
                  for(let i=1;i<=item.num;i++){
                    this.addnode(item.name,key,x,y,pic)
                    if(x>=1500){
                      x=0
                      y+=100
                    }
                    x+=80
                  }
                  
                }
              })
            }
          }
          this.fasttopo=false
        } else {
          this.$message({
            type:'error',
            message:'error submit!!'
          })
        }
      });
      
    },
    mousemove(e) {
      // window.console.log(1)
      if(this.dragging&&this.svgdrag) {
        const e2 = window.event || e
        e2.preventDefault()
        let nx = e.clientX
        let ny = e.clientY
        let offsetX = nx - this.mousePos.x
        let offsetY = ny - this.mousePos.y
        this.offsetX = offsetX
        this.offsetY = offsetY
        let svg = document.getElementById("myid")
        svg.style.transform = `translate(${offsetX-6000*this.radio}px, ${offsetY-4000*this.radio}px) scale(${this.radio})`
      }
    },
    mousedown(e) {
      // window.console.log(2)
      const event = window.event || e
      event.preventDefault()
      let nx = event.clientX
      let ny = event.clientY
      this.mousePos.x = nx - this.offsetX
      this.mousePos.y = ny - this.offsetY
      this.dragging=true
      window.addEventListener("mouseup",this.mouseupFunction)
    },
    mouseupFunction(){
      this.dragging=false
      window.removeEventListener("mouseup",this.mouseupFunction)
    },
    mousewheel(e) {
      if(this.svgdrag){
        const event = window.event || e
        event.preventDefault()
        let newRadio = this.radio
        if(e.deltaY < 0) {
          newRadio += 0.2
        } else {
          newRadio -= 0.1
        }	
        if(newRadio <= 0.1) {
            return
        }
        let svg = document.getElementById('myid')
        svg.style.transform = `translate(${this.offsetX-6000*newRadio}px, ${this.offsetY-4000*newRadio}px) scale(${newRadio})`
        this.radio = newRadio
      }
      
    },
    // zoomElement(){
    //   var elmnt = document.getElementById('myid');
    //   // window.console.log(elmnt)
      

    //   if (elmnt.addEventListener) {
    //       // IE9, Chrome, Safari, Opera
    //       elmnt.addEventListener('mousewheel', MouseWheelHandler, false);
    //       // Firefox
    //       elmnt.addEventListener('DOMMouseScroll', MouseWheelHandler, false);
    //   }

    //   let i = 1;
    //   function MouseWheelHandler(e) {
    //       // cross-browser wheel delta
    //       const event = window.event || e; // old IE support
    //       event.preventDefault();
    //       const delta = Math.max(-1, Math.min(1, (event.wheelDelta || -event.detail)));
    //       if (delta === 1) {
    //           i += 0.2;
    //           elmnt.style.transform = 'scale(' + i + ')';
    //       } else if (delta === -1) {
    //           if (i < 0.3) {
    //               i = 0.2;
    //           } else {
    //               i -= 0.2;
    //           }
    //           elmnt.style.transform = 'scale(' + i + ')';
    //       }
    //       return false;
    //   }
    // },
    filterHandler(value, row, column) {
      const property = column['property'];
      return row[property] === value;
    },
    submitForm_update(value){
      // window.console.log(value)
      const index = this.topoLinks.findIndex(link => 
        link.startNodeName === value.startNodeName &&
        link.endNodeName === value.endNodeName
      );
      this.linkupdate(index);
    },
    submitForm_reset(value){
      // window.console.log(value)

      const index = this.topoLinks.findIndex(link => 
        link.startNodeName === value.startNodeName &&
        link.endNodeName === value.endNodeName
      );
      this.topoLinks[index]={...value};
      this.linkreset(index);
    },
    resetForm(formName) {
      // window.console.log('gt')
      this.$refs[formName].resetFields();
    },
    showlinkdetail(value){
      this.currentlinkdata = {...value};
      this.linkVisible = true;
    },
    shownodedetail(value){
      this.currentnodedata = {...value};
      this.nodeVisible = true;
    },
    outClose() {
      this.$confirm('确认关闭', '提示框').then(() => {
        this.linksVisible=false
      }).catch(() => {
      })
    },
    outClose1() {
      this.$confirm('确认关闭', '提示框').then(() => {
        this.linkVisible=false
      }).catch(() => {
      })
    },
    outClose2() {
      this.$confirm('确认关闭', '提示框').then(() => {
        this.nodesVisible=false
      }).catch(() => {
      })
    },
    outClose3() {
      this.$confirm('确认关闭', '提示框').then(() => {
        this.nodeVisible=false
      }).catch(() => {
      })
    },
    linkreset(index){
      axios.post('http://localhost:5000/linkreset',{link:this.topoLinks[index]})
      .then((response)=>{
        this.topoLinks[index].BandWidth='50',
        this.topoLinks[index].timedelay='0',
        this.topoLinks[index].timedelay_fluct='0',
        this.topoLinks[index].timedelay_fluct_percent='0',
        this.topoLinks[index].timedelay_fluct_distribution='normal',
        this.topoLinks[index].corrupt='0',
        this.topoLinks[index].duplicate='0',
        this.topoLinks[index].packetloss='0',
        this.topoLinks[index].packetloss_success='100',
        window.console.log(response)
        this.$message({
          type:'success',
          message:'重置成功'
        })
      }).catch((error)=>{
        window.console.log(error)
        this.$message({
          type:'error',
          message:'重置失败'
        })
      })
    },
    linkupdate(index){
      axios.post('http://localhost:5000/linkupdate',{link:this.topoLinks[index]})
      .then((response)=>{
        window.console.log(response)
        this.$message({
          type:'success',
          message:'更新成功'
        })
        this.islinkdatachange = false
      })
      .catch((error)=>{
        this.$message({
          type:'error',
          message:'更新失败:'+error
        })
      })
    },
    linkdatachange(row, column){
      // window.console.log(row, column)
      MessageBox.prompt(`请输入新的${column.label}`, '修改链路数据', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        inputPattern: /^.*\S.*$/,
        inputErrorMessage:'输入有误请重新输入',
      }).then((value)=>{
        this.topoLinks[row.index][column.property]=value.value;
        this.$message({
          type:'success',
          message:`${column.label}配置成功`
        });
        Notification({
          title: '消息',
          message: '链路信息已修改请记得更新链路以便保存新的链路连接状态',
          type:'warning'
        });
        this.islinkdatachange=true
      }).catch((error)=>{
        window.console.log(error)
      })
      // window.console.log(this.topoLinks)
    },
    linedata(index){
      // window.console.log('gt1')
      return [{
        index:index,
        startNodeName:this.topoLinks[index].startNodeName,
        endNodeName:this.topoLinks[index].endNodeName,
        BandWidth:this.topoLinks[index].BandWidth,
        timedelay:this.topoLinks[index].timedelay,
        corrupt:this.topoLinks[index].corrupt,
        packetloss:this.topoLinks[index].packetloss,
        duplicate:this.topoLinks[index].duplicate,
        timedelay_fluct:this.topoLinks[index].timedelay_fluct,
        packetloss_success:this.topoLinks[index].packetloss_success,
        timedelay_fluct_percent:this.topoLinks[index].timedelay_fluct_percent,
        timedelay_fluct_distribution:this.topoLinks[index].timedelay_fluct_distribution,
      }]
    },
    closePopover(index) {
      if(this.islinkdatachange){
        MessageBox.confirm('确认退出？(修改但未更新链路数据)','提示',{
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'warning'
        }).then(()=>{
          this.lines[index].popvisible = false;
          this.$refs['popover_' + index][0].doClose();
        }).catch(()=>{
          this.$message({
            type:'info',
            message:'取消退出'
          });
        })
      }
      else{
        this.lines[index].popvisible = false;
        this.$refs['popover_' + index][0].doClose();
      }
    },
    linehello(index){
      // window.console.log(this.$refs['popover_' + index][0])
      this.$refs['popover_' + index][0].doShow();
      this.lines[index].popvisible = true;
      // window.console.log(this.lines)
      // this.$message({
      //   type:'success',
      //   message:'hello,this is a line'
      // })
    },
    changetypelist(){
      if(this.changetypelist_add){
        // window.console.log(this.changetypelist_value)
        MessageBox.prompt('请输入节点类型名称', '添加节点类型', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
        })
        .then(({ value }) => {
          this.libraryList[this.changetypelist_value].push({name:value,pic: require('../data/img/router.png')})
          this.typelist_options = Object.entries(this.libraryList).map(([category, items]) => ({
            value: category,
            label: category,
            children: items.map(item => ({ value: item.name, label: item.name })),
          }))
          this.$message({
            type: 'success',
            message: '添加成功'
          });
        })
        .catch((error) => {
          this.$message({
            type: 'info',
            message: '取消添加'+error
          });       
        });
      }
      else{
        // window.console.log(this.changetypelist_value)
        const filteredLibraryList = Object.fromEntries(
          Object.entries(this.libraryList).map(([category, items]) => {
            // 获取当前类别需要删除的name列表
            const namesToRemove = this.changetypelist_value.filter(arr => arr[0] === category).map(arr => arr[1]);

            // 过滤items，排除需要删除的name
            const filteredItems = items.filter(item => !namesToRemove.includes(item.name));

            // 返回过滤后的新类别及其包含的items
            return [category, filteredItems];
          })
        );
        // window.console.log(filteredLibraryList);
        this.libraryList = filteredLibraryList;
        this.typelist_options = Object.entries(this.libraryList).map(([category, items]) => ({
          value: category,
          label: category,
          children: items.map(item => ({ value: item.name, label: item.name })),
        }))
        this.$message({
          type: 'error',
          message: '删除成功'
        });
      }
      this.centerDialogVisible_typelist = false
    },
    typelist(value){
      this.centerDialogVisible_typelist = true;
      if(value == 'add')this.changetypelist_add = true
      if(value == 'del')this.changetypelist_add = false
    },
    importtopo(){
      this.centerDialogVisible = true;
      axios.get('http://localhost:5000/topodatafilename')
      .then(response=>{
        let filenames = response.data
        window.console.log(filenames)
        this.filename_options = filenames.map((filename)=> ({value:filename,label:filename}))
      })
    },
    importtopo2(){
      this.centerDialogVisible = false;
      window.console.log(this.filename_value)
      axios.post('http://localhost:5000/gettopodata',{filename:this.filename_value})
      .then(response => {
        const topoData = response.data;
        
        // 将数据还原到前端变量中
        // this.topoNodes = JSON.parse(topoData.nodedata);
        // this.topoLinks = JSON.parse(topoData.linkdata);
        // this.Nodes_type_number = topoData.Nodes_type_number;
        // this.node_id = topoData.node_id;

        // 或者如果后端返回的是已经解析好的对象，则无需再次解析
        this.topoNodes = topoData.nodedata;
        this.topoLinks = topoData.linkdata;
        this.Nodes_type_number = topoData.Nodes_type_number;
        this.node_id = topoData.node_id;
      })
      .catch(error => window.console.error(error));
    },
    addfile(index,value){
      this.topoNodes[index].addfile = value
      // window.console.log(value,this.topoNodes[index])
    },
    getimages(data){
      this.images=data;
      // window.console.log(data)
      // let imagesname = this.images.map(item=>item.tags[0]).filter(item => item !== undefined)
      // window.console.log(imagesname)
    },
    changefileinput(){
      this.isshow_fileinput=!this.isshow_fileinput;
    },
    handleOpen(key, keyPath) {
      window.console.log(key, keyPath);
    },
    handleClose(key, keyPath) {
      window.console.log(key, keyPath);
    },
    changeshowmanage(){
      this.showmanage = false
    },
    openmanage(){
      this.showmanage = true
      /* eslint-disable */
          // console.log("gt3", this.showmanage)
          /* eslint-disable */
    },
    // 从左边的节点库拖出节点
    dragToBoardStart (e) {
      // 设置拖出的数据
      e.dataTransfer.setData('text/plain', JSON.stringify({pic: e.target.children[0].src,type:e.target.children[0].alt,name: e.target.children[1].innerText}))
      e.dataTransfer.effectAllowed = "copy" // 设置拖的操作为复制操作
      // window.console.log(e)
    },
    // 节点拖放到topo图区域，即新建节点
    dropToBoard (e) {
      const content = JSON.parse(e.dataTransfer.getData('text/plain')) // 接收来自拖出的内容,并还原为对象
      //const date = new Date()
      // window.console.log(content)
      this.addnode(content.name,content.type,e.layerX,e.layerY,content.pic)
      // const name = this.Nodes_type_number.find(obj => obj.name === content.name)
      // // window.console.log(this.Nodes_type_number)
      // if (name) {
      //   name.number += 1
      // }
      // else{
      //   this.Nodes_type_number.push({name: content.name, number: 1,type:content.type})
      // }
      // const newname = this.Nodes_type_number.find(obj => obj.name === content.name)
      // // window.console.log(this.Nodes_type_number)
      // let node = {
      //   id: this.node_id, //每个节点的唯一id
      //   number: newname.number, // 每个节点的类型id
      //   pic: content.pic, // 图片
      //   name: content.name + newname.number, // 默认显示名称，可修改
      //   type: content.name , 
      //   x: e.layerX, // 横坐标
      //   y: e.layerY, // 纵坐标
      //   offsetX:this.offsetX,
      //   offsetY:this.offsetY,
      //   radio:this.radio,
      //   ports: {},
      //   cls: content.type,
      //   addfile:'',
      // }
      // // window.console.log(node.type)
      // if(node.type=='station'){
      //   // window.console.log(123)
      //   node.ports['wlan0']={
      //     ip: null,
      //     ipv6:null,
      //     id: node.id,
      //   }
      // }
      // // if (node.ports["3000"] === undefined) {
      // //   // 添加端口
      // //   node.ports["3000"] = {
      // //     ip: "192.168.0.1"
      // //   }
      // // } 
      // this.node_id += 1
      // this.topoNodes.push(node)
    },
    // 移动topo图中的节点，连接节点 自动配置ip
    moveAndLink (index, e) {
      // 判断当前模式
      if (this.move) {
        // 移动模式
        const layerX = e.layerX - this.topoNodes[index].x
        const layerY = e.layerY - this.topoNodes[index].y
        document.onmousemove = (e) => {
          this.topoNodes[index].x = e.layerX - layerX 
          this.topoNodes[index].y = e.layerY - layerY
        }
        document.onmouseup = () => {
          document.onmousemove = null
          document.onmouseup = null
        }
      } 
      else {
        // 连线模式
        //连线是否存在标志
        let flag=false

        for (const key in this.topoLinks) {
          if ((this.topoLinks[key].startNodeId==this.topoNodes[this.indexOfMenu].id) && (this.topoLinks[key].endNodeId==this.topoNodes[index].id)) {
            flag=true
          }
          if((this.topoLinks[key].startNodeId==this.topoNodes[index].id) && (this.topoLinks[key].endNodeId==this.topoNodes[this.indexOfMenu].id))
          {
            flag=true
          }
        }

        if(flag)
        {
          MessageBox('连接已存在，请重新连接')
        }
        else {
          this.addlink(this.indexOfMenu,index)
          // this.topoLinks.push({
          //   startNodeId: this.topoNodes[this.indexOfMenu].id,
          //   startNodeName: this.topoNodes[this.indexOfMenu].name,
          //   endNodeId: this.topoNodes[index].id,
          //   endNodeName: this.topoNodes[index].name,
          //   startInterface: 'fa0/1',
          //   endInterface: 'fa0/1',
          //   linestate:this.linestate,
          //   linecolor:this.linecolor,
          //   linewidth:this.linewidth,
          //   BandWidth:'50',
          //   timedelay:'0',
          //   timedelay_fluct:'0',
          //   timedelay_fluct_percent:'0',
          //   timedelay_fluct_distribution:'normal',
          //   corrupt:'0',
          //   duplicate:'0',
          //   packetloss:'0',
          //   packetloss_success:'0',
          // })

          // // // 默认根据连接的数量进行ip配置：192.168.1. /24 第一根链路ip为1 2，第二根为 3 4 ...
          // // this.topoNodes[this.indexOfMenu].ports[`to ${this.topoNodes[index].name}`] = {
          // //   ip: '192.168.1.' + (this.topoLinks.length * 2 - 1) + '/24',
          // //   ipv6:null,
          // //   id: this.topoNodes[index].id
          // // }
          // // this.topoNodes[index].ports[`to ${this.topoNodes[this.indexOfMenu].name}`] = {
          // //   ip: '192.168.1.' + (this.topoLinks.length * 2 ) + '/24',
          // //   ipv6:null,
          // //   id: this.topoNodes[this.indexOfMenu].id
          // // }
          // if(this.topoNodes_copy.some(node => node.name === this.topoNodes[this.indexOfMenu].name)){
          //   this.topoNodes[this.indexOfMenu].ports[`to ${this.topoNodes[index].name}`] = {
          //     ip: null,
          //     ipv6:null,
          //     id: this.topoNodes[index].id
          //   }
          //   window.console.log('创建了容器的节点',this.topoNodes[this.indexOfMenu])
          // }else{
          //   this.topoNodes[this.indexOfMenu].ports[`to ${this.topoNodes[index].name}`] = {
          //     ip: '192.168.1.' + this.ipnum + '/24',
          //     ipv6:null,
          //     id: this.topoNodes[index].id
          //   }
          //   this.ipnum++;
          //   window.console.log('没创建容器的节点',this.topoNodes[this.indexOfMenu])
          // }

          // if(this.topoNodes_copy.some(node => node.name === this.topoNodes[index].name)){
          //   this.topoNodes[index].ports[`to ${this.topoNodes[this.indexOfMenu].name}`] = {
          //     ip: null,
          //     ipv6:null,
          //     id: this.topoNodes[this.indexOfMenu].id
          //   }
          //   window.console.log('创建了容器的节点',this.topoNodes[index])
          // }else{
          //   this.topoNodes[index].ports[`to ${this.topoNodes[this.indexOfMenu].name}`] = {
          //     ip: '192.168.1.' + this.ipnum + '/24',
          //     ipv6:null,
          //     id: this.topoNodes[this.indexOfMenu].id
          //   }
          //   this.ipnum++;
          //   window.console.log('没创建容器的节点',this.topoNodes[index])
          // }
        }
        
        this.connecting = { // 重置正在连接的线
          x1: 0,
          y1: 0,
          x2: 0,
          y2: 0
        }
        document.onmousemove = null // 重置鼠标移动事件
        this.move = true // 重置为移动模式
      }
    },
    // 显示topo图上的节点的右键菜单
    nodeMenu (index, e) {
      // window.console.log(123)
      this.position = {x: e.layerX, y: e.layerY}
      this.showMenu = true
      this.indexOfMenu = index
      this.indexOfMenu_copy = index 
    },
    handleClose(done) {
      this.$confirm('确认关闭？')
        .then(() => {
          done();
        })
        .catch(()=> {});
    },
    //未使用
    dealChange(data) {
      /* eslint-disable */
      console.log('gtdealchange', data)
      /* eslint-disable */
      // if(this.topoNodes[this.indexOfMenu].name!=data[0].topo_name){
      //   this.topoNodes[this.indexOfMenu].name=data[0].topo_name
      //   axios.post('http://localhost:5000/posts', {nodes: this.topoNodes[this.indexOfMenu],warning:`id为${this.indexOfMenu}节点名称被修改`},{
      //     headers: {
      //       'Content-Type': 'application/json'
      //     }
      //   })
      // }
    },
    //修改端口ip
    dealChange2(data) {
      for (const key in data) {
        let ip_change = false
        let ipv6_change = false
        if(this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou].ip!=data[key].topo_ip){
          this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou] = { 
            ...this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou],
            ip:data[key].topo_ip,
          }
          ip_change = true
        }
        if(this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou].ipv6!=data[key].topo_ipv6){
          this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou] = { 
            ...this.topoNodes[this.indexOfMenu].ports[data[key].topo_duankou],
            ipv6:data[key].topo_ipv6,
          }
          ipv6_change = true
        }
        if(ip_change||ipv6_change){
          axios.post('http://localhost:5000/setport', {node: this.topoNodes[this.indexOfMenu],portname:data[key].topo_duankou,ip_change:ip_change,ipv6_change:ipv6_change},{
            headers: {
              'Content-Type': 'application/json'
            }
          }).then(response=>{
            window.console.log(response)
          }).catch(error=>{
            window.console.log(error)
          })
        }
        
        // if(this.topoNodes[data[key].topo_id].name!=data[key].topo_name){
        //   this.topoNodes[data[key].topo_id].name=data[key].topo_name
        //   axios.post('http://localhost:5000/posts', {nodes: this.topoNodes[data[key].topo_id],warning:`与id为${this.indexOfMenu}的节点端口相连的设备名称被修改`},{
        //     headers: {
        //       'Content-Type': 'application/json'
        //     }
        //   })
        // }
      }
    },
    // 执行右键菜单的功能
    clickMenuItem (option) {
      // 关闭右键菜单
      this.showMenu = false
      // 连接功能
      if (option === 'link') {
        // 设置为连线模式
        this.move = false
        // 创建连线
        this.connecting = {
          x1: this.topoNodes[this.indexOfMenu].x + 20 - this.topoNodes[this.indexOfMenu].offsetX + this.topoNodes[this.indexOfMenu].radio*6000,
          y1: this.topoNodes[this.indexOfMenu].y + 20 - this.topoNodes[this.indexOfMenu].offsetY + this.topoNodes[this.indexOfMenu].radio*4000,
          x2: this.topoNodes[this.indexOfMenu].x + 20 - this.topoNodes[this.indexOfMenu].offsetX + this.topoNodes[this.indexOfMenu].radio*6000,
          y2: this.topoNodes[this.indexOfMenu].y + 20 - this.topoNodes[this.indexOfMenu].offsetY + this.topoNodes[this.indexOfMenu].radio*4000
        }
        // 连线终点跟随鼠标
        document.onmousemove = (e) => {
          this.connecting.x2 = e.layerX- this.topoNodes[this.indexOfMenu].offsetX + this.topoNodes[this.indexOfMenu].radio*6000
          this.connecting.y2 = e.layerY- this.topoNodes[this.indexOfMenu].offsetY + this.topoNodes[this.indexOfMenu].radio*4000
        }
      }
      // 重命名功能
      if (option === 'rename') {
        MessageBox.prompt('请输入新名称', '重命名', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          inputPattern: /\S/,
          inputErrorMessage: '名称不能为空'
        }).then(({ value }) => {
          this.topoNodes[this.indexOfMenu].name = value
        }).catch((error)=>{
          window.console.log('rename'+error)
        })
      }
      // 删除节点功能
      if (option === 'delete') {
        MessageBox.confirm(`是否删除节点 "${this.topoNodes[this.indexOfMenu].name}"`, '删除节点', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'warning'
        }).then(() => {
          this.topoNodes.splice(this.indexOfMenu, 1)

          // 普通函数的 this 指向容易随环境和调用者变化
          // function test1(link) {
          //   console.log("gt3", link, this.indexOfMenu)
          //   if (link.startNodeId === this.indexOfMenu) return false
          //   if (link.endNodeId === this.indexOfMenu) return false
          //   return true
          // }

          // 第二种写法 箭头函数 不用声明 随时可用
          // this指向 决定于 它所定义的环境，一生下来就固定
          const test2 = (link) => {
            if (link.startNodeId === this.indexOfMenu) return false
            if (link.endNodeId === this.indexOfMenu) return false
            return true
          }

          this.topoLinks = this.topoLinks.filter(test2)

        }).catch((error)=>{
          window.console.log('delete'+error)
        })
      }
      //属性功能
      if (option === 'setting') {
        this.showzujian = true
      }
      //打开容器终端功能
      if (option === 'dialog') {
        // this.dialogVisible = true
        axios.post('http://localhost:5000/xterm',{nodes:this.topoNodes[this.indexOfMenu]})
        .then(response => {
          window.console.log(response) 
        })
        .catch(error => {
          this.$message({
            type: 'error',
            message: '打开终端错误：'+ error,
          });  
        })
        // console.log(this.topoNodes[this.indexOfMenu])
      }
      //显示容器状态信息功能
      if (option === 'stats') {
        this.dialogVisible = true;
        this.cpuChartData.datasets[0].data=[];
        this.cpuChartData.labels=[];
        this.ioChartData.datasets[0].data=[];
        this.ioChartData.datasets[1].data=[];
        this.ioChartData.labels=[];
        this.memoryChartData.datasets[0].data=[];
        this.memoryChartData.datasets[1].data=[];
        this.memoryChartData.labels=[];
        this.networkChartData.datasets[0].data=[];
        this.networkChartData.datasets[1].data=[];
        this.networkChartData.labels=[];
        this.fetchContainerStats();
        setInterval(() => {
          if (this.dialogVisible){
            this.fetchContainerStats();
          }
        }, 6000);
      }
      //输出容器日志信息功能
      if (option === 'logs') {
        // console.log('输出日志信息')
        // console.log(this.topoNodes[this.indexOfMenu_copy].name)
        axios.post('http://localhost:5000/containers/logs',{name:this.topoNodes[this.indexOfMenu_copy].name})
        .then(response => {
            /* eslint-disable */
            // console.log("gt3", response.data)
            /* eslint-disable */
            console.log(response.data)
        })
        .catch((error) => {
          console.error("logs:", error);
        });
      }
      //容器向指定服务器发起文本资源请求功能
      if (option === 'server1'){
        MessageBox.prompt('请输入要访问的服务器ip及文件位置（示例x.x.x.x/xx.txt）', '获取服务器资源（文本）', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          inputPattern: /^(?:(2(?:5[0-5]|[0-4]\d)|[0-1]?\d{1,2})(\.(2(?:5[0-5]|[0-4]\d)|[0-1]?\d{1,2})){3})(?:\/(.*))?/,
          inputErrorMessage: '输入格式有误请重新输入'
        }).then(({ value }) => {
          // window.console.log(value.split('/')[0],value.split('/')[1])
          axios.post('http://localhost:5000/server/nginx/text',{name:this.topoNodes[this.indexOfMenu].name,ip:value.split('/')[0],filepath:value.split('/')[1]})
          .then(response=>{
            const newWindow = window.open('', '_blank');

            newWindow.document.open();
            newWindow.document.write(response.data);
            newWindow.document.close();
            Notification({
              title: '成功',
              message: '文件获取成功',
              type: 'success',
              duration:0,
            });
          })
          .catch((error) => {
              console.error("gtError:", error);
              Notification({
                title: '错误',
                message: '文件获取失败',
                type: 'error',
                duration:0,
              });
          });
        }).catch((error)=>{
          window.console.log('gtserver1'+error)
        })
        // console.log(this.topoNodes.find(node=>node.name=='nginx1').ports['to '+this.topoNodes[this.indexOfMenu].name].ip)
      }
      //容器向指定服务器发起视频资源请求功能
      if (option === 'server2'){
        MessageBox.prompt('请输入要访问的服务器ip及文件位置（示例x.x.x.x/xx.txt）', '获取服务器资源（视频）', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          inputPattern: /^(?:(2(?:5[0-5]|[0-4]\d)|[0-1]?\d{1,2})(\.(2(?:5[0-5]|[0-4]\d)|[0-1]?\d{1,2})){3})(?:\/(.*))?/,
          inputErrorMessage: '输入格式有误请重新输入'
        }).then(({ value }) => {
          // window.console.log(value.split('/')[0],value.split('/')[1])
          axios.post('http://localhost:5000/server/nginx/video',{name:this.topoNodes[this.indexOfMenu].name,ip:value.split('/')[0],filepath:value.split('/')[1]})
          .then(response=>{
            window.console.log(response)
            Notification({
              title: '成功',
              message: '文件获取成功'+response,
              type: 'success',
              duration:0,
            });
          })
          .catch((error) => {
              console.error("gtError:", error);
              Notification({
                title: '失败',
                message: '文件获取失败'+error,
                type: 'success',
                duration:0,
              });
          });
        }).catch((error)=>{
          window.console.log('gtserver2'+error)
        })
        // console.log(this.topoNodes.find(node=>node.name=='nginx1').ports['to '+this.topoNodes[this.indexOfMenu].name].ip)
      }
      //将容器封装成镜像功能
      if (option === 'makeimagefromcontainer'){
        // console.log(this.topoNodes[this.indexOfMenu].name)
        // console.log(this.images)
        if(this.images.length==0){
          this.$message({
            type: 'info',
            message: '当前未知本地仓库镜像信息，请先查看容器镜像'
          });  
        }
        else{
          MessageBox.prompt('请输入镜像名称', '创建镜像', {
            confirmButtonText: '确定',
            cancelButtonText: '取消',
          })
          .then(({ value }) => {
            // console.log(value)
            value = value.trim()
            if(value!=''){
              let imagesname = this.images.map(item=>item.tags[0]).filter(item => item !== undefined)
              // console.log(imagesname)
              if(imagesname.some(item=>(item===value)||(item===`${value}:latest`)))
              {
                this.$message({
                  type: 'error',
                  message: '镜像名称已存在，请重试'
                });
              }
              else
              {
                axios.post('http://localhost:5000/makeimagefromcontainer',{name:this.topoNodes[this.indexOfMenu].name,imagename:value})
                .then(response=>{
                    console.log(response)
                    this.$message({
                      type: 'success',
                      message: value + '镜像创建成功'
                    });
                })
                .catch((error) => {
                      console.error("gtError:", error);
                      this.$message({
                      type: 'error',
                      message: error
                    });
                });
              }
            }
            else{
              this.$message({
                  type: 'error',
                  message: '输入有误，请重新输入'
                });
            }
          })
          .catch(() => {
            this.$message({
              type: 'info',
              message: '取消创建'
            });       
          });
        }
        
      }
    },
    

    // ---------------canvas相关函数------------------------
    extractCPUData(cpuUsage) {
      return parseFloat(cpuUsage.trim()) * 100; 
    },
    extractData(data) {
      const [preWithUnit, postWithUnit] = data.split('/').map(str => str.trim());

      const [pre, unitPre] = this.extractValueAndUnit(preWithUnit);
      const [post, unitPost] = this.extractValueAndUnit(postWithUnit);

      // 根据单位进行处理
      const processedPre = unitPre === 'kB' ? pre / 1024 : pre;
      const processedPost = unitPost === 'kB' ? post / 1024 : post;

      return [processedPre, processedPost];
    },
    extractValueAndUnit(str) {
      const match = str.match(/([\d.]+)(\S+)/);
      if (match) {
        return [parseFloat(match[1]), match[2]];
      }
      return [null, null];
    },
    updateChartData(data, chartData, extractor) {
      const currentTime = new Date();
      const formattedTime = currentTime.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
      const extractedData = extractor(data);
      let unit = '';
      if(chartData.datasets[0].label==='CPU Usage'){
        chartData.labels.push(formattedTime)
        chartData.datasets[0].data.push(extractedData);
        unit = '%'
      }
      else {
        chartData.labels.push(formattedTime)
        chartData.datasets[0].data.push(extractedData[0]);
        chartData.datasets[1].data.push(extractedData[1]);
        unit = 'MB'
      }
      
      this.updateChart(chartData,unit);
    },
    updateChart(chartData,unit) {
      const canvas = this.$refs[`${chartData.datasets[0].label}`];
      if (canvas) {
        const ctx = canvas.getContext('2d');
        Chart.register(Filler,PointElement,LineElement,LineController,BarElement,BarController, LinearScale,TimeScale,CategoryScale, Tooltip,Legend);
        if (!chartData._chart) {
          console.log('gta')
          chartData._chart = new Chart(ctx, {
              type: 'line',
              data: chartData,
              options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                  x: {
                      type: 'category', // 使用 'category' 类型
                      position: 'bottom', 
                      ticks: {
                        font: {
                          size: 10, // 设置 x 轴刻度字体大小
                        }
                      },
                  },
                  y: {
                    type: 'linear', // 同样，根据数据类型选择比例尺类型
                    beginAtZero: true,
                    position: 'left', // Y 轴位置
                    ticks: {
                      // 使用回调函数来自定义刻度的显示
                      callback: function (value) {
                        return value + unit; // 在刻度值后添加单位
                      },
                      font: {
                          size: 10, // 设置 x 轴刻度字体大小
                      }
                    }
                  },
                },
                plugins: {
                  legend: {
                    display: true,
                  },
                  tooltip: {
                     enabled: true,
                    mode: 'nearest', // 'nearest' 表示将鼠标放在数据点附近时显示最近的数据
                    callbacks: {
                      label: function (context) {
                        // context 包含有关要显示的数据的信息
                        const label = context.dataset.label || '';
                        const value = context.parsed.y + unit;
                        return label + ': ' + value;
                      }
                    }
                  },
                },
              },
            }
          )
        } else {
          chartData._chart.update();
        }
        
      } else {
        console.log('chartRef is undefined or null.');
      }
    },
    fetchContainerStats(){
      // console.log(this.topoNodes[this.indexOfMenu].name);
      axios.post('http://localhost:5000/containers/stats',{name:this.topoNodes[this.indexOfMenu_copy].name})
      .then(response=>{
        // console.log('1',response.data);
        const containerData = response.data;
        this.updateChartData(containerData.CPUUsage, this.cpuChartData,this.extractCPUData);
        this.updateChartData(containerData.IOUsage, this.ioChartData, this.extractData);
        this.updateChartData(containerData.MemoryUsage, this.memoryChartData, this.extractData);
        this.updateChartData(containerData.NetworkUsage, this.networkChartData, this.extractData);

        // console.log('2',this.cpuChartData)
        // console.log('3',this.ioChartData)
        // console.log('4',this.memoryChartData)
        // console.log('5',this.networkChartData)
      }).catch((error) => {
        console.error("gtError fetching data:", error);
      });
    },
    // ---------------canvas相关函数------------------------


    // 点击空白地方，关闭右键菜单，取消连线模式
    closeContextMenu () {
      // console.log('gt')
      // 关闭右键菜单
      this.position = {x: 0, y: 0}
      this.showMenu = false
      this.showzujian = false
      this.indexOfMenu = null
      // 取消连线模式
      this.move = true
      this.connecting = {
        x1: 0,
        y1: 0,
        x2: 0,
        y2: 0
      }
      document.onmousemove = null
    },
    // 保存Topo
    saveTopo () {
      localStorage.topoNodes = JSON.stringify(this.topoNodes)
      localStorage.topoLinks = JSON.stringify(this.topoLinks)
      localStorage.Nodes_type_number = JSON.stringify(this.Nodes_type_number)
      localStorage.node_id = JSON.stringify(this.node_id)
      MessageBox.prompt('请输入要保存的文件名称', '保存topo文件', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        inputPattern : /\S/,
        inputErrorMessage : '输入不能为空或仅包含空格',
      })
      .then(({ value }) => {
        axios.post('http://localhost:5000/savetopo',{nodedata:this.topoNodes,linkdata:this.topoLinks,
        Nodes_type_number:this.Nodes_type_number,node_id:this.node_id,filename:value
        })
        .then(response=>{
          this.$message({
            type: 'success',
            message: response+' 保存成功'
          });
        })
        .catch((error) => {
          this.$message({
            type: 'error',
            message: '保存失败请重试：'+error
          });       
        });
      })
      .catch(() => {
        this.$message({
          type: 'info',
          message: '取消保存'
        });       
      });
      
    },
    createTopo() {
      // 假设没有节点删除
      // 假设没有连接删除
      let len1 = this.topoNodes_copy.length
      let len2 = this.topoLinks_copy.length
      let new_topo = []
      let new_link = []
      new_topo = this.topoNodes.slice(len1)
      new_link = this.topoLinks.slice(len2)
      console.log(new_topo)
      this.createtopoloading=true;
      axios.post('http://localhost:5000/create', {nodes: new_topo, links: new_link},{
        headers: {
          'Content-Type': 'application/json'
        }
      })
      .then((response)=>{
        window.console.log(response)
        this.$message({
          type:'success',
          message:'创建成功'
        })
        this.createtopoloading=false;
      })
      .catch((error)=>{
        // window.console.log(error)
        this.$message({
          type:'error',
          message:'创建失败：'+error
        })
        this.createtopoloading=false;
      })
      
      this.topoNodes_copy = this.topoNodes.slice()
      this.topoLinks_copy = this.topoLinks.slice()
    },
    clearTopo () {
      MessageBox.confirm('是否清空当前拓扑图？', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(() => {
        this.clearloading=true
        localStorage.removeItem('topoNodes')
        localStorage.removeItem('topoLinks')
        this.topoNodes = []
        this.topoNodes_copy = []
        this.topoLinks = []
        this.topoLinks_copy = []
        this.Nodes_type_number = []
        this.node_id = 0
        this.ipnum = 1
        axios.post('http://localhost:5000/delete')
        .then(response=>{
          window.console.log(response)
          this.clearloading=false
        })
        .catch((error)=>{
          this.clearloading=false
          this.$message({
            type:'error',
            message:error
          })
        })
      })
      .catch((error)=>{
        this.$message({
          type:'error',
          message:error
        })
      })
      // window.console.log(this.clearloading)
    },

  },
  computed: {
    // 动态计算节点间的连线
    lines () {
      let hash = {}
      const OFFSET = 20
      this.topoNodes.forEach((node, index) => {
        hash[node.id] = index
      })
      return this.topoLinks.map(item => {
        const startNode = this.topoNodes[hash[item.startNodeId]]
        const endNode = this.topoNodes[hash[item.endNodeId]]
        return {
          x1: startNode.x + OFFSET,
          y1: startNode.y + OFFSET,
          x2: endNode.x + OFFSET,
          y2: endNode.y + OFFSET,
          offsetX1:startNode.offsetX,
          offsetX2:endNode.offsetX,
          offsetY1:startNode.offsetY,
          offsetY2:endNode.offsetY,
          radio1:startNode.radio,
          radio2:endNode.radio,
          startInterface: item.startInterface,
          endInterface: item.endInterface,
          linestate: item.linestate,
          linecolor: item.linecolor,
          linewidth:item.linewidth,
          popvisible:false,
        }
      })
    },
  },
  mounted () {
    // this.zoomElement()
    this.libraryList = nodeData
    for(const category in this.libraryList){
      this.$set(this.customizedtopoform, category, [{ name: '', num: 0 }]);
    }
    // window.console.log(this.customizedtopoform)
    
    this.typelist_options = Object.entries(this.libraryList).map(([category, items]) => ({
      value: category,
      label: category,
      children: items.map(item => ({ value: item.name, label: item.name })),
    }))
    // console.log(this.typelist_options)
    this.typeList = Object.keys(this.libraryList)
    if (localStorage.topoNodes && localStorage.topoLinks) {
      this.topoNodes = JSON.parse(localStorage.topoNodes)
      this.topoLinks = JSON.parse(localStorage.topoLinks)
      this.node_id = JSON.parse(localStorage.node_id)
      this.Nodes_type_number = JSON.parse(localStorage.Nodes_type_number)
    }
  }
}
</script>

<style scoped>
* {
  box-sizing: border-box;
}

.asidetoggle {
  /* position:absolute; */
  display: flex;
  /* flex-direction: column; */
  /* top:10px; */
  /* top:5%; */
  left:200px;
  height: 20px;
  width: 20px;
  font-size: 20px;
  text-align: center;
  align-items: center;
  justify-content: center;
  border: 1px solid #409EFF;
  border-radius: 50%;
  /* background-color: #409EFF; */
  color: #409eff;
  /* z-index:99; */

  -webkit-box-align: center;
  -webkit-box-pack: center;
  border-radius: 0.25rem;
  border-width: 0;
  transition-timing-function: cubic-bezier(0.4, 0, 0.2, 1);
  transition-duration: .2s;
  font-size: .875rem;
  line-height: 1.25rem;
}
.asidetoggle:hover{
  background-color: #026aa2;
}

/* .el-icon-d-arrow-left:hover{
  background-color: rgb(39, 172, 190);
}
.el-icon-d-arrow-right:hover{
  background-color: rgb(39, 172, 190);
} */
.el-menu-vertical-demo:not(.el-menu--collapse) {
  width: 200px;
  /* min-height: 200px; */
}
.container_stats {
  position: absolute;
  /* right: -100%; */
  left:66%;
  top:30%;
}
.chart-container {
  padding-left: 2px;
  border-radius: 4px;
  border: 1px solid rgb(124, 123, 123);
  background-color: #fff;
  font-size: 14px;
  width: 220px;
  height: 150px;
}

.abc {
  font-size: 16px;
  font-weight: 400;
  margin: 0%;
}


.el-menu-demo .el-menu-item.centered {
  line-height: 30px;
}
.el-menu-demo .el-submenu .el-submenu__title .submenu-title-container {
  line-height: 30px;
}


.aside {
  background-color: #F2F6FC;
  border-right: 2px solid #F2F6FC;
  height: 100%;
  /* display: flex; */
}
.asidetogg1{
  animation:asidetogg1 0.5s linear forwards;
}
@keyframes asidetogg1 {
  from{
    width:200px;
  }
  to{
    /* transform: translateX(-200px); */
    width:0;
  }
}
.asidetogg2{
  animation:asidetogg2 0.5s linear forwards;
}
@keyframes asidetogg2 {
  from{
    width:0;
  }
  to{
    width:200;
  }
}
.library-item {
  color: #606266;
}
.library-item img {
  width: 30px;
  height: 30px;
  margin-right: 10px;
}
.board-container {
  padding: 0;
  position: relative;
  width:100%;
  height:100%;
  background-color:rgb(170, 221, 170);
}
.board-container::-webkit-scrollbar {
  display: none; /* Chrome, Safari, and Opera */
}
.button-container {
  /* display:flex; */
  position: fixed;
  right: 16px;
  top: 90px;
  z-index:99;
}
.board {
  /* background-color: rgb(235, 216, 218); */
  /* display: flex; */
  position:absolute;
  background-color: rgb(255, 250, 250);
  background-image: linear-gradient(90deg, #e3e7e759 1px, transparent 1px),
      linear-gradient(0deg, #d6dfe259 1px, transparent 1px);
  background-size: 20px 20px, 20px 20px;
  height: 8000px;
  width: 12000px;
  overflow:hidden;
  transform: translate(-50%,-50%);
  /* transform-origin: center; */
}
.svg-left
{
  /* display: flex; */
  position: absolute;
  box-sizing: border-box;
  width: 300px;
  height: 200px;
  padding: 10px 15px;
  overflow: auto;
  background: linear-gradient(#99fffe, #99fffe) left -3px top 0, 
      linear-gradient(#99fffe, #99fffe) left -3px top -3px, 
      linear-gradient(#99fffe, #99fffe) right -3px top 0, 
      linear-gradient(#99fffe, #99fffe) right -3px top -3px, 
      linear-gradient(#99fffe, #99fffe) left -3px bottom 0, 
      linear-gradient(#99fffe, #99fffe) left -3px bottom -3px, 
      linear-gradient(#99fffe, #99fffe) right -3px bottom 0, 
      linear-gradient(#99fffe, #99fffe) right -3px bottom -3px;
  background-color: #00223380;
  background-repeat: no-repeat;
  /* background-size: 20px 20px, 20px 20px; */
  background-size: 3px 16px, 16px 3px;
  border: 1px solid transparent;
  backdrop-filter: blur(1px);
}
.svg-left-title {
  display: flex;
  justify-content: space-between;
  height: 30px;
  margin-bottom: 5px;
  font-size: 18px;
  font-weight: bold;
  color: #fff;
}

.svg-left-main {
  /* display: block; */
  height: calc(100% - 35px);
  overflow: hidden;
}


.header-container {
  /* background-color: #409EFF; */
  background-color: #080808;
}
.background-link {
  display: block;
  /* background-image: url('../data/img/R-C (2).gif'); 替换 'path/to/your/image.jpg' 为你的背景图片文件路径 */
  background-size: cover; /* 可以根据需要设置背景尺寸，cover 表示图片将填充整个链接元素 */
  background-repeat: no-repeat; /* 确保背景图片不重复 */
  background-position: center; /* 居中对齐背景图片 */
  text-align: center; /* 水平居中文本 */
  color: rgb(16, 161, 197); /* 文本颜色，可以根据需要自定义 */
  text-decoration: none; /* 去掉链接的下划线 */
}
.bg-image-0 {
  background-image: url('../data/img/R-C (0).gif');
}
.bg-image-1 {
  background-image: url('../data/img/R-C (1).gif');
}
.bg-image-2 {
  background-image: url('../data/img/R-C (2).gif');
}
.bg-image-3 {
  background-image: url('../data/img/R-C (3).gif');
}
.bg-image-4 {
  background-image: url('../data/img/R-C (4).gif');
}
.bg-image-5 {
  background-image: url('../data/img/R-C (5).gif');
}
.bg-image-6 {
  background-image: url('../data/img/R-C (6).gif');
}
.bg-image-7 {
  background-image: url('../data/img/R-C (7).gif');
}
.bg-image-8 {
  background-image: url('../data/img/R-C (8).gif');
}
.bg-image-9 {
  background-image: url('../data/img/R-C (9).gif');
}
.bg-image-10 {
  background-image: url('../data/img/R-C (10).gif');
}
.bg-image-11 {
  background-image: url('../data/img/R-C (11).gif');
}
.bg-image-12 {
  background-image: url('../data/img/R-C (12).gif');
}
.bg-image-13 {
  background-image: url('../data/img/R-C (13).gif');
}
.bg-image-14 {
  background-image: url('../data/img/R-C (14).gif');
}

.title {
  margin: 0;
  line-height: 60px;
  color: rgb(170, 216, 228);
}
.address {
  margin: 0;
  color: #409EFF;
  line-height: 60px;
}
.address:hover{
  color: #c01919ec;
}
</style>
