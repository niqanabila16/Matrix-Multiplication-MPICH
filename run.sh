#!/usr/bin/env bash
set -e  # Stop on error

IMAGE=mpi-net
NET=mpi-net
NODES=(node0 node1 node2 node3)

echo "Hapus container lama jika ada…"
docker rm -f "${NODES[@]}" 2>/dev/null || true

echo "Pastikan network ada…"
docker network create "$NET" 2>/dev/null || true

echo "Build image…"
docker build -t "$IMAGE" .

echo "Jalankan container…"
for n in "${NODES[@]}"; do
  docker run -dit --network "$NET" --name "$n" "$IMAGE"
done

echo "Ambil IP dari semua node…"
declare -A NODE_IPS
for n in "${NODES[@]}"; do
  NODE_IPS["$n"]=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$n")
done

echo "Update /etc/hosts di semua container…"
for n in "${NODES[@]}"; do
  for other in "${NODES[@]}"; do
    ip=${NODE_IPS[$other]}
    docker exec "$n" bash -c "echo '$ip $other' >> /etc/hosts"
  done
done

echo "Buat SSH key di node0…"
docker exec node0 bash -c 'ssh-keygen -t rsa -f /root/.ssh/id_rsa -q -N ""'
PUBKEY=$(docker exec node0 cat /root/.ssh/id_rsa.pub)

echo "Sebar pubkey ke semua node…"
for n in "${NODES[@]}"; do
  docker exec "$n" bash -c \
    "mkdir -p /root/.ssh && echo '$PUBKEY' >> /root/.ssh/authorized_keys && chmod 700 /root/.ssh && chmod 600 /root/.ssh/authorized_keys"
done

echo "Nonaktifkan host key checking…"
for n in "${NODES[@]}"; do
  docker exec "$n" bash -c \
    "echo 'StrictHostKeyChecking no' >> /etc/ssh/ssh_config"
done

echo "Salin hostfile & CSV ke node0…"
docker cp hostfile      node0:/home/mpiuser/hostfile
docker cp file_1.csv    node0:/home/mpiuser/
docker cp file_2.csv    node0:/home/mpiuser/

echo "Jalankan program MPI…"
docker exec node0 mpirun --allow-run-as-root \
  --hostfile /home/mpiuser/hostfile \
  -np 3 \
  -wdir /home/mpiuser \
  ./p13_054