package main

import (
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"log"
	"net/http"
)

var reg = prometheus.NewRegistry()

func main() {
	counter := prometheus.NewCounter(prometheus.CounterOpts{
		Name: "test_counter",
		Help: "test counter",
	})
	reg.MustRegister(counter)

	promhttp.Handler()

	http.Handle("/metrics", promhttp.InstrumentMetricHandler(
		reg, promhttp.HandlerFor(reg, promhttp.HandlerOpts{}),
	))

	log.Fatal(http.ListenAndServe(":18080", nil))
}
