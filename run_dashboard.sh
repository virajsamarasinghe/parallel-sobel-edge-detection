#!/bin/bash

# Exit immediately if any command fails
set -e

# Visual divider
echo "================================================================="
echo "⚡ Starting Sobel Parallel Edge Detection Benchmarking Suite ⚡"
echo "================================================================="

# Check if Docker daemon is running
if ! docker info >/dev/null 2>&1; then
  echo "❌ Error: Docker is not running. Please launch Docker Desktop and try again."
  exit 1
fi

# Check if port 3000 is occupied and stop any conflicting container if possible
if docker ps --format '{{.Ports}}' | grep -q '3000->3000'; then
  CONFLICTING_CONTAINER=$(docker ps --filter "publish=3000" --format "{{.ID}}")
  if [ -n "$CONFLICTING_CONTAINER" ]; then
    echo "⚠️  Port 3000 is in use by container $CONFLICTING_CONTAINER. Stopping it..."
    docker stop "$CONFLICTING_CONTAINER" >/dev/null
  fi
fi

# 1. Build the Docker image containing compiler tools and Node.js
echo "👉 [1/2] Building custom Docker image (parallel-sobel:latest)..."
docker build -t parallel-sobel .

# 2. Run the Docker container
echo "👉 [2/2] Launching container and running compilation and server..."
echo "-----------------------------------------------------------------"
echo "🌐 Dashboard will be available at: http://localhost:3000"
echo "-----------------------------------------------------------------"

docker run --rm -p 3000:3000 -v "$(pwd):/app" parallel-sobel make dashboard

