package extensions

import "net/http"

type HTTPServerMiddleware interface {
	Handler(handler http.Handler) http.Handler
}
