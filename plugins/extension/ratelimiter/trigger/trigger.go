package trigger

type Trigger interface {
	Open() bool
	Stop()
}

type FeedTrigger interface {
	Trigger
	Feed(value float64)
}
