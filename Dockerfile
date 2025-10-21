FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /home/root
VOLUME /home/root

# Install build tools and dependencies
RUN apt-get update -y && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        curl \
        lsb-release \
        valgrind \
        clang \
        wget \
        nano \
        python3 \
        python3-pip \
        python3-venv

# (Optional) Install norminette if needed
# RUN pip install norminette==3.3.53

# Copy project files
COPY . /home/root

# Build the project
RUN make -j$(nproc) || true

# Expose HTTP ports if needed
EXPOSE 8080 8081 9090

# Default command: open a shell
CMD ["bash"]