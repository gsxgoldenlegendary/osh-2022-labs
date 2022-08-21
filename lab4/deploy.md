# Ceph 部署文档

## 实验环境

- Host-OS：Ubuntu 20.04.4 LTS x86_64
- Host：HP Pavilion Aero Laptop 13-be0152AU
- Kernel：5.13.0-48-generic
- CPU：AMD Ryzen 7 5800U with Radeon Graphics (16) @ 1.900GHz
- GPU：AMD ATI 03:00.0 Device 1638
- VM：VMware Workstation 16.2.3 build-19376536
- VM-OS：CentOS 7

## 环境配置

### 安装 VMware

### 在 VMware 下安装 CentOS

- 增加硬盘：在 VMware 的 Devices 栏目中，新建设备，添加 SCSI 与 NVMe 硬盘各一块。

- 配置网卡：打开 Virtual Network Editor，将 vmnet8 的 IP 地址改为 192.168.153.1。在 VMware 的 Devices 栏目中，新建设备，添加一个选择 vmnet8 的网卡。

- 配置网络：

  ```bash
  cd /etc/sysconfig/network-scripts/
  sudo vi ifcfg-ens33
  ```

  将 `ONBOOT=no` 修改为 `ONBOOT=yes`，IPV4 与 DNS 服务器地址改为 192.168.31.10。

  类似地将 `ifcfg-ens35` 文件中的 `ONBOOT=no` 修改为 `ONBOOT=yes` ，IPV4 与 DNS 服务器地址改为 192.168.153.10。

- 重新启动网络服务：

  ```bash
  systemctl restart network
  ```

  使用 VMware 的 clone 方式创建 3 台与之相同的虚拟机，并将其IP地址最后一段分别改为 11，12，13。

### SSH 免密登录

client 节点作为 deploy 节点，在 client 节点执行

```bash
ssh-keygen
ssh-copy-id -i ~/.ssh/id_rsa.pub root@192.168.153.11
ssh-copy-id -i ~/.ssh/id_rsa.pub root@192.168.153.12
ssh-copy-id -i ~/.ssh/id_rsa.pub root@192.168.153.13
```

### 配置本地主机名解析

在client节点执行

```bash
echo "192.168.31.10  client client.localdomain" >> /etc/hosts
echo "192.168.31.11	 node1 node1.localdomain" >> /etc/hosts
echo "192.168.31.12  node2 node2.localdomain" >> /etc/hosts
echo "192.168.31.13  node3 node3.localdomain" >> /etc/hosts
scp /etc/hosts node1:/etc/hosts
scp /etc/hosts node2:/etc/hosts
scp /etc/hosts node3:/etc/hosts
```

### 配置安全策略

- 在 client，node1，node2，node3 节点上都关闭 selinux：

  ```bash
  vi /etc/selinux/config
  ```

  将 `SELINUX` 值设置为 `disabled`。

- 在 client，node1，node2，node3 节点上都关闭防火墙：

  ```bash
  systemctl stop firewalld
  systemctl disable firewalld
  ```

### 配置 ntp 时间同步

- client 节点作为 ntp server，在 client、node1、node2、node3 上安装 ntp。

  ```bash
  yum install ntp -y
  ```

- 在 node1、node2、node3 上配置 ntp，添加 client 节点作为时间服务器。

  ```bash
  vi /etc/ntp.conf
  ```

  写入server client iburst。

- 开启 ntp 服务

  ```bash
  systemctl start ntpd
  ```

- 确认 ntp 服务器指向了 client 节点

  ```bash
  ntpq -pn
  ```

### 配置 yum 源

- 添加 CentOS7 的 yum 源

  ```bash
   sed -e 's|^mirrorlist=|#mirrorlist=|g' \
           -e 's|^#baseurl=http://mirror.centos.org/centos|baseurl=https://mirrors.ustc.edu.cn/centos|g' \
           -i.bak \
           /etc/yum.repos.d/CentOS-Base.repo
  ```

- 添加 epel 的 yum 源

  ```bash
  sudo yum install -y epel-release
  sudo sed -e 's|^metalink=|#metalink=|g' \
           -e 's|^#baseurl=https\?://download.fedoraproject.org/pub/epel/|baseurl=https://mirrors.ustc.edu.cn/epel/|g' \
           -e 's|^#baseurl=https\?://download.example/pub/epel/|baseurl=https://mirrors.ustc.edu.cn/epel/|g' \
           -i.bak \
           /etc/yum.repos.d/epel.repo
  ```

- 添加 ceph 的 yum 源

  ```bash
  vi /etc/yum.repos.d/ceph.repo
  ```

  ```bash
  [ceph_noarch]
  name=noarch
  baseurl=https://mirrors.ustc.edu.cn/ceph/rpm-nautilus/el7/noarch/
  enabled=1
  gpgcheck=0
  [ceph_x86_64]
  name=x86_64
  baseurl=https://mirrors.ustc.edu.cn/ceph/rpm-nautilus/el7/x86_64/
  enabled=1
  gpgcheck=0
  ```

## 软件安装

### ceph 相关的包的安装

- 在部署节点（client）安装 ceph 的部署工具

  ```bash
  yum install python-setuptools -y
  yum install ceph-deploy -y
  ```

- 确保 ceph-deploy 的版本是 2.0.1

  ```bash
  ceph-deploy --version
  ```

- 在 node1、node2、node3 执行下面命令，安装 ceph 相关的包

  ```bash
  yum install -y ceph ceph-mon ceph-osd ceph-mds ceph-radosgw ceph-mgr
  ```

### 部署 monitor

- node1 作为 monitor 节点，在部署结点（client）创建一个工作目录，后续的命令在该目录下执行，产生的配置文件保存在该目录中。

  ```bash
  mkdir ~/my-cluster
  cd ~/my-cluster
  ceph-deploy new --public-network 192.168.31.0/24 --cluster-network 192.168.153.0/24 node1
  ```

- 初始化 monitor

  ```bash
  ceph-deploy mon create-initial
  ```

- 将配置文件复制到对应的节点

  ```bash
  ceph-deploy admin node1 node2 node3
  ```

- 部署高可用 monitor，将 node2、node3 也加入 mon 集群。

  ```bash
  ceph-deploy mon add node2
  ceph-deploy mon add node3
  ```

### 部署 mgr

- node1 作为 mgr 节点，在部署节点（client）执行

  ```bash
  cd my-cluster
  ceph-deploy mgr create node1
  ```

- 部署高可用 mgr，将 node2、node3 也添加进来

  ```bash
  ceph-deploy mgr create node2 node3
  ```

### 部署 osd

- 确认每个节点的硬盘情况

  ```bash
  ceph-deploy disk list node1 node2 node3
  ```

- 清理 node1、node2、node3 节点上的硬盘里的现有数据和文件系统

  ```bash
  ceph-deploy disk zap node1 /dev/sdb
  ceph-deploy disk zap node2 /dev/sdb
  ceph-deploy disk zap node3 /dev/sdb
  ceph-deploy disk zap node1 /dev/nvme0n1
  ceph-deploy disk zap node2 /dev/nvme0n1
  ceph-deploy disk zap node3 /dev/nvme0n1
  ```

- 添加 OSD

  ```bash
  ceph-deploy osd create --data /dev/sdb --journal /dev/nvme0n1 --filestore node1
  ceph-deploy osd create --data /dev/sdb --journal /dev/nvme0n1 --filestore node2
  ceph-deploy osd create --data /dev/sdb --journal /dev/nvme0n1 --filestore node3
  ```

## 建立存储

### 使用 systemd 管理 ceph 服务

- 列出所有的 ceph 服务

  ```bash
  systemctl status ceph\*.service ceph\*.target
  ```

- 启动所有的服务守护程序

  ```bash
  systemctl start ceph.target
  ```

- 停止所有服务的守护程序

  ```bash
  systemctl stop ceph.target
  ```

- 按服务类型启动所有守护进程

  ```bash
  systemctl start ceph-osd target
  systemctl start ceph-mon target
  systemctl start ceph-mds target
  ```

- 按服务类型停止所有守护进程

  ```bash
  systemctl stop ceph-osd target
  systemctl stop ceph-mon target
  systemctl stop ceph-mds target
  ```

### 创建存储池

- 列出已经创建的存储池

  ```bash
  ceph osd lspools
  ceph osd pool ls
  ```

- 创建存储池

  ```bash
  ceph osd pool create test 64 64
  ```

- 重命名存储池

  ```bash
  ceph osd pool rename test ceph
  ```

### 查看存储池属性

- 查看对象副本数

  ```bash
  ceph osd pool get ceph size
  ```

- 查看 pg 数

  ```bash
  ceph osd pool get ceph pg_num
  ```

- 查看 pgp 数，一般小于等于 pg_num

  ```bash
  ceph osd pool get ceph pg_num
  ```

### 删除存储池

- 在配置文件中写入

  ```bash
  [mon]
  mon allow_pool_delete = true
  ```

- 推送到每个节点

  ```bash
  ceph-deploy --overwrite-conf config push node1 node2 node3 
  ```

- 重新启动所有的服务守护程序

  ```bash
  systemctl restart ceph.target
  ```

- 删除存储池

  ```bash
  ceph osd pool rm ceph ceph --yes-i-really-really-mean-it
  ```

### 状态检查

- 检查集群的状态

  ```bash
  ceph -s
  ceph -w
  ceph health
  ceph health detail
  ```

- 检查 OSD 状态

  ```bash
  ceph osd status
  ceph osd tree
  ```

- 检查 Mon 状态

  ```bash
  ceph mon stat
  ceph quorum_status
  ```

### 为存储池指定 Ceph 应用类型

```bash
ceph osd pool application enable ceph <app>
```

`<app> ` 可以选择  `cephfs`，`rbd`，`rgw`。

### 存储池配额管理

- 根据对象数设置配额

  ```bash
  ceph osd pool set-quota ceph max_objects 10000
  ```

- 根据容量设置配额

  ```bash
  ceph osd pool set-quota ceph max_bytes 1048576
  ```

### 存储池对象访问

- 上传对象到存储池

  ```bash
  echo "test ceph objectstore" > test.txt
  rados -p ceph put test ./test.txt
  ```

- 列出存储池中的对象

  ```bash
  rados -p ceph ls
  ```

- 从存储池下载对象

  ```bash
  rados -p ceph get test ./test.txt.tmp
  ```

- 删除存储池的对象

  ```bash
  rados -p ceph rm test
  ```


## 部署结果

```bash
[root@client my-cluster]# ceph -s
  cluster:
    id:     7c4d332f-e068-4d8c-9d83-a5ed97de575c
    health: HEALTH_WARN
            mons are allowing insecure global_id reclaim
            clock skew detected on mon.node2, mon.node3
 
  services:
    mon: 3 daemons, quorum node1,node2,node3 (age 14m)
    mgr: node2(active, since 14m), standbys: node3, node1
    osd: 3 osds: 3 up (since 14m), 3 in (since 9h)
 
  data:
    pools:   1 pools, 64 pgs
    objects: 1.41k objects, 5.5 GiB
    usage:   17 GiB used, 43 GiB / 60 GiB avail
    pgs:     64 active+clean
```

这是分布式部署的结果，为完成在单机上的优化任务，我们也进行了单机部署。其过程是分布式部署的子过程，在此不再赘述。

## 参考资料

- [x-KATA-Unikernel/lab4 at main](https://github.com/OSH-2021/x-KATA-Unikernel/blob/main/lab4/)
- [Ceph部署实践——基于最新版Ceph14以及Centos7（初级课程）](https://www.bilibili.com/video/BV1PZ4y1L7c3?spm_id_from=333.1007.top_right_bar_window_custom_collection.content.click)
