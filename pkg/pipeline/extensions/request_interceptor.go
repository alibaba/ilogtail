package extensions

import "net/http"

// RequestInterceptor allow custom modifications to the HTTP request before sending
type RequestInterceptor interface {
	// RoundTripper returns a RoundTripper that can be used to intercept the HTTP requests
	RoundTripper(base http.RoundTripper) (http.RoundTripper, error)
}
