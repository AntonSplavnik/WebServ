#!/usr/bin/env python3
"""
slow_reader.py
Creates many clients that request a large resource and then read the response extremely slowly.
Usage:
  python3 slow_reader.py --host 127.0.0.1 --port 8080 --path /largefile --clients 1 --delay 0.5 --chunk 1

Options:
  --clients  : number of sockets to open in this process (default 50)
  --delay    : seconds to wait between reading each small chunk (default 0.5)
  --chunk    : how many bytes to read each time (default 1)  <-- very slow read
  --path     : request path (default /largefile)
"""

import socket, time, argparse, threading, sys

def worker(host, port, path, delay, chunk, id):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        s.connect((host, port))
        req = "GET {} HTTP/1.1\r\nHost: {}\r\nUser-Agent: slow-reader/1.0\r\nConnection: keep-alive\r\n\r\n".format(path, host)
        s.sendall(req.encode('utf-8'))
        print("[w{}] sent request".format(id))

        # Now read very slowly (chunk bytes every delay seconds)
        buf = b''
        # first read headers until we see CRLFCRLF, then start slow body reads
        headers = b''
        while b'\r\n\r\n' not in headers:
            part = s.recv(4096)
            if not part:
                print("[w{}] server closed while waiting headers".format(id))
                s.close()
                return
            headers += part
            # If headers are included with some body, put rest into buf
            if b'\r\n\r\n' in headers:
                idx = headers.find(b'\r\n\r\n') + 4
                buf = headers[idx:]
                break

        print("[w{}] headers received (len headers bytes: {})".format(id, len(headers)))
        # Now slow read body
        total = len(buf)
        if buf:
            # consume initial buffered body slowly
            # push back to stream by handling as already-read bytes
            # we will slowly "process" it as if reading chunk by chunk
            # convert to list of small pieces to simulate slow consumption
            pending = bytearray(buf)
            while pending:
                take = min(chunk, len(pending))
                _ = pending[:take]; pending = pending[take:]
                total += take
                time.sleep(delay)
        # Now continue reading from socket slowly
        while True:
            try:
                data = s.recv(chunk)
            except Exception as e:
                print("[w{}] recv error: {}".format(id, e))
                break
            if not data:
                print("[w{}] connection closed by server, total body bytes read: {}".format(id, total))
                break
            total += len(data)
            # simulate very slow processing/reading
            time.sleep(delay)
        s.close()
    except Exception as e:
        print("[w{}] exception: {}".format(id, e))

def spawn_many(host, port, path, clients, delay, chunk):
    threads = []
    for i in range(clients):
        t = threading.Thread(target=worker, args=(host, port, path, delay, chunk, i))
        t.daemon = True
        t.start()
        threads.append(t)
        time.sleep(0.01)  # small gap to quickly open sockets but not all at once
    try:
        while True:
            alive = any(t.is_alive() for t in threads)
            if not alive:
                break
            time.sleep(1)
    except KeyboardInterrupt:
        print("Interrupted, exiting.")
        sys.exit(0)

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=8080)
    p.add_argument("--path", default="/largefile")
    p.add_argument("--clients", type=int, default=50)
    p.add_argument("--delay", type=float, default=0.5)
    p.add_argument("--chunk", type=int, default=1)
    args = p.parse_args()
    print("Starting {} slow readers to {}:{}{}".format(args.clients, args.host, args.port, args.path))
    spawn_many(args.host, args.port, args.path, args.clients, args.delay, args.chunk)
