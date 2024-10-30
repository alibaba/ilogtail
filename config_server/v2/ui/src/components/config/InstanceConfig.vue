<template>
<!--  <el-button type="primary" style="float:left" @click="addInstanceConfig">新增</el-button>-->
  <el-dialog v-model="showCreateForm" >
    <el-form  :model="createInstanceConfig" ref="ruleFormRef" :rules="createInstanceConfigRules">
      <el-form-item label="名称" prop="name">
        <el-input v-model="createInstanceConfig.name" />
      </el-form-item>
      <el-form-item label="版本" prop="version">
        <el-input v-model.number="createInstanceConfig.version" :disabled="true"/>
      </el-form-item>
      <div style="text-align: left">
        配置详情
        <VAceEditor
            v-model:value="createInstanceConfig.detail"
            lang="yaml"
            theme='github'
            :options="editorOptions"
            style="height: 200px; width: 100%;"
        />
      </div>
      <el-button type="primary" @click="onSubmit">保存</el-button>
    </el-form>
  </el-dialog>

  <el-dialog v-model="showEditForm" @close="initAllTable">
    <el-form  :model="selectedRow" ref="ruleFormRef" :rules="createInstanceConfigRules">
      <el-form-item label="名称" prop="name">
        <el-input disabled v-model="selectedRow.name" />
      </el-form-item>
      <el-form-item label="版本" prop="version">
        <el-input v-model.number="selectedRow.version" :disabled="true"/>
      </el-form-item>
      <div style="text-align: left">
        配置详情
        <VAceEditor
            v-model:value="selectedRow.detail"
            lang="yaml"
            theme='github'
            :options="editorOptions"
            style="height: 200px; width: 100%;"
        />
      </div>
      <el-button type="primary" @click="saveInstanceConfig">保存</el-button>
    </el-form>
  </el-dialog>

  <el-table :data="tableData" style="width: 100%">
    <el-table-column fixed prop="name" label="名称" width="180" >
    </el-table-column>
    <el-table-column prop="version" label="版本" width="180" >
      <template v-slot="scope">
        {{ scope.row.version }}
      </template>
    </el-table-column>
    <el-table-column prop="detail" label="详情" >
      <template v-slot="scope">
        {{ scope.row.detail }}
      </template>
    </el-table-column>
    <el-table-column prop="agentGroupCount" label="agentGroup数量">
      <template v-slot="scope">
        <el-button type="primary" link @click="clickAgentGroupCount(scope.row)">{{scope.row.appliedAgentGroupList.length}}</el-button>
      </template>
    </el-table-column>
    <el-table-column fixed="right" label="操作" min-width="120">
      <template #default="scope">
        <el-button link type="primary" size="small"
                   @click="editInstanceDialog(scope.row)">编辑
        </el-button>
        <el-button link type="primary" size="small"
                   @click="deleteInstanceConfigByName(scope.row.name)">
          删除
        </el-button>
      </template>
    </el-table-column>
  </el-table>

  <el-dialog v-model="showAgentGroupDialog" @close="closeAgentGroupDialog">
    <el-table :data="selectedRow.allAgentGroupList">
      <el-table-column property="name" label="名字"  />
      <el-table-column property="value" label="描述"  />
      <el-table-column label="操作" min-width="100">
        <template v-slot="scope">
          <el-button link type="danger" size="small" v-if="scope.row.isApplied"
                     @click="cancelAppliedAgentGroup(scope.row)">
            {{scope.row.description}}
          </el-button>

          <el-button link type="success" size="small" v-else
                     @click="appliedAgentGroup(scope.row)">
            {{scope.row.description}}
          </el-button>
        </template>
      </el-table-column>
    </el-table>
  </el-dialog>
</template>

<script>
import {
  applyInstanceConfigToAgentGroup, createInstanceConfig,
  deleteInstanceConfig,
  getAppliedAgentGroupsWithInstanceConfig,
  listInstanceConfigs, removeInstanceConfigFromAgentGroup,
  updateInstanceConfig
} from "@/api/instanceConfig";
import {getAgentGroup, listAgentGroups} from "@/api/agentGroup";
import {messageShow} from "@/api/common";
import {ElMessage} from "element-plus";
import { VAceEditor } from 'vue3-ace-editor';
import 'ace-builds/src-noconflict/mode-yaml';
import 'ace-builds/src-noconflict/theme-github';
import yaml from "js-yaml";

export default {
  name: "InstanceConfig",
  components:{
    VAceEditor
  },
  data() {
    return {
      editorOptions: {
        fontSize: '14px',
        showPrintMargin: true,
      },
      showCreateForm: false,
      showEditForm:false,
      createInstanceConfig: {
        name: "",
        version: "",
        detail: "",
      },
      createInstanceConfigRules: {
        name: [
          {required: true, message: '请输入配置名', trigger: 'blur'},
        ],
        version: [
          {required: true, message: '请输入版本号', trigger: 'blur'},
          {type: "integer", message: "请输入数字", trigger: 'change'}
        ],
        detail: [
          {required: true, message: '请输入详情', trigger: 'blur'},
        ],
      },
      tableData: [],
      selectedRow: null,

      isEditDetail: false,
      showAgentGroupDialog: false,
    }
  },
  async created() {
    await this.initAllTable()
  },

  methods: {

    async initAllTable() {
      let data = await listInstanceConfigs()
      if (!data.commonResponse.status) {
        this.tableData = await Promise.all(data.configDetailsList.map(async item => {
          return {
            name: item.name,
            version: item.version,
            detail: item.detail,
            appliedAgentGroupList: (await getAppliedAgentGroupsWithInstanceConfig(item.name)).agentGroupNamesList
                .map(res=>{return{"name":res}})
          }
        }))
      }

    },

    addInstanceConfig() {
      this.showCreateForm = true
      this.createInstanceConfig = {
        name: "",
        version: 1,
        detail: "",
      }
    },

    async onSubmit() {
      this.$refs["ruleFormRef"].validate(async valid => {
        if (valid) {
          if(this.createInstanceConfig.detail==null || this.createInstanceConfig.detail.trim()===""){
            ElMessage.error("detail不能为空")
            return
          }
          if(!this.checkConfigDetail(this.createInstanceConfig.detail)){
            return
          }
          let data=await createInstanceConfig(this.createInstanceConfig.name,this.createInstanceConfig.version,this.createInstanceConfig.detail)
          messageShow(data, "新增成功")
          this.showCreateForm = false
          await this.initAllTable()
        }
      })
    },

    findSelectedRow(name){
      for(let row of this.tableData){
        if(row.name===name){
          return row
        }
      }
      return null
    },

    async clickAgentGroupCount(row) {
      this.selectedRow = this.findSelectedRow(row.name)
      this.showAgentGroupDialog = true
      let appliedAgentGroupList = await Promise.all(
          this.selectedRow.appliedAgentGroupList.map(async group => {
            return (await getAgentGroup(group.name)).agentGroup
          }))

      let allAgentGroupList = (await listAgentGroups()).agentGroupsList

      for (let agentGroup of allAgentGroupList) {
        agentGroup.isApplied = false
        agentGroup.description = "应用"
        for (let applied of appliedAgentGroupList) {
          if (agentGroup.name === applied.name) {
            agentGroup.isApplied = true
            agentGroup.description = "取消应用"
            break
          }
        }
      }
      allAgentGroupList.sort((a1, a2) => {
        return a2.isApplied - a1.isApplied
      })

      this.selectedRow.allAgentGroupList=allAgentGroupList
    },

    someDoesNotHaveType(plugins) {
      return plugins.some(item => !item.Type)
    },
    checkConfigDetail(detail) {
      try {
        const obj = yaml.load(detail);
        if (!obj || !obj.inputs || !obj.flushers ||
            this.someDoesNotHaveType(obj.inputs) ||
            this.someDoesNotHaveType(obj.flushers) ||
            obj.processors === null || obj.aggregators === null ||
            (obj.processors && this.someDoesNotHaveType(obj.processors)) ||
            (obj.aggregators && this.someDoesNotHaveType(obj.aggregators))) {
          ElMessage.warning(`Config invalid`)
          return false
        }
      } catch (e) {
        ElMessage.warning(`${e.name} - ${e.reason}`)
        return false
      }
      return true
    },

    async closeAgentGroupDialog(){
      this.showAgentGroupDialog=false
      await this.initAllTable()
    },

    async deleteInstanceConfigByName(name) {
      const data = await deleteInstanceConfig(name)
      messageShow(data, "删除成功")
      await this.initAllTable()
    },

    async saveInstanceConfig() {

      let row=this.selectedRow
      if(row.version==null||row.version===""){
        ElMessage.error("version不能为空")
        return
      }
      if(row.detail==null||row.detail.trim()===""){
        ElMessage.error("detail不能为空")
        return
      }
      if(!this.checkConfigDetail(row.detail)){
        return
      }
      const data = await updateInstanceConfig(row.name, row.version, row.detail)
      messageShow(data, "保存成功")
      this.showEditForm=false
      await this.initAllTable()
    },

    async cancelAppliedAgentGroup(row){
      let data = await removeInstanceConfigFromAgentGroup(this.selectedRow.name,row.name)
      messageShow(data, "取消应用配置成功")
      if (!data.commonResponse.status) {
        row.description = "应用"
        row.isApplied = !row.isApplied
      }
    },

    async appliedAgentGroup(row){
      let data = await applyInstanceConfigToAgentGroup(this.selectedRow.name,row.name)
      messageShow(data, "应用配置成功")
      if (!data.commonResponse.status) {
        row.description = "取消应用"
        row.isApplied = !row.isApplied
      }
    },

    editInstanceDialog(row){
      this.selectedRow=this.findSelectedRow(row.name)
      this.showEditForm=true
    }
  }
}

</script>

<style scoped>

</style>