# Use NVIDIA CUDA development base image
FROM nvidia/cuda:12.2.0-devel-ubuntu22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libopenmpi-dev \
    openmpi-bin \
    wget \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js v18 LTS
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the workspace files to the container
COPY . .

# Expose port 3000
EXPOSE 3000

# Set default command to bash
CMD ["/bin/bash"]
