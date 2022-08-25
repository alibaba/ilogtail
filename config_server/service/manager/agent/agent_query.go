package agentmanager

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (a *AgentManager) GetMachine(id string) (*model.Machine, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINE, id)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		machine, err := s.Get(common.TYPE_MACHINE, id)
		if err != nil {
			return nil, err
		}
		return machine.(*model.Machine), nil
	}
}
