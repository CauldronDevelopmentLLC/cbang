#!/usr/bin/env python3
# Read the file named by the envelope's `src`, upper-case its content, write
# the result to a file under $TMPDIR and return its path.
import json, os, sys

env = json.load(sys.stdin)
with open(env["src"], "rb") as f: data = f.read()

out = os.path.join(os.environ["TMPDIR"], "out.txt")
with open(out, "wb") as f: f.write(data.upper())

print(json.dumps({"files": {"out": {"path": out, "type": "text/plain"}}}))
