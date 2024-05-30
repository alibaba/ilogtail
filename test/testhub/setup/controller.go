package setup

import (
	"context"
	"fmt"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/avast/retry-go/v4"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
)

type ContainerFilter struct {
	K8sLabel       map[string]string
	ContainerLabel map[string]string
	Env            map[string]string
	K8sNamespace   string
	K8sPod         string
	K8sContainer   string
}

type DeploymentController struct {
	k8sClient *kubernetes.Clientset
}

func NewDeploymentController(k8sClient *kubernetes.Clientset) *DeploymentController {
	return &DeploymentController{k8sClient: k8sClient}
}

func (c *DeploymentController) GetDeploymentPods(deploymentName, deploymentNamespace string) (*corev1.PodList, error) {
	deployment, err := c.k8sClient.AppsV1().Deployments(deploymentNamespace).Get(context.TODO(), deploymentName, metav1.GetOptions{})
	if err != nil {
		return nil, err
	}
	selector := metav1.FormatLabelSelector(deployment.Spec.Selector)
	listOptions := metav1.ListOptions{LabelSelector: selector}

	pods, err := c.k8sClient.CoreV1().Pods(deploymentNamespace).List(context.TODO(), listOptions)
	if err != nil {
		return nil, err
	}
	// Only return running pods, terminating pods will be excluded
	runningPods := make([]corev1.Pod, 0)
	for _, pod := range pods.Items {
		if pod.DeletionTimestamp == nil {
			runningPods = append(runningPods, pod)
		}
	}
	pods.Items = runningPods
	return pods, nil
}

func (c *DeploymentController) AddFilter(deploymentName string, filter ContainerFilter) error {
	if filter.K8sNamespace == "" {
		filter.K8sNamespace = "default"
	}
	existingDeployment, err := c.k8sClient.AppsV1().Deployments(filter.K8sNamespace).Get(context.TODO(), deploymentName, metav1.GetOptions{})
	if err != nil {
		return err
	}
	// K8s label
	if filter.K8sLabel != nil {
		for k, v := range filter.K8sLabel {
			existingDeployment.Spec.Template.Labels[k] = v
		}
	}
	// env
	if filter.Env != nil {
		for k, v := range filter.Env {
			existingDeployment.Spec.Template.Spec.Containers[0].Env = append(existingDeployment.Spec.Template.Spec.Containers[0].Env, corev1.EnvVar{Name: k, Value: v})
		}
	}
	// K8s container
	if filter.K8sContainer != "" {
		existingDeployment.Spec.Template.Spec.Containers[0].Name = filter.K8sContainer
	}
	_, err = c.k8sClient.AppsV1().Deployments(filter.K8sNamespace).Update(context.TODO(), existingDeployment, metav1.UpdateOptions{})
	if err != nil {
		return err
	}
	return c.waitDeploymentAvailable(deploymentName, filter.K8sNamespace)
}

func (c *DeploymentController) RemoveFilter(deploymentName string, filter ContainerFilter) error {
	if filter.K8sNamespace == "" {
		filter.K8sNamespace = "default"
	}
	existingDeployment, err := c.k8sClient.AppsV1().Deployments(filter.K8sNamespace).Get(context.TODO(), deploymentName, metav1.GetOptions{})
	// K8s label
	if filter.K8sLabel != nil {
		for k := range filter.K8sLabel {
			delete(existingDeployment.Spec.Template.Labels, k)
		}
	}
	// env
	if filter.Env != nil {
		for k := range filter.Env {
			for i, v := range existingDeployment.Spec.Template.Spec.Containers[0].Env {
				if v.Name == k {
					existingDeployment.Spec.Template.Spec.Containers[0].Env = append(existingDeployment.Spec.Template.Spec.Containers[0].Env[:i], existingDeployment.Spec.Template.Spec.Containers[0].Env[i+1:]...)
					break
				}
			}
		}
	}
	_, err = c.k8sClient.AppsV1().Deployments(filter.K8sNamespace).Update(context.TODO(), existingDeployment, metav1.UpdateOptions{})
	if err != nil {
		return err
	}
	return c.waitDeploymentAvailable(deploymentName, filter.K8sNamespace)
}

func (c *DeploymentController) waitDeploymentAvailable(deploymentName, deploymentNamespace string) error {
	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	return retry.Do(
		func() error {
			deployment, err := c.k8sClient.AppsV1().Deployments(deploymentNamespace).Get(context.TODO(), deploymentName, metav1.GetOptions{})
			if err != nil {
				return err
			}
			pods, err := c.GetDeploymentPods(deploymentName, deploymentNamespace)
			if err != nil {
				return err
			}
			if deployment.Status.AvailableReplicas == *deployment.Spec.Replicas && pods != nil && len(pods.Items) == int(*deployment.Spec.Replicas) {
				return nil
			}
			return fmt.Errorf("deployment %s/%s not available yet", deploymentNamespace, deploymentName)
		},
		retry.Context(timeoutCtx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
}

func (c *DeploymentController) DeleteDeployment(deploymentName, deploymentNamespace string) error {
	return c.k8sClient.AppsV1().Deployments(deploymentNamespace).Delete(context.TODO(), deploymentName, metav1.DeleteOptions{})
}

type DaemonSetController struct {
	k8sClient *kubernetes.Clientset
}

func NewDaemonSetController(k8sClient *kubernetes.Clientset) *DaemonSetController {
	return &DaemonSetController{k8sClient: k8sClient}
}

func (c *DaemonSetController) GetDaemonSetPods(dsName, dsNamespace string) (*corev1.PodList, error) {
	daemonSet, err := c.k8sClient.AppsV1().DaemonSets(dsNamespace).Get(context.TODO(), dsName, metav1.GetOptions{})
	if err != nil {
		return nil, err
	}
	selector := metav1.FormatLabelSelector(daemonSet.Spec.Selector)
	listOptions := metav1.ListOptions{LabelSelector: selector}

	pods, err := c.k8sClient.CoreV1().Pods(dsNamespace).List(context.TODO(), listOptions)
	if err != nil {
		return nil, err
	}
	return pods, nil
}
