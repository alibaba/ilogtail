package helper

import (
	"context"
	"github.com/alibaba/ilogtail/pkg"
	"github.com/alibaba/ilogtail/pkg/util"
	"sort"
	"strings"
)

// ManagerMeta is designed for a special input plugin to log self telemetry data, such as telegraf.
// The kind of plugin would connect with other agents, so most of them have a global manager to control or connect with other agents.
type ManagerMeta struct {
	Metas map[string]map[string]map[string]struct{}
	meta  *pkg.LogtailContextMeta
	ctx   context.Context
}

func NewmanagerMeta(configName string) *ManagerMeta {
	ctx, meta := pkg.NewLogtailContextMeta("", "", configName)
	return &ManagerMeta{
		Metas: make(map[string]map[string]map[string]struct{}),
		ctx:   ctx,
		meta:  meta,
	}
}

func (b *ManagerMeta) Add(prj, logstore, cfg string) {
	change := false
	if _, ok := b.Metas[prj]; !ok {
		b.Metas[prj] = make(map[string]map[string]struct{})
		change = true
	}
	if _, ok := b.Metas[prj][logstore]; !ok {
		b.Metas[prj][logstore] = make(map[string]struct{})
		change = true
	}
	if _, ok := b.Metas[prj][logstore][cfg]; !ok {
		b.Metas[prj][logstore][cfg] = struct{}{}
	}
	if change {
		b.UpdateAlarm()
	}
}

func (b *ManagerMeta) Delete(prj, logstore, cfg string) {
	change := false
	delete(b.Metas[prj][logstore], cfg)
	if _, ok := b.Metas[prj][logstore]; ok && len(b.Metas[prj][logstore]) == 0 {
		delete(b.Metas[prj], logstore)
		change = true
	}
	if _, ok := b.Metas[prj]; ok && len(b.Metas[prj]) == 0 {
		delete(b.Metas, prj)
		change = true
	}
	if change {
		b.UpdateAlarm()
	}
}

func (b *ManagerMeta) UpdateAlarm() {
	var prjSlice, logstoresSlice []string
	for prj, logstores := range b.Metas {
		for logstore := range logstores {

			logstoresSlice = append(logstoresSlice, logstore)
		}
		prjSlice = append(prjSlice, prj)
	}
	sort.Strings(prjSlice)
	sort.Strings(logstoresSlice)
	b.meta.GetAlarm().Update(strings.Join(prjSlice, ","), strings.Join(logstoresSlice, ","))
}

func (b *ManagerMeta) GetAlarm() *util.Alarm {
	return b.meta.GetAlarm()
}

func (b *ManagerMeta) GetContext() context.Context {
	return b.ctx

}
