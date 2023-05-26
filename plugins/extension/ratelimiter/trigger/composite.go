package trigger

func NewCompositeTrigger(triggers ...Trigger) Trigger {
	return &compositeTrigger{
		triggers: triggers,
	}
}

type compositeTrigger struct {
	triggers []Trigger
}

func (c *compositeTrigger) Open() bool {
	for _, trigger := range c.triggers {
		if trigger.Open() {
			return true
		}
	}
	return false
}

func (c *compositeTrigger) Stop() {
	for _, trigger := range c.triggers {
		trigger.Stop()
	}
}
