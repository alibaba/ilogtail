# Linux /var/log/auth.log日志

## 提供者

[mattquminzhi](https://github.com/quminzhi)

## 描述

用户登录日志可以通过`journalctl | grep
sshd`或直接访问日志文件`sudo cat /var/log/auth.log`获得，用户退出登录时，用户登录信息会被记录在系统日志中。为了更好的解析日志，我们只记录包含`Disconnected from`的用户登录记录，如`ssh myserver 'journalctl | grep sshd | grep "Disconnected from"' | less`。

```shell
# ssh系统日志 /var/log/auth.log
Jul 11 17:40:20 ip-172-31-22-108 sshd[321861]: Failed password for root from 124.223.98.89 port 52178 ssh2
Jul 11 17:40:22 ip-172-31-22-108 sshd[321861]: Connection closed by authenticating user root 124.223.98.89 port 52178 [preauth]
Jul 11 17:40:23 ip-172-31-22-108 sshd[321867]: pam_unix(sshd:auth): authentication failure; logname= uid=0 euid=0 tty=ssh ruser= rhost=124.223.98.89  user=root
Jul 11 17:40:24 ip-172-31-22-108 sudo:    admin : TTY=pts/0 ; PWD=/home/admin ; USER=root ; COMMAND=/usr/bin/cat /var/log/auth.log
Jul 11 17:40:24 ip-172-31-22-108 sudo: pam_unix(sudo:session): session opened for user root(uid=0) by admin(uid=1000)
Jul 11 17:40:25 ip-172-31-22-108 sshd[321867]: Failed password for root from 124.223.98.89 port 53430 ssh2
Jul 11 17:40:25 ip-172-31-22-108 sshd[321867]: Connection closed by authenticating user root 124.223.98.89 port 53430 [preauth]

# 使用processer:processor_filter_regex过滤后的记录
Jan 17 03:13:00 thesquareplanet.com sshd[2631]: Disconnected from user Disconnected from 46.97.239.16 port 55920
```

## 日志输入样例

```shell
May 22 20:45:55 ip-172-31-22-108 sshd[13331]: Disconnected from user admin 205.175.106.86 port 50718
May 22 20:45:55 ip-172-31-22-108 sshd[13331]: Disconnected from user admin 205.175.106.86 port 50718
May 22 20:45:55 ip-172-31-22-108 sshd[13331]: Disconnected from user admin 205.175.106.86 port 50718
```

## 日志输出样例

```shell
2023-07-11 17:59:26 {"__tag__:__path__":"/var/log/auth.log","time":"May 22 20:14:36","user":"172-31-22-108","ipv4_from":"admin","on_port":"205.175.106.86","__time__":"1689098366"}
2023-07-11 17:59:26 {"__tag__:__path__":"/var/log/auth.log","time":"May 22 20:17:21","user":"172-31-22-108","ipv4_from":"admin","on_port":"205.175.106.86","__time__":"1689098366"}
2023-07-11 17:59:26 {"__tag__:__path__":"/var/log/auth.log","time":"May 22 20:17:44","user":"172-31-22-108","ipv4_from":"admin","on_port":"205.175.106.86","__time__":"1689098366"}
```

## 采集配置

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /var/log
    FilePattern: auth.log.*
    MaxDepth: 0
processors:
  - Type: processor_filter_regex
    Include:
      content: '^\w{3}\s\d+*Disconnected from.*$'
  - Type: processor_regex
    SourceKey: content
    Regex: '^(\w{3}\s\d{2}\s\d{2}:\d{2}:\d{2})\sip-(\d+-\d+-\d+-\d+)\s.*Disconnected\sfrom\suser\s([^\s]+)\s([^\s]+)\sport\s(\d+)'
    Keys: ["time", "user", "ipv4_from", "on_port"]
flushers:
  - Type: flusher_stdout
    FileName: ./sshd_login.out.log
```

备注：`/var/log/auth.log`需要sudo权限以访问。
