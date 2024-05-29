package control

import (
	"context"
	"encoding/json"

	"github.com/alibaba/ilogtail/test/testhub/setup"
)

func AddLabel(ctx context.Context, labelStr string) (context.Context, error) {
	var labels map[string]string
	if err := json.Unmarshal([]byte(labelStr), &labels); err != nil {
		return ctx, err
	}
	filter := setup.ContainerFilter{
		K8sLabel: labels,
	}
	if err := setup.Env.AddFilter(filter); err != nil {
		return ctx, err
	}
	return ctx, nil
}

func RemoveLabel(ctx context.Context, labelStr string) (context.Context, error) {
	var labels map[string]string
	if err := json.Unmarshal([]byte(labelStr), &labels); err != nil {
		return ctx, err
	}
	filter := setup.ContainerFilter{
		K8sLabel: labels,
	}
	if err := setup.Env.RemoveFilter(filter); err != nil {
		return ctx, err
	}
	return ctx, nil
}
