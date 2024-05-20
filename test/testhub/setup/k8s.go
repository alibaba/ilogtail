package setup

import (
	"bytes"
	"context"
	"fmt"

	"github.com/alibaba/ilogtail/test/config"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/kubernetes/scheme"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
	"k8s.io/client-go/tools/remotecommand"
)

type K8sEnv struct {
	deployType string
	config     *rest.Config
	k8sClient  *kubernetes.Clientset
}

func NewDaemonSetEnv() *K8sEnv {
	env := &K8sEnv{
		deployType: "daemonset",
	}
	env.initK8sClient()
	return env
}

func (k *K8sEnv) ExecOnLogtail(command string) error {
	fmt.Println(command)
	if k.k8sClient == nil {
		return fmt.Errorf("k8s client init failed")
	}
	pods, err := k.getDaemonSetPods("logtail-ds", "kube-system")
	if err != nil {
		return err
	}
	for _, pod := range pods.Items {
		fmt.Println(pod.Name)
		if err := k.execInPod(k.config, pod.Namespace, pod.Name, pod.Spec.Containers[0].Name, []string{"bash", "-c", command}); err != nil {
			return err
		}
	}
	return nil
}

func (k *K8sEnv) ExecOnSource(command string) error {
	fmt.Println(command)
	if k.k8sClient == nil {
		return fmt.Errorf("k8s client init failed")
	}
	pods, err := k.getDeploymentPods("e2e-generator", "default")
	if err != nil {
		return err
	}
	for _, pod := range pods.Items {
		fmt.Println(pod.Name)
		if err := k.execInPod(k.config, pod.Namespace, pod.Name, pod.Spec.Containers[0].Name, []string{"sh", "-c", command}); err != nil {
			return err
		}
	}
	return nil
}

func (k *K8sEnv) initK8sClient() {
	config, err := clientcmd.BuildConfigFromFlags("", config.TestConfig.KubeConfigPath)
	if err != nil {
		panic(err)
	}
	k8sClient, err := kubernetes.NewForConfig(config)
	if err != nil {
		panic(err)
	}
	k.config = config
	k.k8sClient = k8sClient
}

func (k *K8sEnv) getDaemonSetPods(dsName, dsNamespace string) (*corev1.PodList, error) {
	daemonSet, err := k.k8sClient.AppsV1().DaemonSets(dsNamespace).Get(context.TODO(), dsName, metav1.GetOptions{})
	if err != nil {
		return nil, err
	}
	selector := metav1.FormatLabelSelector(daemonSet.Spec.Selector)
	listOptions := metav1.ListOptions{LabelSelector: selector}

	pods, err := k.k8sClient.CoreV1().Pods(dsNamespace).List(context.TODO(), listOptions)
	if err != nil {
		return nil, err
	}
	return pods, nil
}

func (k *K8sEnv) getDeploymentPods(deploymentName, deploymentNamespace string) (*corev1.PodList, error) {
	deployment, err := k.k8sClient.AppsV1().Deployments(deploymentNamespace).Get(context.TODO(), deploymentName, metav1.GetOptions{})
	if err != nil {
		return nil, err
	}
	selector := metav1.FormatLabelSelector(deployment.Spec.Selector)
	listOptions := metav1.ListOptions{LabelSelector: selector}

	pods, err := k.k8sClient.CoreV1().Pods(deploymentNamespace).List(context.TODO(), listOptions)
	if err != nil {
		return nil, err
	}
	return pods, nil
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
		fmt.Println(err)
		return err
	}
	return nil
}
