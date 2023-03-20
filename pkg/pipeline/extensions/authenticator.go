package extensions

// ClientAuthenticator allow adding auth or signature info to HTTP client request
type ClientAuthenticator interface {
	RequestInterceptor
}
