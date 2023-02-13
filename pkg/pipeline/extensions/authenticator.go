package extensions

import "net/http"

// ClientAuthenticator allow adding auth or signature info to HTTP client request
type ClientAuthenticator interface {
	// RoundTripper returns a RoundTripper that can be used to authenticate HTTP requests
	RoundTripper(base http.RoundTripper) (http.RoundTripper, error)
}
