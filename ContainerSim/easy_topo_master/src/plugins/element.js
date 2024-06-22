import Vue from 'vue'
import { Container, Header, Aside, Main, Footer, Button, Menu, Submenu, MenuItemGroup, MenuItem ,Loading,Cascader,link,Switch,ColorPicker,Slider,Popover,Badge} from 'element-ui'
import { Table,Dialog,Form,Select,Option,Input,Card,FormItem,TableColumn,MessageBox,Message,Descriptions,DescriptionsItem,Tabs,TabPane,Alert,Divider,Col} from 'element-ui'

Vue.prototype.$message = Message
Vue.prototype.$confirm = MessageBox.confirm

Vue.use(Container)
Vue.use(Header)
Vue.use(Aside)
Vue.use(Main)
Vue.use(Footer)
Vue.use(Button)
Vue.use(Menu)
Vue.use(Submenu)
Vue.use(MenuItemGroup)
Vue.use(MenuItem)
Vue.use(Loading)
Vue.use(Cascader)
Vue.use(link)
Vue.use(Switch)
Vue.use(ColorPicker)
Vue.use(Slider)
Vue.use(Popover)
Vue.use(Badge)


Vue.use(Table)
Vue.use(Dialog)
Vue.use(Form)
Vue.use(Select)
Vue.use(Option)
Vue.use(Input)
Vue.use(Card)
Vue.use(FormItem)
Vue.use(TableColumn)
Vue.use(Descriptions)
Vue.use(DescriptionsItem)
Vue.use(Tabs)
Vue.use(TabPane)
Vue.use(Alert)
Vue.use(Divider)
Vue.use(Col)
