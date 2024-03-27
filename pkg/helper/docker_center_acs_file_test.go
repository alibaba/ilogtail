package helper

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"

	"github.com/alibaba/ilogtail/pkg/logger"
)

func init() {
	logger.InitTestLogger(logger.OptionDebugLevel, logger.OptionOpenConsole)

}

var configJsonMain = `{
	"ociVersion": "1.0.2-dev",
	"process": {
		"terminal": false,
		"user": {
			"uid": 0,
			"gid": 0
		},
		"args": ["/bin/sh", "-c", "i=0;\nwhile true; do\n  echo \"$(date) - Log line $i\";\n  sleep 1;\n  i=$((i+1));\ndone\n"],
		"env": ["PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", "HOSTNAME=busybox-log-stdout", "aliyun_logs_centos-stdout=stdout", "SLEEP_DUAL_PORT_80_TCP=tcp://22.10.7.100:80", "KUBERNETES_PORT_443_TCP=tcp://22.10.0.1:443", "SLEEP_DUAL_SERVICE_PORT_HTTP=80", "KUBERNETES_PORT=tcp://22.10.0.1:443", "TCP_ECHO_SERVICE_PORT_TCP=9000", "TCP_ECHO_PORT_9001_TCP_PORT=9001", "NGINX_4_SVC_SERVICE_PORT=80", "NGINX_5_SVC_SERVICE_HOST=22.10.211.245", "TCP_ECHO_DUAL_SERVICE_PORT=9000", "SLEEP_SERVICE_PORT=80", "REDIS_DEMO_SERVICE_HOST=22.10.44.113", "NGINX_4_SVC_PORT_80_TCP_ADDR=22.10.187.126", "SLEEP_IPV6_SERVICE_PORT_HTTP=80", "NGINX_SVC_2_PORT_80_TCP_ADDR=22.10.11.161", "SLEEP_IPV6_PORT_80_TCP_PORT=80", "SLEEP_IPV6_PORT_80_TCP_ADDR=fc00::35a0", "NGINX_4_SVC_PORT=tcp://22.10.187.126:80", "TCP_ECHO_DUAL_PORT_9000_TCP_PROTO=tcp", "SLEEP_PORT=tcp://22.10.36.126:80", "NGINX_5_SVC_SERVICE_PORT=80", "TCP_ECHO_SERVICE_PORT=9000", "TCP_ECHO_SERVICE_PORT_TCP_OTHER=9001", "REDIS_DEMO_SERVICE_PORT_REDIS_PORT=6379", "REDIS_DEMO_PORT_6379_TCP_ADDR=22.10.44.113", "NGINX_SVC_2_PORT=tcp://22.10.11.161:80", "NGINX_4_SVC_SERVICE_HOST=22.10.187.126", "SLEEP_IPV6_SERVICE_PORT=80", "TCP_ECHO_DUAL_PORT_9000_TCP_ADDR=fc00::919e", "SLEEP_SERVICE_PORT_HTTP=80", "KUBERNETES_SERVICE_PORT_HTTPS=443", "TCP_ECHO_PORT=tcp://22.10.83.61:9000", "NGINX_4_SVC_PORT_80_TCP=tcp://22.10.187.126:80", "NGINX_5_SVC_PORT=tcp://22.10.211.245:80", "NGINX_5_SVC_PORT_80_TCP=tcp://22.10.211.245:80", "TCP_ECHO_DUAL_SERVICE_PORT_TCP=9000", "TCP_ECHO_DUAL_PORT_9000_TCP=tcp://[fc00::919e]:9000", "TCP_ECHO_DUAL_PORT_9001_TCP_PROTO=tcp", "KUBERNETES_PORT_443_TCP_PORT=443", "TCP_ECHO_SERVICE_HOST=22.10.83.61", "REDIS_DEMO_SERVICE_PORT=6379", "SLEEP_DUAL_PORT=tcp://22.10.7.100:80", "SLEEP_DUAL_PORT_80_TCP_PROTO=tcp", "NGINX_5_SVC_PORT_80_TCP_PORT=80", "TCP_ECHO_DUAL_PORT_9000_TCP_PORT=9000", "TCP_ECHO_DUAL_PORT_9001_TCP_PORT=9001", "SLEEP_SERVICE_HOST=22.10.36.126", "TCP_ECHO_PORT_9000_TCP_PORT=9000", "TCP_ECHO_PORT_9001_TCP_PROTO=tcp", "NGINX_SVC_2_SERVICE_PORT=80", "NGINX_4_SVC_PORT_80_TCP_PROTO=tcp", "KUBERNETES_SERVICE_PORT=443", "TCP_ECHO_DUAL_PORT_9001_TCP_ADDR=fc00::919e", "TCP_ECHO_PORT_9000_TCP_ADDR=22.10.83.61", "REDIS_DEMO_PORT_6379_TCP=tcp://22.10.44.113:6379", "SLEEP_DUAL_PORT_80_TCP_PORT=80", "SLEEP_DUAL_PORT_80_TCP_ADDR=22.10.7.100", "SLEEP_IPV6_SERVICE_HOST=fc00::35a0", "NGINX_5_SVC_PORT_80_TCP_ADDR=22.10.211.245", "TCP_ECHO_DUAL_PORT=tcp://[fc00::919e]:9000", "SLEEP_PORT_80_TCP=tcp://22.10.36.126:80", "TCP_ECHO_PORT_9000_TCP=tcp://22.10.83.61:9000", "TCP_ECHO_PORT_9001_TCP_ADDR=22.10.83.61", "REDIS_DEMO_PORT=tcp://22.10.44.113:6379", "REDIS_DEMO_PORT_6379_TCP_PORT=6379", "KUBERNETES_SERVICE_HOST=22.10.0.1", "SLEEP_PORT_80_TCP_PROTO=tcp", "TCP_ECHO_PORT_9001_TCP=tcp://22.10.83.61:9001", "REDIS_DEMO_PORT_6379_TCP_PROTO=tcp", "SLEEP_DUAL_SERVICE_HOST=22.10.7.100", "SLEEP_IPV6_PORT_80_TCP_PROTO=tcp", "NGINX_5_SVC_PORT_80_TCP_PROTO=tcp", "SLEEP_PORT_80_TCP_ADDR=22.10.36.126", "NGINX_SVC_2_SERVICE_HOST=22.10.11.161", "NGINX_SVC_2_PORT_80_TCP_PROTO=tcp", "NGINX_4_SVC_PORT_80_TCP_PORT=80", "SLEEP_IPV6_PORT_80_TCP=tcp://[fc00::35a0]:80", "TCP_ECHO_DUAL_SERVICE_PORT_TCP_OTHER=9001", "TCP_ECHO_DUAL_PORT_9001_TCP=tcp://[fc00::919e]:9001", "KUBERNETES_PORT_443_TCP_ADDR=22.10.0.1", "NGINX_SVC_2_PORT_80_TCP_PORT=80", "SLEEP_IPV6_PORT=tcp://[fc00::35a0]:80", "TCP_ECHO_DUAL_SERVICE_HOST=fc00::919e", "KUBERNETES_PORT_443_TCP_PROTO=tcp", "SLEEP_PORT_80_TCP_PORT=80", "TCP_ECHO_PORT_9000_TCP_PROTO=tcp", "NGINX_SVC_2_PORT_80_TCP=tcp://22.10.11.161:80", "SLEEP_DUAL_SERVICE_PORT=80"],
		"cwd": "/",
		"capabilities": {
			"bounding": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"effective": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"permitted": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"]
		},
		"rlimits": [{
			"type": "RLIMIT_NOFILE",
			"hard": 1048576,
			"soft": 1048576
		}],
		"noNewPrivileges": false,
		"oomScoreAdj": 100
	},
	"root": {
		"path": "/run/kata-containers/shared/containers/passthrough/d18a7d0a8630b8fdb2f6bb5739fdbf6206fc5648e15da259f222f356dd640960/rootfs",
		"readonly": false
	},
	"mounts": [{
		"destination": "/proc",
		"type": "proc",
		"source": "proc",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/dev",
		"type": "tmpfs",
		"source": "tmpfs",
		"options": ["nosuid", "strictatime", "mode=755", "size=65536k"]
	}, {
		"destination": "/dev/pts",
		"type": "devpts",
		"source": "devpts",
		"options": ["nosuid", "noexec", "newinstance", "ptmxmode=0666", "mode=0620", "gid=5"]
	}, {
		"destination": "/dev/mqueue",
		"type": "mqueue",
		"source": "mqueue",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/sys",
		"type": "sysfs",
		"source": "sysfs",
		"options": ["nosuid", "noexec", "nodev", "ro"]
	}, {
		"destination": "/sys/fs/cgroup",
		"type": "cgroup",
		"source": "cgroup",
		"options": ["nosuid", "noexec", "nodev", "relatime", "ro"]
	}, {
		"destination": "/etc/hosts",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-5767324-hosts",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/dev/termination-log",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-5767481-termination-log",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/etc/hostname",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-9044150-hostname",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/etc/resolv.conf",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-9044170-resolv.conf",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/dev/shm",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/shm",
		"options": ["rbind"]
	}],
	"hooks": {
		"prestart": [{
			"path": "/opt/guest_hook/prestart/start_nsm_proxy.sh"
		}, {
			"path": "/opt/guest_hook/prestart/fix_arp.sh"
		}],
		"createRuntime": [{
			"path": "/bin/bash",
			"args": ["-c", "! /bin/grep -q \\\"alibabacloud.com/log-destination\\\" config.json || /usr/bin/runtime-agent dump --max-size=10Mi --max-tolerance=500Mi --max-files=5"],
			"env": ["guest_hook=true"],
			"timeout": 10
		}]
	},
	"annotations": {
		"io.kubernetes.cri.sandbox-uid": "13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"io.kubernetes.cri.sandbox-namespace": "default",
		"io.kubernetes.cri.sandbox-name": "busybox-log-stdout",
		"securecontainer.alibabacloud.com/guest-volume-mounts": "{\"logtail\": \"guest_view_volume:/logtail_host\"}",
		"kubernetes.io/config.seen": "2024-01-30T16:01:50.398486661+08:00",
		"io.kubernetes.pod.terminationGracePeriod": "30",
		"io.kubernetes.cri.container-name": "centos",
		"io.kubernetes.container.terminationMessagePolicy": "File",
		"io.kubernetes.cri.sandbox-id": "dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b",
		"io.kubernetes.cri.image-name": "registry-vpc.cn-hangzhou.aliyuncs.com/acs/centos:latest",
		"securecontainer.alibabacloud.com/cpus": "2",
		"alibabacloud.com/security-context": "{\"podSecurityContext\":{}}",
		"securecontainer.alibabacloud.com/memory": "671088640",
		"io.kubernetes.container.restartCount": "1",
		"alibabacloud.com/container-hash-version": "1.24.6",
		"io.kubernetes.container.terminationMessagePath": "/dev/termination-log",
		"kubernetes.io/bandwidth": "{\"request\":\"100M\",\"limit\":\"1G\"}",
		"io.kubernetes.container.hash": "96710e28",
		"io.kubernetes.cri.container-type": "container",
		"io.containerd.image.user": "0",
		"alibabacloud.com/interface-ips": "{\"eth0\":[\"172.16.144.36\"],\"lo\":[\"127.0.0.1\",\"::1\"]}",
		"alibabacloud.com/rich-container-cpu-source": "cpu_quota",
		"io.containerd.image.gids": "0",
		"io.kubernetes.cri.sandbox_oomscoreadj": "99",
		"alibabacloud.com/actual-pod-cgroup-path": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"alibabacloud.com/port-forward-interface": "acs0",
		"kubernetes.io/config.source": "api"
	},
	"linux": {
		"resources": {
			"devices": [{
				"allow": false,
				"type": "b",
				"major": 259,
				"minor": 1,
				"access": "rw"
			}],
			"memory": {
				"limit": 536870912,
				"swap": 536870912,
				"disableOOMKiller": false
			},
			"cpu": {
				"shares": 256,
				"quota": 25000,
				"period": 100000
			},
			"hugepageLimits": [{
				"pageSize": "2MB",
				"limit": 0
			}, {
				"pageSize": "1GB",
				"limit": 0
			}]
		},
		"cgroupsPath": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53/d18a7d0a8630b8fdb2f6bb5739fdbf6206fc5648e15da259f222f356dd640960",
		"namespaces": [{
			"type": "ipc",
			"path": "/var/run/sandbox-ns/ipc"
		}, {
			"type": "uts",
			"path": "/var/run/sandbox-ns/uts"
		}, {
			"type": "mount"
		}, {
			"type": "cgroup"
		}, {
			"type": "pid"
		}],
		"maskedPaths": ["/proc/acpi", "/proc/kcore", "/proc/keys", "/proc/latency_stats", "/proc/timer_list", "/proc/timer_stats", "/proc/sched_debug", "/proc/scsi", "/sys/firmware"],
		"readonlyPaths": ["/proc/asound", "/proc/bus", "/proc/fs", "/proc/irq", "/proc/sys", "/proc/sysrq-trigger"]
	}
}
`

var configJsonLogtail = `{
	"ociVersion": "1.0.2-dev",
	"process": {
		"terminal": false,
		"user": {
			"uid": 0,
			"gid": 0
		},
		"args": ["/usr/local/ilogtail/run_logtail.sh"],
		"env": ["PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", "HOSTNAME=busybox-log-stdout", "ALIYUN_LOG_ENV_TAGS=_pod_name_|_pod_ip_|_namespace_", "_pod_name_=busybox-log-stdout", "_pod_ip_=172.16.144.36", "_namespace_=", "SIGMA_APP_SITE=", "ALIYUN_LOGTAIL_USER_ID=1586306209475141", "ALIYUN_LOGTAIL_USER_DEFINED_ID=k8s-group-cb68353c02e4b476f92490746d1ca7f65", "ALIYUN_LOGTAIL_CONFIG=/etc/ilogtail/conf/cn-hangzhou/ilogtail_config.json", "REDIS_DEMO_PORT_6379_TCP_ADDR=22.10.44.113", "SLEEP_DUAL_PORT=tcp://22.10.7.100:80", "SLEEP_IPV6_PORT_80_TCP_PROTO=tcp", "TCP_ECHO_DUAL_PORT_9000_TCP_PORT=9000", "TCP_ECHO_DUAL_PORT_9001_TCP_ADDR=fc00::919e", "KUBERNETES_SERVICE_PORT_HTTPS=443", "SLEEP_PORT_80_TCP_ADDR=22.10.36.126", "TCP_ECHO_PORT_9001_TCP_ADDR=22.10.83.61", "SLEEP_IPV6_PORT_80_TCP_ADDR=fc00::35a0", "NGINX_5_SVC_PORT_80_TCP_PROTO=tcp", "TCP_ECHO_DUAL_PORT_9001_TCP=tcp://[fc00::919e]:9001", "TCP_ECHO_DUAL_PORT_9001_TCP_PORT=9001", "TCP_ECHO_SERVICE_PORT_TCP=9000", "SLEEP_DUAL_PORT_80_TCP_PROTO=tcp", "SLEEP_DUAL_PORT_80_TCP_ADDR=22.10.7.100", "TCP_ECHO_DUAL_PORT_9000_TCP_PROTO=tcp", "KUBERNETES_PORT_443_TCP=tcp://22.10.0.1:443", "SLEEP_SERVICE_PORT_HTTP=80", "SLEEP_PORT=tcp://22.10.36.126:80", "NGINX_4_SVC_PORT_80_TCP_PORT=80", "NGINX_5_SVC_SERVICE_HOST=22.10.211.245", "NGINX_5_SVC_PORT_80_TCP_ADDR=22.10.211.245", "NGINX_4_SVC_PORT_80_TCP=tcp://22.10.187.126:80", "REDIS_DEMO_SERVICE_PORT_REDIS_PORT=6379", "NGINX_SVC_2_SERVICE_HOST=22.10.11.161", "NGINX_SVC_2_PORT_80_TCP_ADDR=22.10.11.161", "KUBERNETES_PORT_443_TCP_ADDR=22.10.0.1", "SLEEP_SERVICE_HOST=22.10.36.126", "REDIS_DEMO_SERVICE_HOST=22.10.44.113", "SLEEP_IPV6_SERVICE_PORT_HTTP=80", "SLEEP_IPV6_PORT_80_TCP_PORT=80", "NGINX_5_SVC_PORT_80_TCP=tcp://22.10.211.245:80", "NGINX_4_SVC_SERVICE_HOST=22.10.187.126", "SLEEP_DUAL_SERVICE_HOST=22.10.7.100", "SLEEP_DUAL_SERVICE_PORT=80", "SLEEP_DUAL_PORT_80_TCP_PORT=80", "TCP_ECHO_DUAL_PORT_9001_TCP_PROTO=tcp", "SLEEP_PORT_80_TCP_PROTO=tcp", "REDIS_DEMO_PORT_6379_TCP=tcp://22.10.44.113:6379", "NGINX_4_SVC_PORT=tcp://22.10.187.126:80", "SLEEP_DUAL_SERVICE_PORT_HTTP=80", "SLEEP_DUAL_PORT_80_TCP=tcp://22.10.7.100:80", "SLEEP_IPV6_PORT=tcp://[fc00::35a0]:80", "TCP_ECHO_DUAL_PORT_9000_TCP=tcp://[fc00::919e]:9000", "SLEEP_PORT_80_TCP=tcp://22.10.36.126:80", "TCP_ECHO_PORT_9000_TCP_PROTO=tcp", "TCP_ECHO_PORT_9000_TCP_PORT=9000", "NGINX_4_SVC_SERVICE_PORT=80", "KUBERNETES_SERVICE_PORT=443", "TCP_ECHO_PORT=tcp://22.10.83.61:9000", "REDIS_DEMO_PORT_6379_TCP_PORT=6379", "NGINX_SVC_2_PORT=tcp://22.10.11.161:80", "TCP_ECHO_DUAL_SERVICE_HOST=fc00::919e", "TCP_ECHO_DUAL_PORT=tcp://[fc00::919e]:9000", "KUBERNETES_SERVICE_HOST=22.10.0.1", "TCP_ECHO_PORT_9000_TCP=tcp://22.10.83.61:9000", "TCP_ECHO_PORT_9001_TCP_PROTO=tcp", "TCP_ECHO_PORT_9001_TCP=tcp://22.10.83.61:9001", "NGINX_SVC_2_PORT_80_TCP_PORT=80", "KUBERNETES_PORT=tcp://22.10.0.1:443", "KUBERNETES_PORT_443_TCP_PORT=443", "TCP_ECHO_SERVICE_HOST=22.10.83.61", "NGINX_SVC_2_PORT_80_TCP=tcp://22.10.11.161:80", "NGINX_5_SVC_PORT_80_TCP_PORT=80", "NGINX_4_SVC_PORT_80_TCP_ADDR=22.10.187.126", "KUBERNETES_PORT_443_TCP_PROTO=tcp", "TCP_ECHO_PORT_9001_TCP_PORT=9001", "REDIS_DEMO_PORT_6379_TCP_PROTO=tcp", "REDIS_DEMO_PORT=tcp://22.10.44.113:6379", "SLEEP_IPV6_SERVICE_HOST=fc00::35a0", "TCP_ECHO_DUAL_SERVICE_PORT=9000", "TCP_ECHO_DUAL_SERVICE_PORT_TCP=9000", "TCP_ECHO_DUAL_PORT_9000_TCP_ADDR=fc00::919e", "SLEEP_SERVICE_PORT=80", "TCP_ECHO_SERVICE_PORT=9000", "REDIS_DEMO_SERVICE_PORT=6379", "NGINX_4_SVC_PORT_80_TCP_PROTO=tcp", "TCP_ECHO_DUAL_SERVICE_PORT_TCP_OTHER=9001", "SLEEP_PORT_80_TCP_PORT=80", "TCP_ECHO_SERVICE_PORT_TCP_OTHER=9001", "SLEEP_IPV6_PORT_80_TCP=tcp://[fc00::35a0]:80", "SLEEP_IPV6_SERVICE_PORT=80", "NGINX_5_SVC_SERVICE_PORT=80", "NGINX_5_SVC_PORT=tcp://22.10.211.245:80", "TCP_ECHO_PORT_9000_TCP_ADDR=22.10.83.61", "NGINX_SVC_2_SERVICE_PORT=80", "NGINX_SVC_2_PORT_80_TCP_PROTO=tcp"],
		"cwd": "/",
		"capabilities": {
			"bounding": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"effective": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"permitted": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"]
		},
		"rlimits": [{
			"type": "RLIMIT_NOFILE",
			"hard": 1048576,
			"soft": 1048576
		}],
		"noNewPrivileges": false,
		"oomScoreAdj": 100
	},
	"root": {
		"path": "/run/kata-containers/shared/containers/passthrough/606e6bbea39af42f06601933eb4c344edb770e647b42c9dada19f7f65e3f4501/rootfs",
		"readonly": false
	},
	"mounts": [{
		"destination": "/proc",
		"type": "proc",
		"source": "proc",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/dev",
		"type": "tmpfs",
		"source": "tmpfs",
		"options": ["nosuid", "strictatime", "mode=755", "size=65536k"]
	}, {
		"destination": "/dev/pts",
		"type": "devpts",
		"source": "devpts",
		"options": ["nosuid", "noexec", "newinstance", "ptmxmode=0666", "mode=0620", "gid=5"]
	}, {
		"destination": "/dev/mqueue",
		"type": "mqueue",
		"source": "mqueue",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/sys",
		"type": "sysfs",
		"source": "sysfs",
		"options": ["nosuid", "noexec", "nodev", "ro"]
	}, {
		"destination": "/sys/fs/cgroup",
		"type": "cgroup",
		"source": "cgroup",
		"options": ["nosuid", "noexec", "nodev", "relatime", "ro"]
	}, {
		"destination": "/stdlog",
		"type": "bind",
		"source": "/run/builtin_wlayer/emptydir/stdlog",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/etc/hosts",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-5767324-hosts",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/dev/termination-log",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-5767480-termination-log",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/etc/hostname",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-9044150-hostname",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/etc/resolv.conf",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-9044170-resolv.conf",
		"options": ["rbind", "rprivate", "rw"]
	}, {
		"destination": "/dev/shm",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/shm",
		"options": ["rbind"]
	}, {
		"destination": "/logtail_host",
		"type": "bind",
		"source": "/run/kata-containers/volumes/guest-view-volume/dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b",
		"options": ["rbind", "rslave", "ro"]
	}],
	"hooks": {
		"prestart": [{
			"path": "/opt/guest_hook/prestart/start_nsm_proxy.sh"
		}, {
			"path": "/opt/guest_hook/prestart/fix_arp.sh"
		}],
		"createRuntime": [{
			"path": "/bin/bash",
			"args": ["-c", "! /bin/grep -q \\\"alibabacloud.com/log-destination\\\" config.json || /usr/bin/runtime-agent dump --max-size=10Mi --max-tolerance=500Mi --max-files=5"],
			"env": ["guest_hook=true"],
			"timeout": 10
		}]
	},
	"annotations": {
		"securecontainer.alibabacloud.com/guest-volume-mounts": "{\"logtail\": \"guest_view_volume:/logtail_host\"}",
		"alibabacloud.com/security-context": "{\"podSecurityContext\":{}}",
		"io.kubernetes.cri.sandbox-uid": "13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"io.containerd.image.gids": "0",
		"securecontainer.alibabacloud.com/cpus": "2",
		"io.kubernetes.cri.sandbox-namespace": "default",
		"io.kubernetes.container.terminationMessagePolicy": "File",
		"io.kubernetes.cri.sandbox-id": "dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b",
		"alibabacloud.com/rich-container-cpu-source": "cpu_quota",
		"io.kubernetes.cri.sandbox_oomscoreadj": "99",
		"io.kubernetes.pod.terminationGracePeriod": "30",
		"kubernetes.io/config.source": "api",
		"io.kubernetes.container.terminationMessagePath": "/dev/termination-log",
		"alibabacloud.com/actual-pod-cgroup-path": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"io.kubernetes.container.restartCount": "1",
		"alibabacloud.com/interface-ips": "{\"eth0\":[\"172.16.144.36\"],\"lo\":[\"127.0.0.1\",\"::1\"]}",
		"io.kubernetes.container.hash": "f48f4a9",
		"alibabacloud.com/container-hash-version": "1.24.6",
		"io.kubernetes.cri.container-type": "container",
		"kubernetes.io/bandwidth": "{\"request\":\"100M\",\"limit\":\"1G\"}",
		"alibabacloud.com/port-forward-interface": "acs0",
		"io.containerd.image.user": "0",
		"io.kubernetes.cri.container-name": "logtail",
		"io.kubernetes.cri.sandbox-name": "busybox-log-stdout",
		"io.kubernetes.cri.image-name": "asi-acr-ee-registry-vpc.cn-hangzhou.cr.aliyuncs.com/asi/logtail:v1.8.4.0-aliyun",
		"securecontainer.alibabacloud.com/memory": "671088640",
		"kubernetes.io/config.seen": "2024-01-30T16:01:50.398486661+08:00"
	},
	"linux": {
		"resources": {
			"devices": [{
				"allow": false,
				"type": "b",
				"major": 259,
				"minor": 1,
				"access": "rw"
			}],
			"memory": {
				"limit": 402653184,
				"swap": 402653184,
				"disableOOMKiller": false
			},
			"cpu": {
				"shares": 256,
				"quota": 25000,
				"period": 100000
			},
			"hugepageLimits": [{
				"pageSize": "2MB",
				"limit": 0
			}, {
				"pageSize": "1GB",
				"limit": 0
			}]
		},
		"cgroupsPath": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53/606e6bbea39af42f06601933eb4c344edb770e647b42c9dada19f7f65e3f4501",
		"namespaces": [{
			"type": "ipc",
			"path": "/var/run/sandbox-ns/ipc"
		}, {
			"type": "uts",
			"path": "/var/run/sandbox-ns/uts"
		}, {
			"type": "mount"
		}, {
			"type": "cgroup"
		}, {
			"type": "pid"
		}],
		"maskedPaths": ["/proc/acpi", "/proc/kcore", "/proc/keys", "/proc/latency_stats", "/proc/timer_list", "/proc/timer_stats", "/proc/sched_debug", "/proc/scsi", "/sys/firmware"],
		"readonlyPaths": ["/proc/asound", "/proc/bus", "/proc/fs", "/proc/irq", "/proc/sys", "/proc/sysrq-trigger"]
	}
}`

var configJsonSandBox = `{
	"ociVersion": "1.0.2-dev",
	"process": {
		"terminal": false,
		"user": {
			"uid": 0,
			"gid": 0
		},
		"args": ["/pause"],
		"env": ["PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"],
		"cwd": "/",
		"capabilities": {
			"bounding": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"effective": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"],
			"permitted": ["CAP_CHOWN", "CAP_DAC_OVERRIDE", "CAP_FSETID", "CAP_FOWNER", "CAP_MKNOD", "CAP_NET_RAW", "CAP_SETGID", "CAP_SETUID", "CAP_SETFCAP", "CAP_SETPCAP", "CAP_NET_BIND_SERVICE", "CAP_SYS_CHROOT", "CAP_KILL", "CAP_AUDIT_WRITE"]
		},
		"rlimits": [{
			"type": "RLIMIT_NOFILE",
			"hard": 1048576,
			"soft": 1048576
		}],
		"noNewPrivileges": true,
		"oomScoreAdj": 99
	},
	"root": {
		"path": "/run/kata-containers/shared/containers/passthrough/dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b/rootfs",
		"readonly": true
	},
	"hostname": "busybox-log-stdout",
	"mounts": [{
		"destination": "/proc",
		"type": "proc",
		"source": "proc",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/dev",
		"type": "tmpfs",
		"source": "tmpfs",
		"options": ["nosuid", "strictatime", "mode=755", "size=65536k"]
	}, {
		"destination": "/dev/pts",
		"type": "devpts",
		"source": "devpts",
		"options": ["nosuid", "noexec", "newinstance", "ptmxmode=0666", "mode=0620", "gid=5"]
	}, {
		"destination": "/dev/shm",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/shm",
		"options": ["rbind"]
	}, {
		"destination": "/dev/mqueue",
		"type": "mqueue",
		"source": "mqueue",
		"options": ["nosuid", "noexec", "nodev"]
	}, {
		"destination": "/sys",
		"type": "sysfs",
		"source": "sysfs",
		"options": ["nosuid", "noexec", "nodev", "ro"]
	}, {
		"destination": "/dev/shm",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/shm",
		"options": ["rbind"]
	}, {
		"destination": "/etc/resolv.conf",
		"type": "bind",
		"source": "/run/kata-containers/sandbox/66308-9044170-resolv.conf",
		"options": ["rbind", "ro"]
	}],
	"hooks": {
		"prestart": [{
			"path": "/opt/guest_hook/prestart/start_nsm_proxy.sh"
		}, {
			"path": "/opt/guest_hook/prestart/fix_arp.sh"
		}]
	},
	"annotations": {
		"alibabacloud.com/interface-ips": "{\"eth0\":[\"172.16.144.36\"],\"lo\":[\"127.0.0.1\",\"::1\"]}",
		"io.kubernetes.cri.sandbox-memory": "939524096",
		"io.kubernetes.cri.sandbox-name": "busybox-log-stdout",
		"alibabacloud.com/enable-host-custom-storage": "true",
		"alibabacloud.com/port-forward-interface": "acs0",
		"io.kubernetes.cri.sandbox-cpu-period": "100000",
		"io.kubernetes.cri.sandbox-cpu-quota": "50000",
		"securecontainer.alibabacloud.com/guest-volume-mounts": "{\"logtail\": \"guest_view_volume:/logtail_host\"}",
		"kubernetes.io/bandwidth": "{\"request\":\"100M\",\"limit\":\"1G\"}",
		"securecontainer.alibabacloud.com/cpus": "2",
		"io.kubernetes.cri.sandbox-namespace": "default",
		"io.kubernetes.cri.sandbox-cpu-shares": "256",
		"io.kubernetes.cri.sandbox_oomscoreadj": "99",
		"io.kubernetes.cri.sandbox-uid": "13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"securecontainer.alibabacloud.com/memory": "671088640",
		"io.kubernetes.cri.container-type": "sandbox",
		"io.kubernetes.cri.sandbox-log-directory": "/var/log/pods/default_busybox-log-stdout_13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"kubernetes.io/config.seen": "2024-01-30T16:01:50.398486661+08:00",
		"alibabacloud.com/actual-pod-cgroup-path": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53",
		"kubernetes.io/config.source": "api",
		"alibabacloud.com/rich-container-cpu-source": "cpu_quota",
		"io.kubernetes.cri.sandbox-id": "dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b",
		"alibabacloud.com/security-context": "{\"podSecurityContext\":{}}"
	},
	"linux": {
		"resources": {
			"devices": [{
				"allow": false,
				"type": "b",
				"major": 259,
				"minor": 1,
				"access": "rw"
			}],
			"cpu": {
				"shares": 2,
				"quota": 0
			}
		},
		"cgroupsPath": "/kubepods/burstable/pod13096fc4-1a35-4d52-965f-4c8c0151bf53/dd7af61094f33b5e0a452a65a4953d9ff976a96298b3dfdf8b3d9c9dc39a6e9b",
		"namespaces": [{
			"type": "ipc",
			"path": "/var/run/sandbox-ns/ipc"
		}, {
			"type": "uts",
			"path": "/var/run/sandbox-ns/uts"
		}, {
			"type": "mount"
		}, {
			"type": "pid"
		}],
		"maskedPaths": ["/proc/acpi", "/proc/asound", "/proc/kcore", "/proc/keys", "/proc/latency_stats", "/proc/timer_list", "/proc/timer_stats", "/proc/sched_debug", "/sys/firmware", "/proc/scsi"],
		"readonlyPaths": ["/proc/bus", "/proc/fs", "/proc/irq", "/proc/sys", "/proc/sysrq-trigger"]
	}
}`

var podInfoPodName = "demo-log-stdout"

var podInfoPodNamespace = "default"

var podInfoPodLabels = `alibabacloud.com/runtime-class-name="rund"
app="taiye-demo"
labeltest="test"
taiye="test1"`

func TestTryReadACSStaticContainerInfo(t *testing.T) {
	defer os.Remove("./etc")
	defer os.Remove("./bundle")

	defer os.Unsetenv(acsStaticContainerInfoMountPathEnvKey)
	defer os.Unsetenv(acsPodInfoMountPathEnvKey)

	os.Mkdir("./bundle", os.ModePerm)
	os.Mkdir("./bundle/main", os.ModePerm)
	os.Mkdir("./bundle/logtail", os.ModePerm)
	os.Mkdir("./bundle/sandbox", os.ModePerm)

	os.Mkdir("./etc", os.ModePerm)
	os.Mkdir("./etc/podinfo", os.ModePerm)

	os.WriteFile("./bundle/main/config.json", []byte(configJsonMain), os.ModePerm)
	os.WriteFile("./bundle/logtail/config.json", []byte(configJsonLogtail), os.ModePerm)
	os.WriteFile("./bundle/sandbox/config.json", []byte(configJsonSandBox), os.ModePerm)

	os.WriteFile("./etc/podinfo/namespace", []byte(podInfoPodNamespace), os.ModePerm)
	os.WriteFile("./etc/podinfo/name", []byte(podInfoPodName), os.ModePerm)
	os.WriteFile("./etc/podinfo/labels", []byte(podInfoPodLabels), os.ModePerm)

	os.Setenv(acsStaticContainerInfoMountPathEnvKey, "./bundle")
	os.Setenv(acsPodInfoMountPathEnvKey, "./etc/podinfo")

	containerInfo, removedIDs, changed, err := tryReadACSStaticContainerInfo()
	require.Nil(t, err)
	require.Len(t, containerInfo, 1)
	require.Empty(t, removedIDs)
	require.True(t, changed)

	containerInfo, removedIDs, changed, err = tryReadACSStaticContainerInfo()
	require.Nil(t, err)
	require.Len(t, containerInfo, 1)
	require.Empty(t, removedIDs)
	require.False(t, changed)
}
