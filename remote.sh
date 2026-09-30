#!/usr/bin/env bash
set -euo pipefail

LOCAL_DIR="$(cd "$(dirname "$0")" && pwd)"
REMOTE_DIR="CS4402/$(basename "$LOCAL_DIR")"
MPI_HOME='$HOME/opt/openmpi'
MAX_HOSTS="${MAX_HOSTS:-8}"

scan_hosts() {
  ssh csgate 'for i in $(seq -w 2 47); do h=csg21-$i.ucc.ie;
    timeout 2 bash -c "</dev/tcp/$h/22" 2>/dev/null && echo $h & done; wait' \
    2>/dev/null | sort | head -n "$MAX_HOSTS"
}

setup() {
  ssh-copy-id -i ~/.ssh/id_ed25519.pub csgate
  ssh-copy-id -i ~/.ssh/id_ed25519.pub "$(scan_hosts | head -n1 | cut -d. -f1)"
  ssh "$(scan_hosts | head -n1 | cut -d. -f1)" bash -s <<'EOF'
set -e
[ -f ~/.ssh/id_ed25519 ] || ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519
grep -qf ~/.ssh/id_ed25519.pub ~/.ssh/authorized_keys 2>/dev/null \
  || cat ~/.ssh/id_ed25519.pub >> ~/.ssh/authorized_keys
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys
grep -q '^Host csg21-\*' ~/.ssh/config 2>/dev/null || printf '%s\n' \
  'Host csg21-*' '    StrictHostKeyChecking accept-new' '    LogLevel ERROR' \
  >> ~/.ssh/config
chmod 600 ~/.ssh/config
EOF
}

sync() {
  local head="$1"
  ssh "$head" "mkdir -p $REMOTE_DIR"
  rsync -az --delete --exclude main --exclude '*.o' --exclude machines \
    --exclude .clangd "$LOCAL_DIR/" "$head:$REMOTE_DIR/"
}

run() {
  local np="${1:-4}" prog="${2:-main}" hosts head
  hosts="$(scan_hosts)"
  head="$(head -n1 <<<"$hosts" | cut -d. -f1)"
  echo "hosts: $(tr '\n' ' ' <<<"$hosts")" >&2
  sync "$head"
  ssh -t "$head" "cd $REMOTE_DIR && printf '%s\n' $(tr '\n' ' ' <<<"$hosts") > machines \
    && export PATH=$MPI_HOME/bin:\$PATH && make -s $prog \
    && mpirun --prefix $MPI_HOME -np $np --hostfile machines --map-by node \
       --oversubscribe ./$prog" 2> >(grep -v setlocale >&2)
}

case "${1:-}" in
  setup) setup ;;
  hosts) scan_hosts ;;
  run) shift; run "$@" ;;
  shell) ssh -t "$(scan_hosts | head -n1 | cut -d. -f1)" "cd $REMOTE_DIR; exec \$SHELL -l" ;;
  *) echo "usage: $0 setup | hosts | run [np] [prog] | shell" >&2; exit 1 ;;
esac
