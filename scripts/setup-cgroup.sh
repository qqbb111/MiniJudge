#!/usr/bin/env bash

set -e # 只要脚本中的某条命令执行失败，整个脚本就会立即退出

CGROUP_ROOT="/sys/fs/cgroup"
MINIJUDGE_CGROUP="$CGROUP_ROOT/minijudge"
MANAGER_CGROUP="$MINIJUDGE_CGROUP/manager"

CURRENT_USER="$USER"
CURRENT_GROUP="$(id -gn)"
CALLER_PID="$PPID"

echo "Setting up MiniJudge cgroup..."

# 检查是否为 cgroup v2
if [ ! -f "$CGROUP_ROOT/cgroup.controllers" ]; then # -f 判断路径是否为普通文件；[...] 等价于 test ...
    echo "Error: cgroup v2 is not available." >&2
    exit 1
fi

# 检查 memory controller 是否存在
if ! grep -qw memory "$CGROUP_ROOT/cgroup.controllers"; then # -q：quiet，安静模式，只返回状态码，不打印匹配内容；-w：whole word，匹配完整单词 memory
    echo "Error: memory controller is not available." >&2
    exit 1
fi

# 创建 MiniJudge 的 delegated cgroup
sudo mkdir -p "$MINIJUDGE_CGROUP"
sudo mkdir -p "$MANAGER_CGROUP"

# 把当前 shell 移到 manager cgroup
echo "$CALLER_PID" | sudo tee "$MANAGER_CGROUP/cgroup.procs" > /dev/null

# 开启 memory controller，使子 cgroup 可以使用 memory.max 等接口
if ! grep -qw memory "$MINIJUDGE_CGROUP/cgroup.subtree_control"; then
    echo +memory | sudo tee "$MINIJUDGE_CGROUP/cgroup.subtree_control" > /dev/null
fi

if ! grep -qw pids "$MINIJUDGE_CGROUP/cgroup.subtree_control"; then
    echo +pids | sudo tee "$MINIJUDGE_CGROUP/cgroup.subtree_control" > /dev/null
fi

# 把 MiniJudge 需要管理的 cgroup 节点交给当前用户
sudo chown "$CURRENT_USER:$CURRENT_GROUP" "$MINIJUDGE_CGROUP"
sudo chown "$CURRENT_USER:$CURRENT_GROUP" "$MINIJUDGE_CGROUP/cgroup.procs"
sudo chown "$CURRENT_USER:$CURRENT_GROUP" "$MINIJUDGE_CGROUP/cgroup.subtree_control"

echo "MiniJudge cgroup setup complete."
echo "User: $CURRENT_USER"
echo "Cgroup: $MINIJUDGE_CGROUP"
