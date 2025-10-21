FROM debian:bookworm-slim

# Install compiler and required tools
RUN apt-get update && apt-get install -y \
    build-essential cmake git libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy your webserver source code
WORKDIR /app
COPY . .

# Build your webserver
RUN make -j$(nproc)

# Create required runtime directories
RUN mkdir -p runtime/www/uploads runtime/logs runtime/www/admin

# Expose the HTTP ports
EXPOSE 8080 8081 9090

# Run the webserver
CMD ["./webserv", "working.conf"]
