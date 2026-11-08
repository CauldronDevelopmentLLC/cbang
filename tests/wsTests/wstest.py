#!/usr/bin/env python3
'''
Drives wsEcho for wsTests.  Each scenario starts the server on a free
127.0.0.1 port, talks to it over a raw socket with hand-built frames and
prints exactly what arrives on the wire.  The server's event log is printed
after the client's, so the output does not depend on thread timing.

  wstest.py <path to wsEcho> <scenario>

Python standard library only.  A WebSocket library would hide the framing
these tests check.
'''

import base64
import hashlib
import os
import socket
import struct
import subprocess
import sys
import threading
import time


OPCODES = {0: 'CONTINUE', 1: 'TEXT', 2: 'BINARY', 8: 'CLOSE', 9: 'PING',
           10: 'PONG'}
TEXT, CLOSE, PING, PONG, CONTINUE = 1, 8, 9, 10, 0
RESERVED_STATUS = (1005, 1006, 1015) # RFC 6455 7.4.1: never sent
GUID = b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11'
KEY = b'dGhlIHNhbXBsZSBub25jZQ==' # The RFC's example key
MASK = b'\x37\xfa\x21\x3d'        # Fixed, so runs are reproducible

wsEcho = None
reserved_seen = [] # Reserved status codes found in frames the server sent


def out(*args, **kwargs): print(*args, flush = True, **kwargs)


def payload_text(data):
  if len(data) <= 32: return 'hex=' + (data.hex() or '-')
  return 'sha1=' + hashlib.sha1(data).hexdigest()[:16]


def pattern(n): return (b'abcdefghijklmnopqrstuvwxyz' * (n // 26 + 1))[:n]


def close_payload(status = None, reason = b''):
  if status is None: return b''
  return struct.pack('!H', status) + reason


def build_frame(opcode, payload = b'', fin = True, mask = True):
  header = bytes([(0x80 if fin else 0) | opcode])
  n = len(payload)
  m = 0x80 if mask else 0

  if n < 126: header += bytes([m | n])
  elif n <= 0xffff: header += bytes([m | 126]) + struct.pack('!H', n)
  else: header += bytes([m | 127]) + struct.pack('!Q', n)

  if mask:
    header += MASK
    payload = bytes(b ^ MASK[i & 3] for i, b in enumerate(payload))

  return header + payload


class Peer:
  'One end of a WebSocket connection over a raw socket.'

  def __init__(self, sock, name):
    self.sock = sock
    self.name = name
    self.buf = b''
    self.eof = None # How the TCP connection ended, once it has


  def fill(self, timeout):
    if self.eof: return False
    self.sock.settimeout(timeout)

    try: data = self.sock.recv(65536)
    except socket.timeout: return False
    except ConnectionResetError: data, self.eof = b'', 'reset'

    if not data:
      if not self.eof: self.eof = 'closed'
      return False

    self.buf += data
    return True


  def read(self, n, timeout):
    deadline = time.time() + timeout
    while len(self.buf) < n:
      left = deadline - time.time()
      if left <= 0 or not self.fill(left): return None

    data, self.buf = self.buf[:n], self.buf[n:]
    return data


  def send(self, data):
    try:
      self.sock.sendall(data)
      return True
    except (BrokenPipeError, ConnectionResetError): return False


  def send_frame(self, opcode, payload = b'', fin = True, mask = True):
    return self.send(build_frame(opcode, payload, fin, mask))


  def recv_frame(self, timeout = 5):
    '''Returns (fin, opcode, masked, payload), or None if no whole frame
    arrives before the timeout or the TCP connection closes.'''
    hdr = self.read(2, timeout)
    if hdr is None: return None

    fin, opcode = bool(hdr[0] & 0x80), hdr[0] & 0x0f
    masked, n = bool(hdr[1] & 0x80), hdr[1] & 0x7f
    if n == 126: n = struct.unpack('!H', self.read(2, timeout))[0]
    elif n == 127: n = struct.unpack('!Q', self.read(8, timeout))[0]
    key = self.read(4, timeout) if masked else None

    payload = self.read(n, timeout) if n else b''
    if payload is None: return None
    if masked: payload = bytes(b ^ key[i & 3] for i, b in enumerate(payload))

    return fin, opcode, masked, payload


  def print_frame(self, frame):
    fin, opcode, masked, payload = frame
    out('%s: frame op=%s fin=%d mask=%d len=%d %s' % (
      self.name, OPCODES.get(opcode, opcode), fin, masked, len(payload),
      payload_text(payload)))

    if opcode == CLOSE:
      if len(payload) < 2: out('%s:   close no status' % self.name)
      else:
        status = struct.unpack('!H', payload[:2])[0]
        if status in RESERVED_STATUS: reserved_seen.append(status)
        out('%s:   close status=%d reason=%r' % (
          self.name, status, payload[2:].decode(errors = 'replace')))


  def recv_message(self, timeout = 5):
    'Print frames until one with FIN set.  Returns the joined payload.'
    data = b''
    while True:
      frame = self.recv_frame(timeout)
      if frame is None:
        self.print_end(timeout)
        return None

      self.print_frame(frame)
      data += frame[3]
      if frame[0]: return data


  def print_end(self, timeout):
    if self.eof: out('%s: tcp %s' % (self.name, self.eof))
    else: out('%s: no frame within %ds' % (self.name, timeout))


  def drain(self, timeout = 5):
    'Print every frame until the TCP connection ends.'
    while True:
      frame = self.recv_frame(timeout)
      if frame is None: return self.print_end(timeout)
      self.print_frame(frame)


  def close(self): self.sock.close()


def accept_key(key):
  return base64.b64encode(hashlib.sha1(key + GUID).digest())


class Server:
  'Runs wsEcho in server mode.'

  def __init__(self, *args, connections = 1):
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    self.port = s.getsockname()[1]
    s.close()

    cmd = [wsEcho, 'server', str(self.port), '--connections',
           str(connections)] + list(args)
    self.proc = subprocess.Popen(cmd, stdout = subprocess.PIPE, text = True)

    line = self.proc.stdout.readline()
    if line != 'server: listening\n':
      raise Exception('Server failed to start: %r' % line)


  def connect(self, name = 'client', quiet = False):
    sock = socket.create_connection(('127.0.0.1', self.port), timeout = 5)
    peer = Peer(sock, name)

    peer.send(b'GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\n'
              b'Connection: Upgrade\r\nSec-WebSocket-Key: ' + KEY +
              b'\r\nSec-WebSocket-Version: 13\r\n\r\n')

    while b'\r\n\r\n' not in peer.buf:
      if not peer.fill(5): raise Exception('No upgrade response')

    head, peer.buf = peer.buf.split(b'\r\n\r\n', 1)
    lines = head.decode().split('\r\n')
    hdrs = dict((k.strip().lower(), v.strip()) for k, v in
                (l.split(':', 1) for l in lines[1:]))

    accept = hdrs.get('sec-websocket-accept', '').encode()
    if not quiet or accept != accept_key(KEY): out('%s: upgrade %s accept=%s' % (
      name, lines[0].split(' ', 2)[1],
      'ok' if accept == accept_key(KEY) else 'bad'))

    return peer


  def finish(self, summarize = False):
    'Wait for the server to exit, then print its log.'
    try: log, _ = self.proc.communicate(timeout = 40)
    except subprocess.TimeoutExpired:
      self.proc.kill()
      log, _ = self.proc.communicate()
      log += 'server: killed\n'

    lines = log.splitlines()

    if summarize:
      counts = {}
      for line in lines: counts[line] = counts.get(line, 0) + 1
      lines = ['%dx %s' % (n, line) for line, n in counts.items()]

    out('--- server log ---')
    for line in lines: out(line)
    out('server: exit %d' % self.proc.returncode)


def scenario_close_no_code():
  srv = Server()
  c = srv.connect()
  c.send_frame(CLOSE, b'')
  c.drain()
  srv.finish()


def scenario_close_normal():
  srv = Server()
  c = srv.connect()
  c.send_frame(CLOSE, close_payload(1000, b'goodbye'))
  c.drain()
  srv.finish()


def scenario_close_before_tcp():
  # Before #10 was fixed close() shut the connection down while the close
  # frame was still queued, so it was usually lost.
  n = 50
  srv = Server(connections = n)
  outcomes = {}

  for i in range(n):
    c = srv.connect('client%d' % i, quiet = True)
    payload = close_payload(1000, b'x') if i % 2 else b''
    c.send_frame(CLOSE, payload)

    frame = c.recv_frame()
    if frame is None: outcome = 'tcp %s before close frame' % c.eof
    elif frame[1] != CLOSE: outcome = 'unexpected %s frame' % frame[1]
    elif frame[3] != payload: outcome = 'close frame payload changed'
    else:
      c.read(1, 5)
      outcome = 'close frame, then tcp %s' % (c.eof or 'still open')

    outcomes[outcome] = outcomes.get(outcome, 0) + 1
    c.close()

  for outcome, count in outcomes.items():
    out('%d of %d: %s' % (count, n, outcome))

  srv.finish(summarize = True)


def scenario_reserved_codes():
  # Peers send no code, then each code RFC 6455 says must never be sent.
  # cbang must not repeat any of them back.
  cases = [None] + list(RESERVED_STATUS)
  srv = Server(connections = len(cases))

  for status in cases:
    c = srv.connect('client[%s]' % status)
    c.send_frame(CLOSE, close_payload(status, b'r'))
    c.drain()
    c.close()

  out('reserved codes sent by server: %s' % (
    ' '.join(map(str, reserved_seen)) or 'none'))
  srv.finish()


def scenario_server_close():
  srv = Server('--close-on-message')
  c = srv.connect()
  c.send_frame(TEXT, b'hi')

  # Don't reply to the close yet, to see whether the server waits for it
  c.drain(timeout = 2)
  srv.finish()


def scenario_framing():
  srv = Server()
  c = srv.connect()

  for n in (0, 125, 126, 65535, 65536):
    data = pattern(n)
    out('client: send TEXT len=%d' % n)
    c.send_frame(TEXT, data)
    echo = c.recv_message(timeout = 2)
    if echo is not None: out('client: echo matches: %s' % (echo == data))

  c.send_frame(CLOSE, close_payload(1000))
  c.drain()
  srv.finish()


def scenario_unmasked():
  srv = Server()
  c = srv.connect()
  c.send_frame(TEXT, b'unmasked', mask = False)
  c.drain()
  srv.finish()


def scenario_fragmented():
  srv = Server()
  c = srv.connect()
  c.send_frame(TEXT, b'Hel', fin = False)
  c.send_frame(CONTINUE, b'lo, ', fin = False)
  c.send_frame(CONTINUE, b'world')
  c.recv_message()
  c.send_frame(CLOSE, close_payload(1000))
  c.drain()
  srv.finish()


def scenario_fragmented_ping():
  # RFC 6455 5.4: control frames may be injected in the middle of a
  # fragmented message and must not disturb it.
  srv = Server()
  c = srv.connect()
  c.send_frame(TEXT, b'Hel', fin = False)
  c.send_frame(PING, b'p')
  c.send_frame(CONTINUE, b'lo')
  c.recv_message()
  c.send_frame(CLOSE, close_payload(1000))
  c.drain()
  srv.finish()


def scenario_ping():
  srv = Server()
  c = srv.connect()
  start = time.time()
  c.send_frame(PING, b'ping-1')
  frame = c.recv_frame(timeout = 10)
  elapsed = time.time() - start
  if frame:
    c.print_frame(frame)
    # cbang defers pongs; see schedulePong()
    out('client: pong within 1s: %s' % (elapsed < 1))
  else: c.print_end(10)

  c.send_frame(CLOSE, close_payload(1000))
  c.drain()
  srv.finish()


def scenario_fragmented_control():
  srv = Server(connections = 2)

  for opcode in (PING, CLOSE):
    c = srv.connect('client[%s]' % OPCODES[opcode])
    c.send_frame(opcode, close_payload(1000) if opcode == CLOSE else b'x',
                 fin = False)
    c.drain()
    c.close()

  srv.finish()


def scenario_invalid_opcode():
  srv = Server(connections = 2)

  for opcode in (3, 11): # A reserved data and a reserved control opcode
    c = srv.connect('client[%d]' % opcode)
    c.send_frame(opcode, b'x')
    c.drain()
    c.close()

  srv.finish()


def scenario_too_big():
  srv = Server('--max-size', '16', connections = 3)

  c = srv.connect('client[16]')
  c.send_frame(TEXT, pattern(16))
  c.recv_message()
  c.send_frame(CLOSE, close_payload(1000))
  c.drain()
  c.close()

  c = srv.connect('client[17]')
  c.send_frame(TEXT, pattern(17))
  c.drain()
  c.close()

  # The limit applies to the whole message, not each fragment
  c = srv.connect('client[10+10]')
  c.send_frame(TEXT, pattern(10), fin = False)
  c.send_frame(CONTINUE, pattern(10))
  c.drain()
  c.close()

  srv.finish()


def client_env():
  env = dict(os.environ)
  for k in list(env):
    if k.lower().endswith('_proxy'): del env[k]
  return env


def run_client(port):
  return subprocess.run([wsEcho, 'client', str(port)], stdout = subprocess.PIPE,
                        text = True, env = client_env(), timeout = 40)


def scenario_cbang_client():
  srv = Server()
  r = run_client(srv.port)
  out('--- client log ---')
  out(r.stdout, end = '')
  out('client: exit %d' % r.returncode)
  srv.finish()


def scenario_cbang_client_wire():
  # cbang's client against a raw server, to see its frames on the wire
  lsock = socket.socket()
  lsock.bind(('127.0.0.1', 0))
  lsock.listen(1)
  port = lsock.getsockname()[1]

  result = {}
  client = threading.Thread(target = lambda: result.update(r = run_client(port)))
  client.start()

  lsock.settimeout(10)
  sock, _ = lsock.accept()
  p = Peer(sock, 'wire')

  while b'\r\n\r\n' not in p.buf:
    if not p.fill(5): raise Exception('No upgrade request')

  head, p.buf = p.buf.split(b'\r\n\r\n', 1)
  lines = head.decode().split('\r\n')
  hdrs = dict((k.strip().lower(), v.strip()) for k, v in
              (l.split(':', 1) for l in lines[1:]))

  out('wire: request %s' % lines[0])
  for name in ('upgrade', 'connection', 'sec-websocket-version'):
    out('wire:   %s: %s' % (name, hdrs.get(name)))
  key = hdrs.get('sec-websocket-key', '').encode()
  out('wire:   key decodes to %d bytes' % len(base64.b64decode(key)))

  p.send(b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n'
         b'Connection: Upgrade\r\nSec-WebSocket-Accept: ' + accept_key(key) +
         b'\r\n\r\n')

  msg = p.recv_message()
  if msg is not None: p.send_frame(TEXT, msg, mask = False)

  # Don't reply to the close yet, to see whether the client waits for it
  p.drain(timeout = 2)
  p.close()
  client.join()

  r = result['r']
  out('--- client log ---')
  out(r.stdout, end = '')
  out('client: exit %d' % r.returncode)


if __name__ == '__main__':
  wsEcho = os.path.abspath(sys.argv[1])
  globals()['scenario_' + sys.argv[2]]()
