#!/usr/bin/env python3
# Stand-in image resizer: read the file named by `src`, prefix the content
# with a marker including `size`, write the result under $TMPDIR and return
# its path.
import json, os, sys

env = json.load(sys.stdin)
with open(env["src"], "rb") as f: data = f.read()

out = os.path.join(os.environ["TMPDIR"], "out")
with open(out, "wb") as f: f.write(b"RESIZED%d:" % env["size"] + data)

print(json.dumps({"files": {"out": {"path": out, "type": "image/png"}}}))
