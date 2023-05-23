package window

type SampleWindow interface {
	Add(value float64)
	Get() float64
}

type Setter interface {
	Set(fn func(current float64) float64)
}
