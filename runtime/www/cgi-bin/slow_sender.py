#!/usr/bin/env python3
"""
slow_sender.py

A configurable "slow sender" client to test servers that use blocking `recv()`.
It opens one or more connections and slowly sends a large request body (or headers),
pausing between small chunks to simulate a very slow client.

Usage examples:
  python3 runtime/www/cgi-bin/slow_sender.py --host 127.0.0.1 --port 8080 --path /upload \
      --size 10485760 --chunk 10 --delay 0.1 --conns 5

Options:
  --host            Server host (default: 127.0.0.1)
  --port            Server port (default: 8080)
  --path            Request path (default: /upload)
  --method          HTTP method: POST or PUT or GET or HEAD (default: POST)
  --size            Total body size in bytes for each connection (default: 10MB)
  --chunk           Chunk size in bytes per send (default: 10)
  --delay           Delay in seconds between chunk sends (default: 0.1)
  --conns           Number of parallel connections to open (default: 1)
  --headers-slow    If set, send headers byte-by-byte with --slow-headers-delay
  --slow-headers-delay Delay between header bytes when --headers-slow (default: 0.02)
  --keepalive       Send 'Connection: keep-alive' instead of close
  --timeout         Socket connect timeout seconds (default: 5)
"""
from __future__ import annotations

import socket
import threading
import time
import argparse
import sys

def make_request_template(host: str, path: str, method: str, content_length: int, keepalive: bool) -> str:
    conn_hdr = "keep-alive" if keepalive else "close"
    headers = [
        f"{method} {path} HTTP/1.1",
        f"Host: {host}",
        "User-Agent: slow-sender/1.0",
        f"Content-Length: {content_length}",
        "Content-Type: application/octet-stream",
        f"Connection: {conn_hdr}",
        "",
        ""
    ]
    return "\r\n".join(headers)

def worker(id: int,
           host: str,
           port: int,
           path: str,
           method: str,
           total_size: int,
           chunk_size: int,
           delay: float,
           headers_slow: bool,
           slow_headers_delay: float,
           keepalive: bool,
           timeout: float) -> None:
    try:
        s = socket.create_connection((host, port), timeout=timeout)
    except Exception as e:
        print(f"[{id}] Connection error: {e}")
        return

    # Try to reduce buffering on sender side
    try:
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    except Exception:
        pass

    content_template = b"x" * chunk_size

    # Prepare headers
    headers = make_request_template(host, path, method.upper(), total_size, keepalive)
    try:
        if headers_slow:
            # Send headers byte-by-byte slowly
            for b in headers.encode('utf-8'):
                s.send(bytes([b]))
                time.sleep(slow_headers_delay)
        else:
            s.sendall(headers.encode("utf-8"))
    except Exception as e:
        print(f"[{id}] Error sending headers: {e}")
        try:
            s.close()
        except Exception:
            pass
        return

    # If method is GET or HEAD and no body desired, finish here
    if method.upper() in ("GET", "HEAD") or total_size == 0:
        print(f"[{id}] Sent {method.upper()} request (no body). Closing.")
        try:
            s.close()
        except Exception:
            pass
        return

    sent = 0
    start_t = time.time()
    try:
        while sent < total_size:
            to_send = min(chunk_size, total_size - sent)
            # reuse the prebuilt bytes for speed
            if to_send == chunk_size:
                s.sendall(content_template)
            else:
                s.sendall(b"x" * to_send)
            sent += to_send

            # Print periodic progress: every 100 KB or on completion
            if sent % (1024 * 100) < to_send or sent == total_size:
                elapsed = time.time() - start_t
                print(f"[{id}] Sent {sent}/{total_size} bytes ({elapsed:.1f}s)")

            time.sleep(delay)
    except Exception as e:
        print(f"[{id}] Error during body send after {sent} bytes: {e}")
    finally:
        # try to half-close to let server process request
        try:
            s.shutdown(socket.SHUT_WR)
        except Exception:
            pass

        # Optionally read a short response (non-blocking-ish with timeout)
        try:
            s.settimeout(1.0)
            resp = b""
            try:
                while True:
                    chunk = s.recv(4096)
                    if not chunk:
                        break
                    resp += chunk
            except socket.timeout:
                pass
            except Exception:
                # other recv errors are ignored here
                pass

            if resp:
                first_line = resp.split(b"\r\n", 1)[0]
                # decode safely for printing
                try:
                    first_line_text = first_line.decode('utf-8', errors='replace')
                except Exception:
                    first_line_text = repr(first_line)
                print(f"[{id}] Received response head: {first_line_text}")
            else:
                print(f"[{id}] No response received.")
        except Exception as e:
            print(f"[{id}] Error reading response: {e}")
        try:
            s.close()
        except Exception:
            pass
        elapsed = time.time() - start_t
        print(f"[{id}] Done. Total sent: {sent} bytes in {elapsed:.1f}s")

def main() -> None:
    parser = argparse.ArgumentParser(description="Slow sender: send a large request very slowly.")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    parser.add_argument("--port", type=int, default=8080, help="Server port")
    parser.add_argument("--path", default="/upload", help="Request path")
    parser.add_argument("--method", default="POST", choices=["POST", "PUT", "GET", "HEAD"], help="HTTP method")
    parser.add_argument("--size", type=int, default=10 * 1024 * 1024, help="Total body size in bytes per connection (default 10MB)")
    parser.add_argument("--chunk", type=int, default=10, help="Chunk size in bytes per send (default 10)")
    parser.add_argument("--delay", type=float, default=0.1, help="Delay (seconds) between chunk sends (default 0.1)")
    parser.add_argument("--conns", type=int, default=1, help="Number of parallel connections (default 1)")
    parser.add_argument("--headers-slow", action="store_true", help="Send headers slowly, byte-by-byte")
    parser.add_argument("--slow-headers-delay", type=float, default=0.02, help="Delay between header bytes when --headers-slow")
    parser.add_argument("--keepalive", action="store_true", help="Use Connection: keep-alive")
    parser.add_argument("--timeout", type=float, default=5.0, help="Connect timeout seconds")
    args = parser.parse_args()

    threads = []
    for i in range(args.conns):
        t = threading.Thread(
            target=worker,
            args=(
                i + 1,
                args.host,
                args.port,
                args.path,
                args.method,
                args.size,
                args.chunk,
                args.delay,
                args.headers_slow,
                args.slow_headers_delay,
                args.keepalive,
                args.timeout
            ),
        )
        t.daemon = True
        t.start()
        threads.append(t)
        # slight spacing between connection starts
        time.sleep(0.05)

    try:
        for t in threads:
            t.join()
    except KeyboardInterrupt:
        print("Interrupted by user, exiting...")
        sys.exit(1)

if __name__ == "__main__":
    main()
