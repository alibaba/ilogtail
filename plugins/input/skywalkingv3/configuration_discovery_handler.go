package skywalkingv3

import "context"
import v3 "github.com/alibaba/ilogtail/plugins/input/skywalkingv3/skywalking/network/agent/configuration/v3"
import v3command "github.com/alibaba/ilogtail/plugins/input/skywalkingv3/skywalking/network/common/v3"

type ConfigurationDiscoveryHandler struct {
}

func (*ConfigurationDiscoveryHandler) FetchConfigurations(context context.Context, config *v3.ConfigurationSyncRequest) (*v3command.Commands, error) {
	return &v3command.Commands{}, nil
}
