#!/usr/bin/env python3
# Stand-in asset resizer: return `web` and `thumb` renditions of `src`.
import json, os, sys

env = json.load(sys.stdin)
with open(env["src"], "rb") as f: data = f.read()

def save(name, prefix):
  path = os.path.join(os.environ["TMPDIR"], name)
  with open(path, "wb") as f: f.write(prefix + data)
  return dict(path = path, type = "image/jpeg")

print(json.dumps({"files": {
  "web": save("web", b"WEB:"), "thumb": save("thumb", b"THUMB:")}}))
