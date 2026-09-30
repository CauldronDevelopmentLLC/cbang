#!/usr/bin/env python3
# Makes the bearer token test fixtures: three RSA key pairs, public.pem with
# the first two (as during a key rotation) and the tokens.  The tests use the
# files as committed; run this only to replace them.

import base64, hashlib, hmac, json, os

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa


def b64(data):
  return base64.urlsafe_b64encode(data).rstrip(b'=').decode()


def encode(obj): return b64(json.dumps(obj, separators=(',', ':')).encode())


def sign(key, header, claims):
  data = encode(header) + '.' + encode(claims)
  sig = key.sign(data.encode(), padding.PKCS1v15(), hashes.SHA256())
  return data + '.' + b64(sig)


def write(name, data):
  with open(name, 'w') as f: f.write(data + '\n')


keys = []
for i in (1, 2, 3):
  key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
  keys.append(key)
  write('key%d.key' % i, key.private_bytes(
    serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
    serialization.NoEncryption()).decode().strip())

pub = lambda key: key.public_key().public_bytes(
  serialization.Encoding.PEM,
  serialization.PublicFormat.SubjectPublicKeyInfo).decode().strip()
write('public.pem', pub(keys[0]) + '\n' + pub(keys[1]))

RS256 = {'alg': 'RS256', 'typ': 'JWT'}
claims = {'sub': 'as1-events', 'groups': ['grow-ingest'], 'iat': 1790000000,
          'exp': 4102444800, 'jti': '0123456789abcdef0123456789abcdef'}
valid = sign(keys[0], RS256, claims)

tokens = {
  'valid':     valid,
  'rotated':   sign(keys[1], RS256, dict(claims, sub='as2-events')),
  'other-key': sign(keys[2], RS256, claims),
  'expired':   sign(keys[0], RS256, dict(claims, exp=946684800)),
  'not-yet':   sign(keys[0], RS256, dict(claims, nbf=4102444800)),
  'no-exp':    sign(keys[0], RS256,
                    {k: v for k, v in claims.items() if k != 'exp'}),
  'no-groups': sign(keys[0], RS256,
                    {k: v for k, v in claims.items() if k != 'groups'}),
  'no-sub':    sign(keys[0], RS256,
                    {k: v for k, v in claims.items() if k != 'sub'}),
  # The claims changed after signing
  'tampered':  '.'.join([valid.split('.')[0],
                         encode(dict(claims, groups=['admin'])),
                         valid.split('.')[2]]),
  'alg-none':  encode({'alg': 'none', 'typ': 'JWT'}) + '.' +
               encode(claims) + '.',
  # HMAC keyed with the public key, the classic algorithm confusion attack
  'hs256':     None,
}

data = encode({'alg': 'HS256', 'typ': 'JWT'}) + '.' + encode(claims)
mac = hmac.new(pub(keys[0]).encode(), data.encode(), hashlib.sha256).digest()
tokens['hs256'] = data + '.' + b64(mac)

for name, token in tokens.items(): write(name + '.jwt', token)
