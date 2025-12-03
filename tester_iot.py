#!/usr/bin/env python3
import socket, threading, struct, queue, json, time, random

# =====================================================
# CONFIGURACIÓN (CAMBIAS ESTO Y LISTO)
# =====================================================
N_PUBLISHERS = 5
PUBLISH_INTERVAL = 1.0  # segundos
TOPIC = "sensor/temp"


# =====================================================
# BROKER
# =====================================================
def start_broker():
    HOST, PORT = "0.0.0.0", 9000
    topics = {}
    lock = threading.Lock()

    def read_line(conn):
        buf = b""
        while True:
            c = conn.recv(1)
            if not c: return None
            if c == b'\n': return buf.decode().strip()
            buf += c

    def read_n(conn, n):
        data = b''
        while len(data) < n:
            chunk = conn.recv(n - len(data))
            if not chunk: return None
            data += chunk
        return data

    def client_thread(conn):
        try:
            line = read_line(conn)
            if not line:
                return

            parts = line.split()
            if parts[0] != "HELLO":
                conn.sendall(b"ERR\n")
                return

            role = parts[2].upper()
            conn.sendall(b"OK\n")

            # ------------------------------
            # SUBSCRIBER
            # ------------------------------
            if role == "SUBSCRIBER":
                while True:
                    line = read_line(conn)
                    if not line:
                        break
                    if line.startswith("SUB "):
                        topic = line.split()[1]
                        with lock:
                            lst = topics.setdefault(topic, [])
                            if conn not in lst:
                                lst.append(conn)
                        conn.sendall(b"OK\n")

            # ------------------------------
            # GATEWAY (recibe PUB)
            # ------------------------------
            elif role == "GATEWAY":
                while True:
                    line = read_line(conn)
                    if not line:
                        break
                    if not line.startswith("PUB "):
                        continue

                    parts = line.split()
                    topic = parts[1]
                    length = int(parts[2])

                    be = read_n(conn, 4)
                    payload = read_n(conn, length)

                    with lock:
                        subs = list(topics.get(topic, []))

                    msg = struct.pack(">I", len(payload)) + payload

                    for s in subs:
                        try:
                            s.sendall(msg)
                        except:
                            pass

        finally:
            with lock:
                for lst in topics.values():
                    if conn in lst:
                        lst.remove(conn)
            conn.close()

    def run_broker():
        sock = socket.socket()
        sock.bind((HOST, PORT))
        sock.listen(100)
        print("[BROKER] Running on 9000")
        while True:
            conn, _ = sock.accept()
            threading.Thread(target=client_thread, args=(conn,), daemon=True).start()

    threading.Thread(target=run_broker, daemon=True).start()


# =====================================================
# GATEWAY
# =====================================================
def start_gateway():
    PUBLISH_PORT = 10000
    BROKER = ("127.0.0.1", 9000)
    msg_queue = queue.Queue()

    def read_line(conn):
        buf = b""
        while True:
            c = conn.recv(1)
            if not c: return None
            if c == b'\n': return buf.decode().strip()
            buf += c

    def read_n(conn, n):
        data = b''
        while len(data) < n:
            chunk = conn.recv(n - len(data))
            if not chunk: return None
            data += chunk
        return data

    def publisher_thread(conn):
        try:
            conn.sendall(b"OK\n")

            while True:
                line = read_line(conn)
                if not line: break
                if line.startswith("PUB "):
                    parts = line.split()
                    topic = parts[1]
                    length = int(parts[2])
                    be = read_n(conn, 4)
                    payload = read_n(conn, length)
                    msg_queue.put((topic, payload))
                    conn.sendall(b"OK\n")
        finally:
            conn.close()

    def broker_sender():
        while True:
            topic, payload = msg_queue.get()
            try:
                s = socket.create_connection(BROKER)
                s.sendall(b"HELLO ROLE GATEWAY\n")
                s.recv(10)
                s.sendall(f"PUB {topic} {len(payload)}\n".encode())
                s.sendall(struct.pack(">I", len(payload)))
                s.sendall(payload)
                s.close()
            except:
                time.sleep(1)

    def run_gateway():
        sock = socket.socket()
        sock.bind(("0.0.0.0", PUBLISH_PORT))
        sock.listen(100)
        print("[GATEWAY] Running on 10000")
        threading.Thread(target=broker_sender, daemon=True).start()
        while True:
            conn, _ = sock.accept()
            threading.Thread(target=publisher_thread, args=(conn,), daemon=True).start()

    threading.Thread(target=run_gateway, daemon=True).start()


# =====================================================
# SUBSCRIBER
# =====================================================
def start_subscriber(topic="sensor/temp"):
    def run_sub():
        s = socket.create_connection(("127.0.0.1", 9000))
        s.sendall(b"HELLO ROLE SUBSCRIBER s1\n")
        s.recv(10)
        s.sendall(f"SUB {topic}\n".encode())
        s.recv(10)
        print("[SUBSCRIBER] Subscribed to", topic)
        while True:
            be = s.recv(4)
            if not be: break
            length = struct.unpack(">I", be)[0]
            payload = s.recv(length)
            print("[MSG]", payload.decode())

    threading.Thread(target=run_sub, daemon=True).start()


# =====================================================
# MULTI PUBLISHERS AUTOMÁTICOS
# =====================================================
def start_publishers(count=N_PUBLISHERS, topic=TOPIC, interval=PUBLISH_INTERVAL):

    def pub_thread(idx):
        time.sleep(1)  # esperar gateway
        s = socket.create_connection(("127.0.0.1", 10000))
        s.sendall(f"HELLO ROLE PUBLISHER pub{idx}\n".encode())
        s.recv(10)

        i = 0
        while True:
            payload_d = {
                "id": f"pub{idx}",
                "seq": i,
                "temp": round(20 + random.random()*10, 2),
                "hum": round(40 + random.random()*20, 2),
                "ts": int(time.time())
            }
            payload = json.dumps(payload_d).encode()

            s.sendall(f"PUB {topic} {len(payload)}\n".encode())
            s.sendall(struct.pack(">I", len(payload)))
            s.sendall(payload)

            s.recv(10)
            print(f"[pub{idx}] sent seq {i}")
            i += 1
            time.sleep(interval)

    for n in range(count):
        threading.Thread(target=pub_thread, args=(n,), daemon=True).start()


# =====================================================
# MAIN
# =====================================================
if __name__ == "__main__":
    print("=== Starting IoT Test Environment (1 file, multi components) ===")
    start_broker()
    start_gateway()
    start_subscriber()
    start_publishers()
    print("Todo ejecutándose. Publishers, Subscriber, Broker y Gateway activos.")
    while True:
        time.sleep(1)
