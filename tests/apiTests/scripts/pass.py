#!/usr/bin/env python3
# Read and discard the envelope; return success with no response so the
# pipeline continues to the next statement.
import json, sys

json.load(sys.stdin)
print(json.dumps({}))
