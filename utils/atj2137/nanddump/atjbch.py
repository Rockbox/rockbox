"""ATJ2137 NAND ECC: binary BCH, GF(2^13) prim poly 0x25af, t=8.
Codeword per 512 B sector: data[512] + user[3] (LSB-first bits) then 13 parity bytes
(remainder, x^103 first, each byte bit-reversed). Layout in page spare: 16 B per sector
= user[3] + parity[13]."""
M = 13; N = (1 << M) - 1; T = 8; PRIM = 0x25af
REV = bytes(int(f'{i:08b}'[::-1], 2) for i in range(256))
EXP = [0] * (2 * N); LOG = [0] * (N + 1)
_x = 1
for _i in range(N):
    EXP[_i] = _x; LOG[_x] = _i; _x <<= 1
    if _x >> M: _x ^= PRIM
for _i in range(N, 2 * N): EXP[_i] = EXP[_i - N]
def gmul(a, b): return 0 if a == 0 or b == 0 else EXP[LOG[a] + LOG[b]]
def ginv(a): return EXP[N - LOG[a]]
def pmod(a, g):
    dg = g.bit_length() - 1
    while a and a.bit_length() - 1 >= dg:
        a ^= g << (a.bit_length() - 1 - dg)
    return a
def _gen():
    g = [1]; done = set()
    for i in range(1, 2 * T, 2):
        c = i
        if c in done: continue
        cls = []
        while c not in cls: cls.append(c); c = c * 2 % N
        done.update(cls)
        for r in cls:
            ng = [0] * (len(g) + 1)
            for k, co in enumerate(g):
                ng[k + 1] ^= co; ng[k] ^= gmul(co, EXP[r])
            g = ng
    return sum(c << k for k, c in enumerate(g))
G = _gen(); NPAR = G.bit_length() - 1   # 104
_MASK = (1 << NPAR) - 1
_TAB = [pmod(i << NPAR, G) for i in range(256)]
def encode(data, user):
    # byte-wise LFSR (CRC style) over the reflected message, x^103 first
    r = 0; tab = _TAB; mask = _MASK
    for b in (data + user).translate(REV):
        r = ((r << 8) & mask) ^ tab[(r >> (NPAR - 8)) ^ b]
    return r.to_bytes(13, 'big').translate(REV)
def check(data, user, par):
    """quick path: decode only when parity differs"""
    if encode(data, user) == par:
        return 0, data, user
    return decode(data, user, par)
def decode(data, user, par):
    """returns (nerr, data, user) with errors corrected, nerr=-1 if uncorrectable"""
    msg = bytearray(data + user); nb = len(msg) * 8
    R = int.from_bytes(bytes(msg).translate(REV), 'big') << NPAR | int.from_bytes(par.translate(REV), 'big')
    bits = [i for i in range(R.bit_length()) if R >> i & 1]
    S = []
    for j in range(1, 2 * T + 1):
        s = 0
        for i in bits: s ^= EXP[(i * j) % N]
        S.append(s)
    if not any(S): return 0, data, user
    # Berlekamp-Massey
    C = [1]; B = [1]; L = 0; m = 1; b = 1
    for n in range(2 * T):
        d = S[n]
        for i in range(1, L + 1):
            if i < len(C): d ^= gmul(C[i], S[n - i])
        if d == 0: m += 1; continue
        coef = gmul(d, ginv(b)); Tt = C[:]
        C = C + [0] * (len(B) + m - len(C))
        for i, bi in enumerate(B): C[i + m] ^= gmul(coef, bi)
        if 2 * L <= n: L = n + 1 - L; B = Tt; b = d; m = 1
        else: m += 1
    # Chien over codeword positions (degree i), root of locator at alpha^-i
    total = nb + NPAR; errs = []
    for i in range(total):
        v = 0
        for k, c in enumerate(C[:L + 1]):
            if c: v ^= gmul(c, EXP[(-(i * k)) % N])
        if v == 0: errs.append(i)
    if len(errs) != L: return -1, data, user
    for i in errs:
        if i >= NPAR:
            pos = nb - 1 - (i - NPAR)          # bit index in message, MSB-first order of reflected bytes
            byte, bit = divmod(pos, 8)
            msg[byte] ^= 1 << bit              # message bits are LSB-first, so this is the original bit
    return L, bytes(msg[:len(data)]), bytes(msg[len(data):])

if __name__ == '__main__':
    # self test: encode random sector, inject t errors, decode
    import os, random
    d = os.urandom(512); u = os.urandom(3); p = encode(d, u)
    bad = bytearray(d)
    for pos in random.sample(range(4096), T): bad[pos // 8] ^= 1 << (pos % 8)
    n, d2, u2 = decode(bytes(bad), u, p)
    print('corrected', n, 'errors:', 'OK' if d2 == d and u2 == u else 'FAIL')
