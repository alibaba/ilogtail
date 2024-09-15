<template>
  <el-table :data="tableData" style="width: 100%">
    <el-table-column fixed prop="name" label="名称" width="180" />
    <el-table-column prop="version" label="版本" width="180" />
    <el-table-column prop="detail" label="详情" />
    <el-table-column prop="agentGroupCount" label="agentGroup数量">

    </el-table-column>
    <el-table-column fixed="right" label="操作" min-width="120">
      <template #default="scope">
        <el-button link type="primary" size="small"
                   @click="scope.row.isEditing=!scope.row.isEditing">
          <div v-if="!scope.row.isEditing">
            编辑
          </div>
          <div v-else @click="saveInstanceConfig(scope.row.name,scope.row.value)">
            保存
          </div>
        </el-button>
        <el-button link type="primary" size="small"
                   @click="deleteInstanceConfigByName(scope.row.$index,scope.row.name)">
          删除
        </el-button>
      </template>
    </el-table-column>
  </el-table>
</template>

<script>
import {getAppliedAgentGroupsWithInstanceConfig, listInstanceConfigs} from "@/api/instanceConfig";

export default {
  name: "InstanceConfig",
  data(){
    return{
      tableData:[],
      simpleTableData:[],
      mapTableData:{}
    }
  },
  async mounted() {
    this.tableData=await this.initAllTable()
  },
  methods:{
    async initAllTable(){
      let data=await listInstanceConfigs()
      await Promise.all(data.configDetailsList.map(async item => {
        item.agentGroups=(await getAppliedAgentGroupsWithInstanceConfig(item.name)).agentGroupNamesList
        return item
      }))
      return data.configDetailsList
    },
    async initSimpleData() {
      this.tableData.forEach(item => {
        this.simpleTableData.push({
          name: item.name,
          version: item.version,
          detail: item.detail,
          agentGroupCount:item.agentGroupsList.length
        })
        this.mapTableData[item.name] = {
          name: item.name,
          version: item.version,
          detail: item.detail,
          agentGroups:item.agentGroupsList.length,
          agentGroupCount: item.agentList.length,
          isEditing: false,
        }
      })
    },

    deleteInstanceConfigByName(){

    },

    saveInstanceConfig(){

    }
  }
}

</script>

<style scoped>

</style>