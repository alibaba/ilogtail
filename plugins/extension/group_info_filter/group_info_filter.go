package groupinfofilter

import (
	"regexp"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

type filterCondition struct {
	Pattern string
	Reverse bool
	Reg     *regexp.Regexp
}

type ExtensionGroupInfoFilter struct {
	Tags  map[string]*filterCondition // match config for tags, the map value support regex
	Metas map[string]*filterCondition // match config for metas, the map value support regex

	tagsPattern  map[string]*filterCondition
	metasPattern map[string]*filterCondition
	context      pipeline.Context
}

func (e *ExtensionGroupInfoFilter) Description() string {
	return "filter extension that test the GroupInfo if match the config"
}

func (e *ExtensionGroupInfoFilter) Init(context pipeline.Context) error {
	e.context = context
	e.tagsPattern = map[string]*filterCondition{}
	e.metasPattern = map[string]*filterCondition{}

	for k, v := range e.Tags {
		reg, err := regexp.Compile(v.Pattern)
		if err != nil {
			logger.Error(context.GetRuntimeContext(), "EXTENSION_FILTER_ALARM", "regex compile tags err, pattern", v, "error", err)
			return err
		}
		v.Reg = reg
		e.tagsPattern[k] = v
	}

	for k, v := range e.Metas {
		reg, err := regexp.Compile(v.Pattern)
		if err != nil {
			logger.Error(context.GetRuntimeContext(), "EXTENSION_FILTER_ALARM", "regex compile metas err, pattern", v, "error", err)
			return err
		}
		v.Reg = reg
		e.metasPattern[k] = v
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
		isMatch := r.Reg.MatchString(v)
		if isMatch != !r.Reverse {
			return nil
		}
	}

	for k, r := range e.tagsPattern {
		v := group.Group.GetTags().Get(k)
		isMatch := r.Reg.MatchString(v)
		if isMatch != !r.Reverse {
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
