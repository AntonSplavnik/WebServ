#!/usr/bin/env python3
"""
infinite_disconnect.py
Continuously spawn client threads that repeatedly connect to a server CGI endpoint,
send a GET request and then disconnect (gracefully or abruptly) after a randomized hold time.
Designed to run indefinitely to test how the server handles many client disconnections.

Usage:
  python3 infinite_disconnect.py --host 127.0.0.1 --port 8080 --path /largefile --clients 100 --hold-min 0.5 --hold-max 3 --abort 0.3

Options:
  --clients    : number of concurrent worker threads (default 100)
  --hold-min   : minimum seconds to hold connection before disconnect (default 0.5)
  --hold-max   : maximum seconds to hold connection before disconnect (default 3.0)
  --abort      : probability (0..1) to perform an abrupt disconnect (default 0.3)
  --spawn-delay: delay between starting worker threads (default 0.01)
"""

import socket, threading, time, argparse, random, sys


def worker(host, port, path, hold_min, hold_max, abort_prob, id, spawn_delay):
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(5)
            s.connect((host, port))
            req = f"GET {path} HTTP/1.1\r\nHost: {host}\r\nConnection: keep-alive\r\nUser-Agent: infinite-disconnect/1.0\r\n\r\n"
            s.sendall(req.encode('utf-8'))

            # try to read a small amount of headers (non-blocking-ish)
            try:
                s.settimeout(1.0)
                hdr = b''
                while b'\r\n\r\n' not in hdr:
                    part = s.recv(1024)
                    if not part:
                        break
                    hdr += part
                    if len(hdr) > 8192:
                        break
            except Exception:
                pass

            hold = random.uniform(hold_min, hold_max)
            time.sleep(hold)

            if random.random() < abort_prob:
                # abrupt disconnect: try shutdown then close
                try:
                    s.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                s.close()
                print(f"[w{id}] aborted after {hold:.2f}s")
            else:
                # graceful: try to read until the server closes or a short timeout expires
                try:
                    s.settimeout(0.5)
                    while True:
                        data = s.recv(1024)
                        if not data:
                            break
                except Exception:
                    pass
                s.close()
                print(f"[w{id}] closed gracefully after {hold:.2f}s")

            # small pause between iterations to avoid tight-looping on errors
            time.sleep(spawn_delay)

        except Exception as e:
            print(f"[w{id}] exception: {e}")
            time.sleep(1)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=8080)
    p.add_argument("--path", default="/")
    p.add_argument("--clients", type=int, default=100)
    p.add_argument("--hold-min", type=float, default=0.5)
    p.add_argument("--hold-max", type=float, default=3.0)
    p.add_argument("--abort", type=float, default=0.3)
    p.add_argument("--spawn-delay", type=float, default=0.01)
    args = p.parse_args()

    for i in range(args.clients):
        t = threading.Thread(target=worker, args=(args.host, args.port, args.path, args.hold_min, args.hold_max, args.abort, i, args.spawn_delay))
        t.daemon = True
        t.start()
        time.sleep(args.spawn_delay)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Interrupted, exiting.")
        sys.exit(0)


if __name__ == "__main__":
    main()

