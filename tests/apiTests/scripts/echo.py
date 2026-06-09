#!/usr/bin/env python3
# Echo the received metadata envelope back as the response, so tests can
# verify the exec input templating and protocol.
import json, sys

env = json.load(sys.stdin)
print(json.dumps({"response": env}))
