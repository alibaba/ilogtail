package envconfig

import (
	"fmt"

	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	sls20201230 "github.com/alibabacloud-go/sls-20201230/v5/client"

	"github.com/alibaba/ilogtail/pkg/flags"
)

// CreateNormalInterface 用于创建一个普通的日志服务客户端
func CreateNormalInterface(configObj *AliyunLogConfigSpec) (*sls20201230.Client, error) {
	openapiConfig := &openapi.Config{}
	openapiConfig.Endpoint = flags.LogServiceEndpoint
	if *flags.AliCloudECSFlag {
		accessKeyID, accessKeySecret, stsToken, _, err := UpdateTokenFunction()
		if err != nil {
			return nil, err
		}
		openapiConfig.AccessKeyId = &accessKeyID
		openapiConfig.AccessKeySecret = &accessKeySecret
		openapiConfig.SecurityToken = &stsToken
	} else {
		openapiConfig.AccessKeyId = flags.DefaultAccessKeyID
		openapiConfig.AccessKeySecret = flags.DefaultAccessKeySecret
		openapiConfig.SecurityToken = flags.DefaultSTSToken
	}
	logClient, err := sls20201230.NewClient(openapiConfig)
	if err != nil {
		err = fmt.Errorf("aliyunlog NewClient error:%v", err.Error())
	}
	return logClient, err
}
