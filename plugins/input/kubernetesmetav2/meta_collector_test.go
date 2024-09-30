package kubernetesmetav2

import (
	"testing"

	"github.com/alibaba/ilogtail/pkg/flags"
	"github.com/stretchr/testify/assert"
)

func TestGenEntityTypeKeyAcs(t *testing.T) {
	m := metaCollector{
		serviceK8sMeta: &ServiceK8sMeta{},
	}
	*flags.ClusterMode = ackCluster
	m.serviceK8sMeta.initDomain()
	assert.Equal(t, "acs.k8s.pod", m.genEntityTypeKey("pod"))
	assert.Equal(t, "acs.ack.cluster", m.genEntityTypeKey("cluster"))

	*flags.ClusterMode = oneCluster
	m.serviceK8sMeta.initDomain()
	assert.Equal(t, "acs.k8s.pod", m.genEntityTypeKey("pod"))
	assert.Equal(t, "acs.one.cluster", m.genEntityTypeKey("cluster"))

	*flags.ClusterMode = asiCluster
	m.serviceK8sMeta.initDomain()
	assert.Equal(t, "acs.k8s.pod", m.genEntityTypeKey("pod"))
	assert.Equal(t, "acs.asi.cluster", m.genEntityTypeKey("cluster"))
}

func TestGenEntityTypeKeyInfra(t *testing.T) {
	m := metaCollector{
		serviceK8sMeta: &ServiceK8sMeta{},
	}
	*flags.ClusterMode = "k8s"
	m.serviceK8sMeta.initDomain()
	assert.Equal(t, "infra.k8s.pod", m.genEntityTypeKey("pod"))
	assert.Equal(t, "infra.k8s.cluster", m.genEntityTypeKey("cluster"))
}

func TestGenEntityTypeKeyEmpty(t *testing.T) {
	m := metaCollector{
		serviceK8sMeta: &ServiceK8sMeta{},
	}
	m.serviceK8sMeta.initDomain()
	assert.Equal(t, "infra.k8s.pod", m.genEntityTypeKey("pod"))
	assert.Equal(t, "infra.k8s.cluster", m.genEntityTypeKey("cluster"))
}
