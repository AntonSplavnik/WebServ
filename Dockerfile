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

# Expose the HTTP port
EXPOSE 8080

# Run the webserver
CMD ["./webserv"]
