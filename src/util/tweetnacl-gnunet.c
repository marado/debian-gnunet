/*
      This file has been placed in the public domain.

      Based on TweetNaCl version 20140427

      Originally obtained from:
      https://tweetnacl.cr.yp.to/20140427/tweetnacl.h

      SPDX-License-Identifier: 0BSD
*/

#include "platform.h"
#include "gnunet_crypto_lib.h"
#include "tweetnacl-gnunet.h"
#define FOR(i,n) for (i = 0; i < n; ++i)

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef i64 gf[16];

static const u8 _9[32] = {9};
static const gf
  gf0,
  gf1 = {1},
  _121665 = {0xDB41,1},
  D = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070, 0xe898,
       0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203},
  D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0, 0xd130,
        0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406},
  X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c, 0xdc5c,
       0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169},
  Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
       0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666},
  I = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43, 0xd7a7,
       0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83};

static int
vn (const u8 *x,const u8 *y,int n)
{
  u32 i,d = 0;
  FOR (i,n) d |= x[i] ^ y[i];
  return (1 & ((d - 1) >> 8)) - 1;
}

static int
crypto_verify_32 (const u8 *x,const u8 *y)
{
  return vn (x,y,32);
}

static void
set25519 (gf r, const gf a)
{
  int i;
  FOR (i,16) r[i] = a[i];
}

static void
car25519 (gf o)
{
  int i;
  i64 c;
  FOR (i,16) {
    o[i] += (1LL << 16);
    c = o[i] >> 16;
    o[(i + 1) * (i<15)] += c - 1 + 37 * (c - 1) * (i==15);
    o[i] -= c << 16;
  }
}

static void
sel25519 (gf p,gf q,int b)
{
  i64 t,i,c = ~(b - 1);
  FOR (i,16) {
    t = c & (p[i] ^ q[i]);
    p[i] ^= t;
    q[i] ^= t;
  }
}

static void
pack25519 (u8 *o,const gf n)
{
  int i,j,b;
  gf m,t;
  FOR (i,16) t[i] = n[i];
  car25519 (t);
  car25519 (t);
  car25519 (t);
  FOR (j,2) {
    m[0] = t[0] - 0xffed;
    for (i = 1; i<15; i++) {
      m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
      m[i - 1] &= 0xffff;
    }
    m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
    b = (m[15] >> 16) & 1;
    m[14] &= 0xffff;
    sel25519 (t,m,1 - b);
  }
  FOR (i,16) {
    o[2 * i] = t[i] & 0xff;
    o[2 * i + 1] = t[i] >> 8;
  }
}

static int
neq25519 (const gf a, const gf b)
{
  u8 c[32],d[32];
  pack25519 (c,a);
  pack25519 (d,b);
  return crypto_verify_32 (c,d);
}

static uint8_t
par25519 (const gf a)
{
  u8 d[32];
  pack25519 (d,a);
  return d[0] & 1;
}

static void
unpack25519 (gf o, const u8 *n)
{
  int i;
  FOR (i,16) o[i] = n[2 * i] + ((i64) n[2 * i + 1] << 8);
  o[15] &= 0x7fff;
}

static void
A (gf o,const gf a,const gf b)
{
  int i;
  FOR (i,16) o[i] = a[i] + b[i];
}

static void
Z (gf o,const gf a,const gf b)
{
  int i;
  FOR (i,16) o[i] = a[i] - b[i];
}

static void
M (gf o,const gf a,const gf b)
{
  i64 i,j,t[31];
  FOR (i,31) t[i] = 0;
  FOR (i,16) FOR (j,16) t[i + j] += a[i] * b[j];
  FOR (i,15) t[i] += 38 * t[i + 16];
  FOR (i,16) o[i] = t[i];
  car25519 (o);
  car25519 (o);
}

static void
S (gf o,const gf a)
{
  M (o,a,a);
}

static void
inv25519 (gf o,const gf i)
{
  gf c;
  int a;
  FOR (a,16) c[a] = i[a];
  for (a = 253; a>=0; a--) {
    S (c,c);
    if ((a!=2)&&(a!=4))
      M (c,c,i);
  }
  FOR (a,16) o[a] = c[a];
}

static void pow2523 (gf o,const gf i)
{
  gf c;
  int a;
  FOR (a,16) c[a] = i[a];
  for (a = 250; a>=0; a--) {
    S (c,c);
    if (a!=1)
      M (c,c,i);
  }
  FOR (a,16) o[a] = c[a];
}

int
GNUNET_TWEETNACL_scalarmult_curve25519 (u8 *q,const u8 *n,const u8 *p)
{
  u8 z[32];
  i64 x[80],r,i;
  gf a,b,c,d,e,f;
  FOR (i,31) z[i] = n[i];
  z[31] = (n[31] & 127) | 64;
  z[0] &= 248;
  unpack25519 (x,p);
  FOR (i,16) {
    b[i] = x[i];
    d[i] = a[i] = c[i] = 0;
  }
  a[0] = d[0] = 1;
  for (i = 254; i>=0; --i) {
    r = (z[i >> 3] >> (i & 7)) & 1;
    sel25519 (a,b,r);
    sel25519 (c,d,r);
    A (e,a,c);
    Z (a,a,c);
    A (c,b,d);
    Z (b,b,d);
    S (d,e);
    S (f,a);
    M (a,c,a);
    M (c,b,e);
    A (e,a,c);
    Z (a,a,c);
    S (b,a);
    Z (c,d,f);
    M (a,c,_121665);
    A (a,a,d);
    M (c,c,a);
    M (a,d,f);
    M (d,b,x);
    S (b,e);
    sel25519 (a,b,r);
    sel25519 (c,d,r);
  }
  FOR (i,16) {
    x[i + 16] = a[i];
    x[i + 32] = c[i];
    x[i + 48] = b[i];
    x[i + 64] = d[i];
  }
  inv25519 (x + 32,x + 32);
  M (x + 16,x + 16,x + 32);
  pack25519 (q,x + 16);
  return 0;
}

int
GNUNET_TWEETNACL_scalarmult_curve25519_base (u8 *q,const u8 *n)
{
  return GNUNET_TWEETNACL_scalarmult_curve25519 (q,n,_9);
}

static int
crypto_hash (u8 *out,const u8 *m,u64 n)
{
  struct GNUNET_HashCode *hc = (void *) out;
  GNUNET_CRYPTO_hash (m, n, hc);
  return 0;
}

static void
add (gf p[4],gf q[4])
{
  gf a,b,c,d,t,e,f,g,h;

  Z (a, p[1], p[0]);
  Z (t, q[1], q[0]);
  M (a, a, t);
  A (b, p[0], p[1]);
  A (t, q[0], q[1]);
  M (b, b, t);
  M (c, p[3], q[3]);
  M (c, c, D2);
  M (d, p[2], q[2]);
  A (d, d, d);
  Z (e, b, a);
  Z (f, d, c);
  A (g, d, c);
  A (h, b, a);

  M (p[0], e, f);
  M (p[1], h, g);
  M (p[2], g, f);
  M (p[3], e, h);
}

static void
cswap (gf p[4],gf q[4],u8 b)
{
  int i;
  FOR (i,4)
  sel25519 (p[i],q[i],b);
}

static void
pack (u8 *r,gf p[4])
{
  gf tx, ty, zi;
  inv25519 (zi, p[2]);
  M (tx, p[0], zi);
  M (ty, p[1], zi);
  pack25519 (r, ty);
  r[31] ^= par25519 (tx) << 7;
}

static void
scalarmult (gf p[4],gf q[4],const u8 *s)
{
  int i;
  set25519 (p[0],gf0);
  set25519 (p[1],gf1);
  set25519 (p[2],gf1);
  set25519 (p[3],gf0);
  for (i = 255; i >= 0; --i) {
    u8 b = (s[i / 8] >> (i & 7)) & 1;
    cswap (p,q,b);
    add (q,p);
    add (p,p);
    cswap (p,q,b);
  }
}

static void 
scalarbase (gf p[4],const u8 *s)
{
  gf q[4];
  set25519 (q[0],X);
  set25519 (q[1],Y);
  set25519 (q[2],gf1);
  M (q[3],X,Y);
  scalarmult (p,q,s);
}

static const u64 L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6,
                          0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0x10};

static void
modL (u8 *r,i64 x[64])
{
  i64 carry,i,j;
  for (i = 63; i >= 32; --i) {
    carry = 0;
    for (j = i - 32; j < i - 12; ++j) {
      x[j] += carry - 16 * x[i] * L[j - (i - 32)];
      carry = (x[j] + 128) >> 8;
      x[j] -= carry << 8;
    }
    x[j] += carry;
    x[i] = 0;
  }
  carry = 0;
  FOR (j,32) {
    x[j] += carry - (x[31] >> 4) * L[j];
    carry = x[j] >> 8;
    x[j] &= 255;
  }
  FOR (j,32) x[j] -= carry * L[j];
  FOR (i,32) {
    x[i + 1] += x[i] >> 8;
    r[i] = x[i] & 255;
  }
}

static void
reduce (u8 *r)
{
  i64 x[64],i;
  FOR (i,64) x[i] = (u64) r[i];
  FOR (i,64) r[i] = 0;
  modL (r,x);
}

static int
unpackneg (gf r[4],const u8 p[32])
{
  gf t, chk, num, den, den2, den4, den6;
  set25519 (r[2],gf1);
  unpack25519 (r[1],p);
  S (num,r[1]);
  M (den,num,D);
  Z (num,num,r[2]);
  A (den,r[2],den);

  S (den2,den);
  S (den4,den2);
  M (den6,den4,den2);
  M (t,den6,num);
  M (t,t,den);

  pow2523 (t,t);
  M (t,t,num);
  M (t,t,den);
  M (t,t,den);
  M (r[0],t,den);

  S (chk,r[0]);
  M (chk,chk,den);
  if (neq25519 (chk, num))
    M (r[0],r[0],I);

  S (chk,r[0]);
  M (chk,chk,den);
  if (neq25519 (chk, num))
    return -1;

  if (par25519 (r[0]) == (p[31] >> 7))
    Z (r[0],gf0,r[0]);

  M (r[3],r[0],r[1]);
  return 0;
}

/* The following functions have been added for GNUnet */

void
GNUNET_TWEETNACL_sign_pk_from_seed (u8 *pk, const u8 *seed)
{
  u8 d[64];
  gf p[4];

  crypto_hash (d, seed, 32);
  d[0] &= 248;
  d[31] &= 127;
  d[31] |= 64;

  scalarbase (p,d);
  pack (pk,p);
}

void
GNUNET_TWEETNACL_sign_sk_from_seed (u8 *sk, const u8 *seed)
{
  u8 d[64];
  gf p[4];
  u8 pk[32];
  int i;

  crypto_hash (d, seed, 32);
  d[0] &= 248;
  d[31] &= 127;
  d[31] |= 64;

  scalarbase (p,d);
  pack (pk,p);

  FOR (i,32) sk[i] = seed[i];
  FOR (i,32) sk[32 + i] = pk[i];
}

int
GNUNET_TWEETNACL_sign_ed25519_pk_to_curve25519 (u8 *x25519_pk,
                                                const u8 *ed25519_pk)
{
  gf ge_a[4];
  gf x;
  gf one_minus_y;

  if (0 != unpackneg (ge_a, ed25519_pk))
    return -1;

  set25519 (one_minus_y, gf1);
  Z (one_minus_y, one_minus_y, ge_a[1]);

  set25519 (x, gf1);
  A (x, x, ge_a[1]);

  inv25519 (one_minus_y, one_minus_y);
  M (x, x, one_minus_y);
  pack25519 (x25519_pk, x);

  return 0;
}

int GNUNET_TWEETNACL_sign_detached_verify (const u8 *sig,
                                           const u8 *m,
                                           u64 n,
                                           const u8 *pk)
{
  struct GNUNET_HashContext *hc;
  u8 t[32],h[64];
  gf p[4],q[4];

  if (unpackneg (q,pk))
    return -1;

  hc = GNUNET_CRYPTO_hash_context_start ();
  GNUNET_CRYPTO_hash_context_read (hc, sig, 32);
  GNUNET_CRYPTO_hash_context_read (hc, pk, 32);
  GNUNET_CRYPTO_hash_context_read (hc, m, n);
  GNUNET_CRYPTO_hash_context_finish (hc, (void *) h);

  reduce (h);
  scalarmult (p,q,h);

  scalarbase (q,sig+32);
  add (p,q);
  pack (t,p);

  if (crypto_verify_32 (sig, t))
    return -1;
  return 0;
}

int
GNUNET_TWEETNACL_sign_detached (u8 *sig,
                                const u8 *m,
                                u64 n,
                                const u8 *sk)
{
  struct GNUNET_HashContext *hc;
  u8 d[64],h[64],r[64];
  i64 i,j,x[64];
  gf p[4];

  crypto_hash (d, sk, 32);
  d[0] &= 248;
  d[31] &= 127;
  d[31] |= 64;

  hc = GNUNET_CRYPTO_hash_context_start ();
  GNUNET_CRYPTO_hash_context_read (hc, d + 32, 32);
  GNUNET_CRYPTO_hash_context_read (hc, m, n);
  GNUNET_CRYPTO_hash_context_finish (hc, (void *) r);

  reduce (r);
  scalarbase (p,r);
  pack (sig,p);

  hc = GNUNET_CRYPTO_hash_context_start ();
  GNUNET_CRYPTO_hash_context_read (hc, sig, 32);
  GNUNET_CRYPTO_hash_context_read (hc, sk + 32, 32);
  GNUNET_CRYPTO_hash_context_read (hc, m, n);
  GNUNET_CRYPTO_hash_context_finish (hc, (void *) h);

  reduce (h);

  FOR (i,64) x[i] = 0;
  FOR (i,32) x[i] = (u64) r[i];
  FOR (i,32) FOR (j,32) x[i + j] += h[i] * (u64) d[j];
  modL (sig + 32,x);

  return 0;
}
