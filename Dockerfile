FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y openmpi-bin libopenmpi-dev mpich build-essential openssh-server && \
    mkdir -p /var/run/sshd

RUN useradd -m mpiuser && echo "mpiuser:mpiuser" | chpasswd && adduser mpiuser sudo

USER mpiuser
WORKDIR /home/mpiuser

COPY p13_054.c .
RUN mpicc -o p13_054 p13_054.c

USER root
CMD ["/usr/sbin/sshd", "-D"]

# FROM ubuntu:20.04

# ENV DEBIAN_FRONTEND=noninteractive

# RUN apt-get update && \
#     apt-get install -y openmpi-bin libopenmpi-dev mpich build-essential openssh-server && \
#     mkdir -p /var/run/sshd

# RUN useradd -m mpiuser && echo "mpiuser:mpiuser" | chpasswd && adduser mpiuser sudo

# RUN apt-get update && \
#     apt-get install -y openmpi-bin libopenmpi-dev mpich build-essential openssh-server gdb && \
#     mkdir -p /var/run/sshd

# USER mpiuser
# WORKDIR /home/mpiuser

# COPY p13_054.c .
# RUN mpicc -o p13_054 p13_054.c

# USER root
# CMD ["/usr/sbin/sshd", "-D"]
