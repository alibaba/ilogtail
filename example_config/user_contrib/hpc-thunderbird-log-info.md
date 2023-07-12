# Thunderbird-HPC日志

## 提供者
[`Jiang-Weibo`](https://github.com/Jiang-Weibo)

## 描述
超级计算机每天将会收到来自世界各地的计算请求来完成大量的复杂与模拟工作，其应用领域包括物理模拟、天气预测、材料科学与计算生物学等等。在运行期间，超级计算机会产生海量的日志文件，其主要内容包括任务状态，用户状态等等，而分析其日志报告对于提升任务调度效率与数据研究工作有非常大的帮助。

该日志数据来自美国新墨西哥州Albuquerqueat的Sandia National Labs，而Thunderbird是属于该实验室的超级计算机系统，该系统属于NNSA(National Nuclear Security)与ASC(Administration's  Advanced Simulation and Computing)项目的一部分。数据来源[`loghub`](https://github.com/logpai/loghub)，其日志由多种软件和系统层机制共同管理，具体相关内容属于保密信息，推测部分日志来自于SLURM与特定应用程序内部输出。该数据的日志的信息较为丰富，所选取的几条日志可粗略分为以下六个字段

* 编号（seq_num），为每一个计算任务编号。
* 日期（date），标记日志/计算的时间，格式为`YYYY.MM.DD`。
* 用户名(usr)，记录该任务属于哪个用户。
* 时间（time），记录详细时间，举例`Sep 9 12:23:22`。
* 地点（location），记录该条日志由哪台超算，在哪条指令时被记录，例如`src@aadmin1 dhcpd`
* 详细信息（details），记录更为详细的内容（content）。

## 日志输入样例
```
1131566510 2005.11.09 tbird-admin1 Nov 9 12:01:50 local@tbird-admin1 /apps/x86_64/system/ganglia-3.0.1/sbin/gmetad[1682]: data_thread() got not answer from any [Thunderbird_A6] datasource
1131566491 2005.11.09 eadmin1 Nov 9 12:01:31 src@eadmin1 crond(pam_unix)[1205]: session closed for user root
1131566495 2005.11.09 tbird-admin1 Nov 9 12:01:35 local@tbird-admin1 /apps/x86_64/system/ganglia-3.0.1/sbin/gmetad[1682]: data_thread() got not answer from any [Thunderbird_C5] datasource
1131566501 2005.11.09 aadmin1 Nov 9 12:01:41 src@aadmin1 dhcpd: DHCPDISCOVER from 00:11:43:e3:ba:c3 via eth1
1131566502 2005.11.09 tbird-sm1 Nov 9 12:01:42 src@tbird-sm1 ib_sm.x[24904]: [ib_sm_sweep.c:1482]: No configuration change required
```

## 日志输出样例
```
recover stdout
2023-07-04 16:39:50 {"__tag__:__path__":"./simple.log","seq_num":"1131566510","date":"2005.11.09","usr":"tbird-admin1","time":"Nov 9 12:01:50","location":"local@tbird-admin1 /apps/x86_64/system/ganglia-3.0.1/sbin/gmetad[1682]:","details":"data_thread() got not answer from any [Thunderbird_A6] datasource","__time__":"1688459987"}
2023-07-04 16:40:26 {"__tag__:__path__":"./simple.log","seq_num":"1131566491","date":"2005.11.09","usr":"eadmin1","time":"Nov 9 12:01:31","location":"src@eadmin1 crond(pam_unix)[1205]:","details":"session closed for user root","__time__":"1688460023"}
2023-07-04 16:40:41 {"__tag__:__path__":"./simple.log","seq_num":"1131566495","date":"2005.11.09","usr":"tbird-admin1","time":"Nov 9 12:01:35","location":"local@tbird-admin1 /apps/x86_64/system/ganglia-3.0.1/sbin/gmetad[1682]:","details":"data_thread() got not answer from any [Thunderbird_C5] datasource","__time__":"1688460040"}
2023-07-04 16:40:59 {"__tag__:__path__":"./simple.log","seq_num":"1131566501","date":"2005.11.09","usr":"aadmin1","time":"Nov 9 12:01:41","location":"src@aadmin1 dhcpd:","details":"DHCPDISCOVER from 00:11:43:e3:ba:c3 via eth1","__time__":"1688460058"}
2023-07-04 16:42:11 {"__tag__:__path__":"./simple.log","seq_num":"1131566502","date":"2005.11.09","usr":"tbird-sm1","time":"Nov 9 12:01:42","location":"src@tbird-sm1 ib_sm.x[24904]: [ib_sm_sweep.c:1482]:","details":"No configuration change required","__time__":"1688460130"}
```

## 采集配置
```
enable: true
inputs:
  - Type: file_log          # 文件输入类型
    LogPath: .              # 文件路径Glob匹配规则
    FilePattern: simple.log # 文件名Glob匹配规则
processors:
  - Type: processor_regex
    SourceKey: content
    Regex: ([\d]+) ([\d\.]+) (.+) ([\w]+ [\d] [\d]+:[\d]+:[\d]+) (.+:) (.*)
    Keys:
    #日期，用户名，时间，地点（机器/代码），详细信息
      - seq_num
      - date
      - usr
      - time
      - location
      - details
flushers:
  - Type: flusher_stdout    # 标准输出流输出类型
    OnlyStdout: true
```