package agentmanager

import (
	"github.com/alibaba/ilogtail/config_server/config_server_service/common"
	"github.com/alibaba/ilogtail/config_server/config_server_service/model"
	"github.com/alibaba/ilogtail/config_server/config_server_service/store"
)

func GetAllMachine() ([]model.Machine, error) {
	myMachines := store.GetStore()
	machineList, err := myMachines.GetAll(common.LABEL_MACHINE)
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.Machine, 0)
		for _, machine := range machineList {
			ans = append(ans, *machine.(*model.Machine))
		}
		return ans, nil
	}
}
