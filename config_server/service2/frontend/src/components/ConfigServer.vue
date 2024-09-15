<template>
    <el-tabs v-model="activeName" tab-position="left" @tab-change="handleClick" >
      <el-tab-pane label="AgentGroup" name="agentGroup" ref="agentGroup">
        <AgentGroup ref="agentGroupComponent"/>
      </el-tab-pane>
      <el-tab-pane label="PipelineConfig" name="pipelineConfig" ref="pipelineConfig">
        <PipelineConfig ref="pipelineConfigComponent"/>
      </el-tab-pane>
<!--      <el-tab-pane label="InstanceConfig" name="instanceConfig" ref="instanceConfig">-->
<!--        <InstanceConfig ref="instanceConfigComponent" />-->
<!--      </el-tab-pane>-->
    </el-tabs>
</template>

<script>
import AgentGroup from "@/components/agentGroup/AgentGroup.vue";
import PipelineConfig from "@/components/config/PipelineConfig.vue";
import InstanceConfig from "@/components/config/InstanceConfig.vue";


export default {
  name: "ConfigServer",
  components: { PipelineConfig, AgentGroup},
  data() {
    return {
      activeName: "agentGroup",
    }
  },

  methods: {
    async handleClick() {
      if (this.activeName === 'agentGroup') {
        await this.$refs.agentGroupComponent.initAllTable()
        this.$refs.agentGroupComponent.initDataMap();
      } else if (this.activeName === 'pipelineConfig') {
        await this.$refs.pipelineConfigComponent.initAllTable()
      } else if (this.activeName === 'instanceConfig') {
        this.$refs.instanceConfigComponent.created();
      }
    },
  }
}
</script>

<style scoped>
</style>

