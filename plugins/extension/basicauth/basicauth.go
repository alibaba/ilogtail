package basicauth

import (
	"net/http"

	"github.com/alibaba/ilogtail/pkg/pipeline"
)

type ExtensionBasicAuth struct {
	Username string
	Password string
}

func (b *ExtensionBasicAuth) Description() string {
	return "basic auth extension to add auth info to client request"
}

func (b *ExtensionBasicAuth) Init(context pipeline.Context) error {
	return nil
}

func (b *ExtensionBasicAuth) Stop() error {
	return nil
}

func (b *ExtensionBasicAuth) RoundTripper(base http.RoundTripper) (http.RoundTripper, error) {
	return &basicAuthRoundTripper{base: base, auth: b}, nil
}

type basicAuthRoundTripper struct {
	base http.RoundTripper
	auth *ExtensionBasicAuth
}

func (b *basicAuthRoundTripper) RoundTrip(request *http.Request) (*http.Response, error) {
	request.SetBasicAuth(b.auth.Username, b.auth.Password)
	return b.base.RoundTrip(request)
}

func init() {
	pipeline.AddExtensionCreator("extension_basicauth", func() pipeline.Extension {
		return &ExtensionBasicAuth{}
	})
}
