'''
Drives httpEcho for httpTests.  Each scenario starts the server on a free
127.0.0.1 port, sends hand-built requests over one raw socket and prints the
status line and body of each response, in order, until the server closes the
connection or goes quiet.

  httptest.py <path to httpEcho> <scenario>

Python standard library only.  An HTTP library would hide the framing these
tests check.
'''

import os
import socket
import subprocess
import sys


httpEcho = None


def out(*args, **kwargs): print(*args, flush = True, **kwargs)


class Server:
  'Runs httpEcho.'

  def __init__(self):
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    self.port = s.getsockname()[1]
    s.close()

    self.proc = subprocess.Popen([httpEcho, str(self.port)],
                                 stdout = subprocess.PIPE, text = True)

    line = self.proc.stdout.readline()
    if line != 'server: listening\n':
      raise Exception('Server failed to start: %r' % line)


  def exchange(self, data):
    'Send data on one connection and print every response that comes back.'
    sock = socket.create_connection(('127.0.0.1', self.port), timeout = 2)
    sock.sendall(data)

    buf = b''
    closed = False
    while True:
      # Parse every complete response in buf
      while b'\r\n\r\n' in buf:
        head, rest = buf.split(b'\r\n\r\n', 1)
        lines = head.decode().split('\r\n')
        hdrs = dict((k.strip().lower(), v.strip()) for k, v in
                    (l.split(':', 1) for l in lines[1:]))
        length = int(hdrs.get('content-length', '0'))
        if len(rest) < length: break

        body, buf = rest[:length], rest[length:]
        status = lines[0].split(' ', 2)[1]
        out('response %s %s%s' % (
          status, body.decode(errors = 'replace'),
          ' (close)' if hdrs.get('connection', '').lower() == 'close' else ''))

      if closed: break

      try:
        data = sock.recv(65536)
        if data: buf += data
        else: closed = True
      except socket.timeout: break

    out('connection closed' if closed else 'connection open')
    sock.close()


  def finish(self):
    s = socket.create_connection(('127.0.0.1', self.port), timeout = 2)
    s.sendall(b'GET /quit HTTP/1.1\r\nHost: x\r\n\r\n')
    try: s.recv(65536)
    except socket.timeout: pass
    s.close()

    try: self.proc.communicate(timeout = 10)
    except subprocess.TimeoutExpired:
      self.proc.kill()
      out('server: did not exit')


def request(method, path, body = None, headers = ()):
  r = '%s %s HTTP/1.1\r\nHost: x\r\n' % (method, path)
  for h in headers: r += h + '\r\n'
  if body is not None and not any(h.lower().startswith('transfer-encoding')
                                  for h in headers):
    r += 'Content-Length: %d\r\n' % len(body)
  return (r + '\r\n').encode() + (body or b'')


def run(data):
  server = Server()
  server.exchange(data)
  server.finish()


# A body on a method without one must be read, not left on the connection to
# be parsed as the next request.
def scenario_delete_body():
  run(request('DELETE', '/x', b'hello') + request('GET', '/y'))


def scenario_get_body():
  run(request('GET', '/x', b'hello') + request('GET', '/y'))


def scenario_post_body():
  run(request('POST', '/x', b'hello') + request('GET', '/y'))


def scenario_chunked_delete():
  run(request('DELETE', '/x', b'5\r\nhello\r\n0\r\n\r\n',
              ['Transfer-Encoding: chunked']))


# Both framings, or one the server can't decode, must be refused and the
# connection closed.  The /y after it must not be answered.
def scenario_both_framings():
  run(request('POST', '/x', b'5\r\nhello\r\n0\r\n\r\n',
              ['Transfer-Encoding: chunked', 'Content-Length: 5']) +
      request('GET', '/y'))


def scenario_unknown_coding():
  run(request('POST', '/x', b'hello', ['Transfer-Encoding: gzip']) +
      request('GET', '/y'))


if __name__ == '__main__':
  httpEcho = os.path.abspath(sys.argv[1])
  globals()['scenario_' + sys.argv[2]]()
