package groupinfofilter

import (
	"regexp"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

type ExtensionGroupInfoFilter struct {
	Tags  map[string]string // match config for tags, the map value support regex
	Metas map[string]string // match config for metas, the map value support regex

	tagsPattern  map[string]*regexp.Regexp
	metasPattern map[string]*regexp.Regexp
	context      pipeline.Context
}

func (e *ExtensionGroupInfoFilter) Description() string {
	return "filter extension that test the GroupInfo if match the config"
}

func (e *ExtensionGroupInfoFilter) Init(context pipeline.Context) error {
	e.context = context
	e.tagsPattern = map[string]*regexp.Regexp{}
	e.metasPattern = map[string]*regexp.Regexp{}

	for k, v := range e.Tags {
		reg, err := regexp.Compile(v)
		if err != nil {
			logger.Error(context.GetRuntimeContext(), "EXTENSION_FILTER_ALARM", "regex compile tags err, pattern", v, "error", err)
			return err
		}
		e.tagsPattern[k] = reg
	}

	for k, v := range e.Metas {
		reg, err := regexp.Compile(v)
		if err != nil {
			logger.Error(context.GetRuntimeContext(), "EXTENSION_FILTER_ALARM", "regex compile metas err, pattern", v, "error", err)
			return err
		}
		e.metasPattern[k] = reg
	}

	return nil
}

// Filter returns original event if matched, nil if not
func (e *ExtensionGroupInfoFilter) Filter(group *models.PipelineGroupEvents) *models.PipelineGroupEvents {
	if group == nil {
		return nil
	}

	for k, r := range e.metasPattern {
		v := group.Group.GetMetadata().Get(k)
		if !r.MatchString(v) {
			return nil
		}
	}

	for k, r := range e.tagsPattern {
		v := group.Group.GetTags().Get(k)
		if !r.MatchString(v) {
			return nil
		}
	}

	return group
}

func (e *ExtensionGroupInfoFilter) Stop() error {
	return nil
}

func init() {
	pipeline.AddExtensionCreator("ext_groupinfo_filter", func() pipeline.Extension {
		return &ExtensionGroupInfoFilter{}
	})
}
