import matplotlib.pyplot as plt
import math
import numpy as np

class vec2:
    def __init__(self, x, y = None):
        if y is None:
            self.x = x
            self.y = x
        else:
            self.x = x
            self.y = y

    def __add__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x + other.x, self.y + other.y)
        else:
            return vec2(self.x + other, self.y + other)

    def __sub__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x - other.x, self.y - other.y)
        else:
            return vec2(self.x - other, self.y - other)

    def __mul__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x * other.x, self.y * other.y)
        else:
            return vec2(self.x * other, self.y * other)

    def __floordiv__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x // other.x, self.y // other.y)
        else:
            return vec2(self.x // other, self.y // other)

    def __truediv__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x / other.x, self.y / other.y)
        else:
            return vec2(self.x / other, self.y / other)

    def __str__(self):
        return f"vec2({self.x}, {self.y}))"

    def dot(self, other):
        return self.x * other.x + self.y * other.y

    def length(self):
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def normalize(self):
        return self / self.length()

    def frac(self):
        x_i, x_f = math.modf(self.x)
        y_i, y_f = math.modf(self.y)
        return vec2(x_f, y_f)

class vec3(vec2):
    def __init__(self, x, y=None, z=None):
        super().__init__(x, y)
        if z is None:
            self.z = x
        else:
            self.z = z

    def length(self):
        return math.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)
    
    def normalize(self):
        return self / self.length()

    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z

    def __truediv__(self, other):
        if isinstance(other, vec3):
            return vec3(self.x / other.x, self.y / other.y, self.z / other.z)
        else:
            return vec3(self.x / other, self.y / other, self.z / other)

    def __str__(self):
        return f"vec3({self.x}, {self.y}, {self.z})"


def index_to_uv_offset(idx, counts):
    s_idx = vec2(idx % counts.x, idx // counts.y) + 0.5 - vec2(counts.x * 0.5, 0.5 * counts.y)
    return s_idx / counts

def octahedral_unmap(uv):
    uv = uv.frac()
    uv = uv * 2 - 1
    v = vec3(uv.x, uv.y, 1 - abs(uv.x) - abs(uv.y))
    if v.z < 0:
        v.x = (1-abs(v.y))*math.copysign(1, v.x)
        v.y = (1-abs(v.x))*math.copysign(1, v.y)
    return v.normalize()

def direction(pos : vec2, cascade_id : int, num_cascades : int, resolution : vec2):
    probe_size = vec2(math.pow(2, cascade_id))
    probe_pos = pos / probe_size
    probe_center = probe_pos * probe_size + 0.5
    pos_in_probe = vec2(pos.x % probe_size.x, pos.y % probe_size.y)

    dir_count_per_pixel = 4*4

    uvs = []
    dirs = []
    for d in range(dir_count_per_pixel):
        dir_counts = vec2(math.sqrt(dir_count_per_pixel))
        dir_offset = index_to_uv_offset(d, dir_counts)
        uv_in_probe = (pos_in_probe + 0.5 + dir_offset) / probe_size
        uvs.append(uv_in_probe)
        direction = octahedral_unmap(uv_in_probe)
        dirs.append(direction)

    return uvs, dirs


def main():
    cascade_count = 5
    cascade_id = 1
    resolution = vec2(8, 8)
    probe_size = math.pow(2, cascade_id)
    for x in range(resolution.x):
        for y in range(resolution.y):
            pos = vec2(x, y)
            print(f"pos=({x}, {y})")
            uvs, dirs = direction(pos, cascade_id, cascade_count, resolution)
            uvr = np.array([[uv.x for uv in uvs[i:i+4]] for i in range(4)])
            uvg = np.array([[uv.y for uv in uvs[i:i+4]] for i in range(4)])
            dirsr = np.array([[d.x for d in dirs[i:i+4]] for i in range(4)])
            dirsg = np.array([[d.y for d in dirs[i:i+4]] for i in range(4)])
            dirsb = np.array([[d.z for d in dirs[i:i+4]] for i in range(4)])
            plt.imshow(uvr)
            #print(dirs)

    plt.show()

#main()
# positions_str = """
# 0, 0: (0.03, 0.03)
# 0, 0: (0.09, 0.03)
# 0, 0: (0.16, 0.03)
# 0, 0: (0.22, 0.03)
# 0, 0: (0.03, 0.09)
# 0, 0: (0.09, 0.09)
# 0, 0: (0.16, 0.09)
# 0, 0: (0.22, 0.09)
# 0, 0: (0.03, 0.16)
# 0, 0: (0.09, 0.16)
# 0, 0: (0.16, 0.16)
# 0, 0: (0.22, 0.16)
# 0, 0: (0.03, 0.22)
# 0, 0: (0.09, 0.22)
# 0, 0: (0.16, 0.22)
# 0, 0: (0.22, 0.22)
# 1, 0: (0.28, 0.03)
# 1, 0: (0.34, 0.03)
# 1, 0: (0.41, 0.03)
# 1, 0: (0.47, 0.03)
# 1, 0: (0.28, 0.09)
# 1, 0: (0.34, 0.09)
# 1, 0: (0.41, 0.09)
# 1, 0: (0.47, 0.09)
# 1, 0: (0.28, 0.16)
# 1, 0: (0.34, 0.16)
# 1, 0: (0.41, 0.16)
# 1, 0: (0.47, 0.16)
# 1, 0: (0.28, 0.22)
# 1, 0: (0.34, 0.22)
# 1, 0: (0.41, 0.22)
# 1, 0: (0.47, 0.22)
# 2, 0: (0.53, 0.03)
# 2, 0: (0.59, 0.03)
# 2, 0: (0.66, 0.03)
# 2, 0: (0.72, 0.03)
# 2, 0: (0.53, 0.09)
# 2, 0: (0.59, 0.09)
# 2, 0: (0.66, 0.09)
# 2, 0: (0.72, 0.09)
# 2, 0: (0.53, 0.16)
# 2, 0: (0.59, 0.16)
# 2, 0: (0.66, 0.16)
# 2, 0: (0.72, 0.16)
# 2, 0: (0.53, 0.22)
# 2, 0: (0.59, 0.22)
# 2, 0: (0.66, 0.22)
# 2, 0: (0.72, 0.22)
# 3, 0: (0.78, 0.03)
# 3, 0: (0.84, 0.03)
# 3, 0: (0.91, 0.03)
# 3, 0: (0.97, 0.03)
# 3, 0: (0.78, 0.09)
# 3, 0: (0.84, 0.09)
# 3, 0: (0.91, 0.09)
# 3, 0: (0.97, 0.09)
# 3, 0: (0.78, 0.16)
# 3, 0: (0.84, 0.16)
# 3, 0: (0.91, 0.16)
# 3, 0: (0.97, 0.16)
# 3, 0: (0.78, 0.22)
# 3, 0: (0.84, 0.22)
# 3, 0: (0.91, 0.22)
# 3, 0: (0.97, 0.22)
# 0, 1: (0.03, 0.28)
# 0, 1: (0.09, 0.28)
# 0, 1: (0.16, 0.28)
# 0, 1: (0.22, 0.28)
# 0, 1: (0.03, 0.34)
# 0, 1: (0.09, 0.34)
# 0, 1: (0.16, 0.34)
# 0, 1: (0.22, 0.34)
# 0, 1: (0.03, 0.41)
# 0, 1: (0.09, 0.41)
# 0, 1: (0.16, 0.41)
# 0, 1: (0.22, 0.41)
# 0, 1: (0.03, 0.47)
# 0, 1: (0.09, 0.47)
# 0, 1: (0.16, 0.47)
# 0, 1: (0.22, 0.47)
# 1, 1: (0.28, 0.28)
# 1, 1: (0.34, 0.28)
# 1, 1: (0.41, 0.28)
# 1, 1: (0.47, 0.28)
# 1, 1: (0.28, 0.34)
# 1, 1: (0.34, 0.34)
# 1, 1: (0.41, 0.34)
# 1, 1: (0.47, 0.34)
# 1, 1: (0.28, 0.41)
# 1, 1: (0.34, 0.41)
# 1, 1: (0.41, 0.41)
# 1, 1: (0.47, 0.41)
# 1, 1: (0.28, 0.47)
# 1, 1: (0.34, 0.47)
# 1, 1: (0.41, 0.47)
# 1, 1: (0.47, 0.47)
# 2, 1: (0.53, 0.28)
# 2, 1: (0.59, 0.28)
# 2, 1: (0.66, 0.28)
# 2, 1: (0.72, 0.28)
# 2, 1: (0.53, 0.34)
# 2, 1: (0.59, 0.34)
# 2, 1: (0.66, 0.34)
# 2, 1: (0.72, 0.34)
# 2, 1: (0.53, 0.41)
# 2, 1: (0.59, 0.41)
# 2, 1: (0.66, 0.41)
# 2, 1: (0.72, 0.41)
# 2, 1: (0.53, 0.47)
# 2, 1: (0.59, 0.47)
# 2, 1: (0.66, 0.47)
# 2, 1: (0.72, 0.47)
# 3, 1: (0.78, 0.28)
# 3, 1: (0.84, 0.28)
# 3, 1: (0.91, 0.28)
# 3, 1: (0.97, 0.28)
# 3, 1: (0.78, 0.34)
# 3, 1: (0.84, 0.34)
# 3, 1: (0.91, 0.34)
# 3, 1: (0.97, 0.34)
# 3, 1: (0.78, 0.41)
# 3, 1: (0.84, 0.41)
# 3, 1: (0.91, 0.41)
# 3, 1: (0.97, 0.41)
# 3, 1: (0.78, 0.47)
# 3, 1: (0.84, 0.47)
# 3, 1: (0.91, 0.47)
# 3, 1: (0.97, 0.47)
# 0, 2: (0.03, 0.53)
# 0, 2: (0.09, 0.53)
# 0, 2: (0.16, 0.53)
# 0, 2: (0.22, 0.53)
# 0, 2: (0.03, 0.59)
# 0, 2: (0.09, 0.59)
# 0, 2: (0.16, 0.59)
# 0, 2: (0.22, 0.59)
# 0, 2: (0.03, 0.66)
# 0, 2: (0.09, 0.66)
# 0, 2: (0.16, 0.66)
# 0, 2: (0.22, 0.66)
# 0, 2: (0.03, 0.72)
# 0, 2: (0.09, 0.72)
# 0, 2: (0.16, 0.72)
# 0, 2: (0.22, 0.72)
# 1, 2: (0.28, 0.53)
# 1, 2: (0.34, 0.53)
# 1, 2: (0.41, 0.53)
# 1, 2: (0.47, 0.53)
# 1, 2: (0.28, 0.59)
# 1, 2: (0.34, 0.59)
# 1, 2: (0.41, 0.59)
# 1, 2: (0.47, 0.59)
# 1, 2: (0.28, 0.66)
# 1, 2: (0.34, 0.66)
# 1, 2: (0.41, 0.66)
# 1, 2: (0.47, 0.66)
# 1, 2: (0.28, 0.72)
# 1, 2: (0.34, 0.72)
# 1, 2: (0.41, 0.72)
# 1, 2: (0.47, 0.72)
# 2, 2: (0.53, 0.53)
# 2, 2: (0.59, 0.53)
# 2, 2: (0.66, 0.53)
# 2, 2: (0.72, 0.53)
# 2, 2: (0.53, 0.59)
# 2, 2: (0.59, 0.59)
# 2, 2: (0.66, 0.59)
# 2, 2: (0.72, 0.59)
# 2, 2: (0.53, 0.66)
# 2, 2: (0.59, 0.66)
# 2, 2: (0.66, 0.66)
# 2, 2: (0.72, 0.66)
# 2, 2: (0.53, 0.72)
# 2, 2: (0.59, 0.72)
# 2, 2: (0.66, 0.72)
# 2, 2: (0.72, 0.72)
# 3, 2: (0.78, 0.53)
# 3, 2: (0.84, 0.53)
# 3, 2: (0.91, 0.53)
# 3, 2: (0.97, 0.53)
# 3, 2: (0.78, 0.59)
# 3, 2: (0.84, 0.59)
# 3, 2: (0.91, 0.59)
# 3, 2: (0.97, 0.59)
# 3, 2: (0.78, 0.66)
# 3, 2: (0.84, 0.66)
# 3, 2: (0.91, 0.66)
# 3, 2: (0.97, 0.66)
# 3, 2: (0.78, 0.72)
# 3, 2: (0.84, 0.72)
# 3, 2: (0.91, 0.72)
# 3, 2: (0.97, 0.72)
# 0, 3: (0.03, 0.78)
# 0, 3: (0.09, 0.78)
# 0, 3: (0.16, 0.78)
# 0, 3: (0.22, 0.78)
# 0, 3: (0.03, 0.84)
# 0, 3: (0.09, 0.84)
# 0, 3: (0.16, 0.84)
# 0, 3: (0.22, 0.84)
# 0, 3: (0.03, 0.91)
# 0, 3: (0.09, 0.91)
# 0, 3: (0.16, 0.91)
# 0, 3: (0.22, 0.91)
# 0, 3: (0.03, 0.97)
# 0, 3: (0.09, 0.97)
# 0, 3: (0.16, 0.97)
# 0, 3: (0.22, 0.97)
# 1, 3: (0.28, 0.78)
# 1, 3: (0.34, 0.78)
# 1, 3: (0.41, 0.78)
# 1, 3: (0.47, 0.78)
# 1, 3: (0.28, 0.84)
# 1, 3: (0.34, 0.84)
# 1, 3: (0.41, 0.84)
# 1, 3: (0.47, 0.84)
# 1, 3: (0.28, 0.91)
# 1, 3: (0.34, 0.91)
# 1, 3: (0.41, 0.91)
# 1, 3: (0.47, 0.91)
# 1, 3: (0.28, 0.97)
# 1, 3: (0.34, 0.97)
# 1, 3: (0.41, 0.97)
# 1, 3: (0.47, 0.97)
# 2, 3: (0.53, 0.78)
# 2, 3: (0.59, 0.78)
# 2, 3: (0.66, 0.78)
# 2, 3: (0.72, 0.78)
# 2, 3: (0.53, 0.84)
# 2, 3: (0.59, 0.84)
# 2, 3: (0.66, 0.84)
# 2, 3: (0.72, 0.84)
# 2, 3: (0.53, 0.91)
# 2, 3: (0.59, 0.91)
# 2, 3: (0.66, 0.91)
# 2, 3: (0.72, 0.91)
# 2, 3: (0.53, 0.97)
# 2, 3: (0.59, 0.97)
# 2, 3: (0.66, 0.97)
# 2, 3: (0.72, 0.97)
# 3, 3: (0.78, 0.78)
# 3, 3: (0.84, 0.78)
# 3, 3: (0.91, 0.78)
# 3, 3: (0.97, 0.78)
# 3, 3: (0.78, 0.84)
# 3, 3: (0.84, 0.84)
# 3, 3: (0.91, 0.84)
# 3, 3: (0.97, 0.84)
# 3, 3: (0.78, 0.91)
# 3, 3: (0.84, 0.91)
# 3, 3: (0.91, 0.91)
# 3, 3: (0.97, 0.91)
# 3, 3: (0.78, 0.97)
# 3, 3: (0.84, 0.97)
# 3, 3: (0.91, 0.97)
# 3, 3: (0.97, 0.97)"""
#
# def costheta_unmap(uv):
#     phi = uv.x * 2 * math.pi
#     theta = uv.y * math.pi
#     return (math.sin(phi) * math.sin(theta), math.cos(phi) * math.sin(theta), math.cos(theta))
#
# ex, ey in [-1,1]
def oct_decode(ex, ey):
    vx = abs(ex); vy = abs(ey)
    sdist = 1 - (vx + vy)
    r = 1 - abs(sdist)
    phi = (1 if r == 0 else (vy - vx) / r + 1) * math.pi / 4
    r2 = r * r
    z = math.copysign(1 - r2, sdist)
    cosp = math.copysign(math.cos(phi), ex)
    sinp = math.copysign(math.sin(phi), ey)
    rscl = r * math.sqrt(2 - r2)
    return (cosp * rscl, sinp * rscl, z)
#
# def frac(x):
#     return x - int(x)
# # ex, ey in [0,1]
# def oct_unmap(ex, ey):
#     ex = frac(ex); ey = frac(ey)
#     ex = 2 * ex - 1; ey = 2 * ey - 1
#     vx = ex; vy = ey; vz = 1 - abs(ex) - abs(ey)
#     if vz < 0:
#         vx = math.copysign(1-abs(vy), vx)
#         vy = math.copysign(1-abs(vx), vy)
#     l = math.sqrt(vx ** 2 + vy ** 2 + vz ** 2)
#     return (vx / l, vy / l, vz / l)
#
# pos_s = []
# for line in positions_str.split("\n"):
#     if not line: continue
#     p_id = (int(line[0]), int(line[3]))
#     p_s = line.index("(")
#     p_e = line.index(")")
#     p = list(map(float, line[p_s+1:p_e].split(",")))
#     c = list(map(lambda x: x/4, p_id))
#     pos_s.append((c, p))
#
# xs = [p[0] for _, p in pos_s]
# ys = [p[1] for _, p in pos_s]
# cols = [(c[0],c[1],0) for c, _ in pos_s]
# cols = [tuple(map(lambda x: x if x > 0 else -x, costheta_unmap(vec2(xs[i], ys[i])))) for i in range(len(xs))]
# cols = [tuple(map(lambda x: x if x > 0 else 0, oct_decode(xs[i] * 2 - 1, ys[i] * 2 - 1))) for i in range(len(xs))]
# cols2 = [tuple(map(lambda x: x if x > 0 else 0, oct_unmap(xs[i], ys[i]))) for i in range(len(xs))]
#
# fig, (ax1, ax2) = plt.subplots(2)
# ax1.scatter(xs, ys, c=cols)
# ax2.scatter(xs, ys, c=cols2)
# plt.show()


def idx2uv(d, dir_counts, pip, probe_size):
    offset = index_to_uv_offset(d, dir_counts)
    uv = (pip + 0.5 + offset) / probe_size
    return uv

def idx2dir(d, dir_counts, pip, probe_size):
    offset = index_to_uv_offset(d, dir_counts)
    uv = (pip + 0.5 + offset) / probe_size
    uv = uv * 2 - 1
    # return uv
    return oct_decode(uv.x, uv.y)

def merge_targets(src_pos, probe_size, pip, d):
    ret = []
    ups = probe_size * 2
    
    # nup = src_pos // 2
    # piup = vec2(src_pos.x % ups, src_pos.y % ups)
    # uptl = nup - (vec2(1,1) - piup)

    uptl = (((src_pos + 0.5) * probe_size) / ups - 0.5) // 1

    ldi = (pip.x + pip.y * probe_size) * 4 # -> [0,4,8,12]
    udi = ldi + d # [0,1,2,3|4,5,6,7|8,9,10,11|12,13,14,15]
    # have: [00,10,20,30|01,11,12,13|02,12,22,23|03,13,23,33]
    do = vec2(udi % ups, udi // ups) 
    # want: [00,10,01,11|20,30,21,31|02,12,03,13|22,32,23,33]
    do = (vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1))[d] + pip * 2 
    # return uptl * ups + do
    for i in range(4):
        offset = (vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1))[i]
        upid = uptl + offset
        upst = upid * ups
        st = upst + do
        ret.append(st)
    return ret


def do_single_probe(cascade_idx):
    probe_size = 2#1 << (cascade_idx + 1)
    upper_probe_size = probe_size * 2
    probe = [] # [[(t, [m;4]);4];ps*ps]
    for x in range(probe_size):
        for y in range(probe_size):
            pip = vec2(x, y)
            probe_dirs = []
            for d in range(4):
                direction = idx2dir(d, vec2(2,2), pip, probe_size)
                mt = merge_targets(vec2(probe_size,probe_size), probe_size, pip, d)[0]
                pos_in_upper_probe = vec2(mt.x % upper_probe_size, mt.y % upper_probe_size)
                merge_dirs = [idx2dir(md, vec2(2,2), pos_in_upper_probe, upper_probe_size) for md in range(4)]
                 
                probe_dirs.append((direction, merge_dirs))
            probe.append(probe_dirs)
    colors = ["red",      "lime",      "blue", "black", 
              "deeppink", "green",     "cyan", "grey"]
    def colorfrom3tup(x):
        x = list(map(lambda e: e * 0.5 + 0.5, x))
        r = int(x[0] * 255)
        g = int(x[1] * 255)
        b = int(x[2] * 255)
        return f"#{r:02X}{g:02X}{b:02X}"
    colors2 = [colorfrom3tup(probe[g][d][0]) for g in range(probe_size * probe_size) for d in range(4)]
    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")
    for g in range(probe_size*probe_size):
        # ax = fig.add_subplot(probe_size, probe_size, 1+g, projection="3d")
        # ax = fig.add_subplot(probe_size, probe_size, 1+g)
        boundsx = (-1, -1, -1, -1,  1,  1,  1,  1)
        boundsy = (-1, -1,  1,  1, -1, -1,  1,  1)
        boundsz = (-1,  1, -1,  1, -1,  1, -1,  1)
        ax.scatter(boundsx, boundsy, boundsz, c="white")
        # ax.scatter(boundsx, boundsy, c="white")
        ax.set_aspect("equal")
        for d in range(4):
            target, merges = probe[g][d]
            # ax.scatter(*target, c=colors[d])
            ax.scatter(*target, c=colors2[g*4+d])
            # ax.scatter(target.x, target.y, c=colors[d])
            for m in merges:
                # ax.scatter(*m, c=colors[d+4])
                ax.scatter(*m, c=colors2[g*4+d])
                # ax.scatter(m.x, m.y, c=colors[d+4])
    
    plt.show()

def show_uvs():
    probe_size = 2
    probe = [] # [[u, v; 4]; ps*ps]
    for x in range(probe_size):
        for y in range(probe_size):
            pip = vec2(x, y)
            probe_uvs = []
            for d in range(4):
                uv = idx2uv(d, vec2(2,2), pip, probe_size)
                probe_uvs.append(uv)
            probe.append(probe_uvs)
    fig = plt.figure()
    ax = fig.add_subplot(111)
    # flat = [i for g in [(uv.x, uv.y, f"#{int(255*uv.x):02X}{int(255*uv.y):02X}00") for d in probe for uv in d] for i in g]
    xs = [uv.x for d in probe for uv in d]
    ys = [uv.y for d in probe for uv in d]
    cs = [f"#{int(255*uv.x):02X}{int(255*uv.y):02X}00" for d in probe for uv in d]
    ax.scatter(xs, ys, c=cs)
    # ax.scatter(flat[0::3], flat[1::3], c=flat[2::3])
    plt.show()

#show_uvs()
do_single_probe(0)

def leak_dirs(ups, pip):
    offsets = [vec2(0,0),vec2(1,0),vec2(0,1),vec2(1,1)]
    dirs = []
    for d in range(4):
        direction = idx2dir(d, vec2(2,2), offsets[d] + pip * 2, ups)
        dirs.append(direction)
    return dirs

def find_out():
    probe_size = 2
    upper_probe_size = probe_size * 2
    probe = [] # [[(t, [m;4]);4];ps*ps]
    for x in range(probe_size):
        for y in range(probe_size):
            pip = vec2(x, y)
            probe_dirs = []
            for d in range(4):
                direction = idx2dir(d, vec2(2,2), pip, probe_size)
                uppers = leak_dirs(upper_probe_size, pip)
                probe_dirs.append((direction, uppers))
            probe.append(probe_dirs)
    colors = ["red",      "lime",      "blue", "black", 
              "deeppink", "green",     "cyan", "grey"]
    def colorfrom3tup(x):
        x = list(map(lambda e: e * 0.5 + 0.5, x))
        r = int(x[0] * 255)
        g = int(x[1] * 255)
        b = int(x[2] * 255)
        return f"#{r:02X}{g:02X}{b:02X}"
    colors2 = [colorfrom3tup(probe[g][d][0]) for g in range(probe_size * probe_size) for d in range(4)]
    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")
    for g in range(probe_size*probe_size):
        # ax = fig.add_subplot(probe_size, probe_size, 1+g, projection="3d")
        # ax = fig.add_subplot(probe_size, probe_size, 1+g)
        boundsx = (-1, -1, -1, -1,  1,  1,  1,  1)
        boundsy = (-1, -1,  1,  1, -1, -1,  1,  1)
        boundsz = (-1,  1, -1,  1, -1,  1, -1,  1)
        ax.scatter(boundsx, boundsy, boundsz, c="white")
        # ax.scatter(boundsx, boundsy, c="white")
        ax.set_aspect("equal")
        for d in range(4):
            target, merges = probe[g][d]
            # ax.scatter(*target, c=colors[d])
            ax.scatter(*target, c=colors2[g*4+d])
            # ax.scatter(target.x, target.y, c=colors[d])
            for m in merges:
                # ax.scatter(*m, c=colors[d+4])
                ax.scatter(*m, c=colors2[g*4+d])
                # ax.scatter(m.x, m.y, c=colors[d+4])
    
    plt.show()

# find_out()

