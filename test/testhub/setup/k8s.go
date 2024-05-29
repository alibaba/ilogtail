package setup

import (
	"bytes"
	"fmt"
	"math/rand"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	corev1 "k8s.io/api/core/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/kubernetes/scheme"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
	"k8s.io/client-go/tools/remotecommand"
)

type K8sEnv struct {
	deployType           string
	config               *rest.Config
	k8sClient            *kubernetes.Clientset
	deploymentController *DeploymentController
	daemonsetController  *DaemonSetController
}

func NewDaemonSetEnv() *K8sEnv {
	env := &K8sEnv{
		deployType: "daemonset",
	}
	env.init()
	return env
}

func (k *K8sEnv) ExecOnLogtail(command string) error {
	if k.k8sClient == nil {
		return fmt.Errorf("k8s client init failed")
	}
	pods, err := k.daemonsetController.GetDaemonSetPods("logtail-ds", "kube-system")
	if err != nil {
		return err
	}
	for _, pod := range pods.Items {
		if err := k.execInPod(k.config, pod.Namespace, pod.Name, pod.Spec.Containers[0].Name, []string{"bash", "-c", command}); err != nil {
			return err
		}
	}
	return nil
}

func (k *K8sEnv) ExecOnSource(command string) error {
	if k.k8sClient == nil {
		return fmt.Errorf("k8s client init failed")
	}
	pods, err := k.deploymentController.GetDeploymentPods("e2e-generator", "default")
	if err != nil {
		return err
	}
	rand.Seed(time.Now().UnixNano())
	randomIndex := rand.Intn(len(pods.Items))
	pod := pods.Items[randomIndex]
	if err := k.execInPod(k.config, pod.Namespace, pod.Name, pod.Spec.Containers[0].Name, []string{"sh", "-c", command}); err != nil {
		return err
	}
	return nil
}

func (k *K8sEnv) AddFilter(filter ContainerFilter) error {
	return k.deploymentController.AddFilter("e2e-generator", filter)
}

func (k *K8sEnv) RemoveFilter(filter ContainerFilter) error {
	return k.deploymentController.RemoveFilter("e2e-generator", filter)
}

func (k *K8sEnv) init() {
	var c *rest.Config
	var err error
	if len(config.TestConfig.KubeConfigPath) == 0 {
		c, err = rest.InClusterConfig()
	} else {
		c, err = clientcmd.BuildConfigFromFlags("", config.TestConfig.KubeConfigPath)
	}
	if err != nil {
		panic(err)
	}
	k8sClient, err := kubernetes.NewForConfig(c)
	if err != nil {
		panic(err)
	}
	k.config = c
	k.k8sClient = k8sClient
	k.deploymentController = NewDeploymentController(k.k8sClient)
	k.daemonsetController = NewDaemonSetController(k.k8sClient)
}

func (k *K8sEnv) execInPod(config *rest.Config, namespace, podName, containerName string, command []string) error {
	req := k.k8sClient.CoreV1().RESTClient().
		Post().
		Resource("pods").
		Name(podName).
		Namespace(namespace).
		SubResource("exec").
		VersionedParams(&corev1.PodExecOptions{
			Container: containerName,
			Command:   command,
			Stdin:     false,
			Stdout:    true,
			Stderr:    true,
			TTY:       false,
		}, scheme.ParameterCodec)
	executor, err := remotecommand.NewSPDYExecutor(config, "POST", req.URL())
	if err != nil {
		return err
	}
	var stdout, stderr bytes.Buffer
	err = executor.Stream(remotecommand.StreamOptions{
		Stdin:  nil,
		Stdout: &stdout,
		Stderr: &stderr,
	})
	if err != nil {
		return err
	}
	return nil
}
