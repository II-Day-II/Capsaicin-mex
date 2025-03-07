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
    s_idx = vec2(idx % counts.x, idx // counts.y) - vec2(1/counts.x, 1/counts.y)
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
positions_str = """
0, 0: (0.03, 0.03)
0, 0: (0.09, 0.03)
0, 0: (0.16, 0.03)
0, 0: (0.22, 0.03)
0, 0: (0.03, 0.09)
0, 0: (0.09, 0.09)
0, 0: (0.16, 0.09)
0, 0: (0.22, 0.09)
0, 0: (0.03, 0.16)
0, 0: (0.09, 0.16)
0, 0: (0.16, 0.16)
0, 0: (0.22, 0.16)
0, 0: (0.03, 0.22)
0, 0: (0.09, 0.22)
0, 0: (0.16, 0.22)
0, 0: (0.22, 0.22)
1, 0: (0.28, 0.03)
1, 0: (0.34, 0.03)
1, 0: (0.41, 0.03)
1, 0: (0.47, 0.03)
1, 0: (0.28, 0.09)
1, 0: (0.34, 0.09)
1, 0: (0.41, 0.09)
1, 0: (0.47, 0.09)
1, 0: (0.28, 0.16)
1, 0: (0.34, 0.16)
1, 0: (0.41, 0.16)
1, 0: (0.47, 0.16)
1, 0: (0.28, 0.22)
1, 0: (0.34, 0.22)
1, 0: (0.41, 0.22)
1, 0: (0.47, 0.22)
2, 0: (0.53, 0.03)
2, 0: (0.59, 0.03)
2, 0: (0.66, 0.03)
2, 0: (0.72, 0.03)
2, 0: (0.53, 0.09)
2, 0: (0.59, 0.09)
2, 0: (0.66, 0.09)
2, 0: (0.72, 0.09)
2, 0: (0.53, 0.16)
2, 0: (0.59, 0.16)
2, 0: (0.66, 0.16)
2, 0: (0.72, 0.16)
2, 0: (0.53, 0.22)
2, 0: (0.59, 0.22)
2, 0: (0.66, 0.22)
2, 0: (0.72, 0.22)
3, 0: (0.78, 0.03)
3, 0: (0.84, 0.03)
3, 0: (0.91, 0.03)
3, 0: (0.97, 0.03)
3, 0: (0.78, 0.09)
3, 0: (0.84, 0.09)
3, 0: (0.91, 0.09)
3, 0: (0.97, 0.09)
3, 0: (0.78, 0.16)
3, 0: (0.84, 0.16)
3, 0: (0.91, 0.16)
3, 0: (0.97, 0.16)
3, 0: (0.78, 0.22)
3, 0: (0.84, 0.22)
3, 0: (0.91, 0.22)
3, 0: (0.97, 0.22)
0, 1: (0.03, 0.28)
0, 1: (0.09, 0.28)
0, 1: (0.16, 0.28)
0, 1: (0.22, 0.28)
0, 1: (0.03, 0.34)
0, 1: (0.09, 0.34)
0, 1: (0.16, 0.34)
0, 1: (0.22, 0.34)
0, 1: (0.03, 0.41)
0, 1: (0.09, 0.41)
0, 1: (0.16, 0.41)
0, 1: (0.22, 0.41)
0, 1: (0.03, 0.47)
0, 1: (0.09, 0.47)
0, 1: (0.16, 0.47)
0, 1: (0.22, 0.47)
1, 1: (0.28, 0.28)
1, 1: (0.34, 0.28)
1, 1: (0.41, 0.28)
1, 1: (0.47, 0.28)
1, 1: (0.28, 0.34)
1, 1: (0.34, 0.34)
1, 1: (0.41, 0.34)
1, 1: (0.47, 0.34)
1, 1: (0.28, 0.41)
1, 1: (0.34, 0.41)
1, 1: (0.41, 0.41)
1, 1: (0.47, 0.41)
1, 1: (0.28, 0.47)
1, 1: (0.34, 0.47)
1, 1: (0.41, 0.47)
1, 1: (0.47, 0.47)
2, 1: (0.53, 0.28)
2, 1: (0.59, 0.28)
2, 1: (0.66, 0.28)
2, 1: (0.72, 0.28)
2, 1: (0.53, 0.34)
2, 1: (0.59, 0.34)
2, 1: (0.66, 0.34)
2, 1: (0.72, 0.34)
2, 1: (0.53, 0.41)
2, 1: (0.59, 0.41)
2, 1: (0.66, 0.41)
2, 1: (0.72, 0.41)
2, 1: (0.53, 0.47)
2, 1: (0.59, 0.47)
2, 1: (0.66, 0.47)
2, 1: (0.72, 0.47)
3, 1: (0.78, 0.28)
3, 1: (0.84, 0.28)
3, 1: (0.91, 0.28)
3, 1: (0.97, 0.28)
3, 1: (0.78, 0.34)
3, 1: (0.84, 0.34)
3, 1: (0.91, 0.34)
3, 1: (0.97, 0.34)
3, 1: (0.78, 0.41)
3, 1: (0.84, 0.41)
3, 1: (0.91, 0.41)
3, 1: (0.97, 0.41)
3, 1: (0.78, 0.47)
3, 1: (0.84, 0.47)
3, 1: (0.91, 0.47)
3, 1: (0.97, 0.47)
0, 2: (0.03, 0.53)
0, 2: (0.09, 0.53)
0, 2: (0.16, 0.53)
0, 2: (0.22, 0.53)
0, 2: (0.03, 0.59)
0, 2: (0.09, 0.59)
0, 2: (0.16, 0.59)
0, 2: (0.22, 0.59)
0, 2: (0.03, 0.66)
0, 2: (0.09, 0.66)
0, 2: (0.16, 0.66)
0, 2: (0.22, 0.66)
0, 2: (0.03, 0.72)
0, 2: (0.09, 0.72)
0, 2: (0.16, 0.72)
0, 2: (0.22, 0.72)
1, 2: (0.28, 0.53)
1, 2: (0.34, 0.53)
1, 2: (0.41, 0.53)
1, 2: (0.47, 0.53)
1, 2: (0.28, 0.59)
1, 2: (0.34, 0.59)
1, 2: (0.41, 0.59)
1, 2: (0.47, 0.59)
1, 2: (0.28, 0.66)
1, 2: (0.34, 0.66)
1, 2: (0.41, 0.66)
1, 2: (0.47, 0.66)
1, 2: (0.28, 0.72)
1, 2: (0.34, 0.72)
1, 2: (0.41, 0.72)
1, 2: (0.47, 0.72)
2, 2: (0.53, 0.53)
2, 2: (0.59, 0.53)
2, 2: (0.66, 0.53)
2, 2: (0.72, 0.53)
2, 2: (0.53, 0.59)
2, 2: (0.59, 0.59)
2, 2: (0.66, 0.59)
2, 2: (0.72, 0.59)
2, 2: (0.53, 0.66)
2, 2: (0.59, 0.66)
2, 2: (0.66, 0.66)
2, 2: (0.72, 0.66)
2, 2: (0.53, 0.72)
2, 2: (0.59, 0.72)
2, 2: (0.66, 0.72)
2, 2: (0.72, 0.72)
3, 2: (0.78, 0.53)
3, 2: (0.84, 0.53)
3, 2: (0.91, 0.53)
3, 2: (0.97, 0.53)
3, 2: (0.78, 0.59)
3, 2: (0.84, 0.59)
3, 2: (0.91, 0.59)
3, 2: (0.97, 0.59)
3, 2: (0.78, 0.66)
3, 2: (0.84, 0.66)
3, 2: (0.91, 0.66)
3, 2: (0.97, 0.66)
3, 2: (0.78, 0.72)
3, 2: (0.84, 0.72)
3, 2: (0.91, 0.72)
3, 2: (0.97, 0.72)
0, 3: (0.03, 0.78)
0, 3: (0.09, 0.78)
0, 3: (0.16, 0.78)
0, 3: (0.22, 0.78)
0, 3: (0.03, 0.84)
0, 3: (0.09, 0.84)
0, 3: (0.16, 0.84)
0, 3: (0.22, 0.84)
0, 3: (0.03, 0.91)
0, 3: (0.09, 0.91)
0, 3: (0.16, 0.91)
0, 3: (0.22, 0.91)
0, 3: (0.03, 0.97)
0, 3: (0.09, 0.97)
0, 3: (0.16, 0.97)
0, 3: (0.22, 0.97)
1, 3: (0.28, 0.78)
1, 3: (0.34, 0.78)
1, 3: (0.41, 0.78)
1, 3: (0.47, 0.78)
1, 3: (0.28, 0.84)
1, 3: (0.34, 0.84)
1, 3: (0.41, 0.84)
1, 3: (0.47, 0.84)
1, 3: (0.28, 0.91)
1, 3: (0.34, 0.91)
1, 3: (0.41, 0.91)
1, 3: (0.47, 0.91)
1, 3: (0.28, 0.97)
1, 3: (0.34, 0.97)
1, 3: (0.41, 0.97)
1, 3: (0.47, 0.97)
2, 3: (0.53, 0.78)
2, 3: (0.59, 0.78)
2, 3: (0.66, 0.78)
2, 3: (0.72, 0.78)
2, 3: (0.53, 0.84)
2, 3: (0.59, 0.84)
2, 3: (0.66, 0.84)
2, 3: (0.72, 0.84)
2, 3: (0.53, 0.91)
2, 3: (0.59, 0.91)
2, 3: (0.66, 0.91)
2, 3: (0.72, 0.91)
2, 3: (0.53, 0.97)
2, 3: (0.59, 0.97)
2, 3: (0.66, 0.97)
2, 3: (0.72, 0.97)
3, 3: (0.78, 0.78)
3, 3: (0.84, 0.78)
3, 3: (0.91, 0.78)
3, 3: (0.97, 0.78)
3, 3: (0.78, 0.84)
3, 3: (0.84, 0.84)
3, 3: (0.91, 0.84)
3, 3: (0.97, 0.84)
3, 3: (0.78, 0.91)
3, 3: (0.84, 0.91)
3, 3: (0.91, 0.91)
3, 3: (0.97, 0.91)
3, 3: (0.78, 0.97)
3, 3: (0.84, 0.97)
3, 3: (0.91, 0.97)
3, 3: (0.97, 0.97)"""

def costheta_unmap(uv):
    phi = uv.x * 2 * math.pi
    theta = uv.y * math.pi
    return (math.sin(phi) * math.sin(theta), math.cos(phi) * math.sin(theta), math.cos(theta))

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

def frac(x):
    return x - int(x)
# ex, ey in [0,1]
def oct_unmap(ex, ey):
    ex = frac(ex); ey = frac(ey)
    ex = 2 * ex - 1; ey = 2 * ey - 1
    vx = ex; vy = ey; vz = 1 - abs(ex) - abs(ey)
    if vz < 0:
        vx = math.copysign(1-abs(vy), vx)
        vy = math.copysign(1-abs(vx), vy)
    l = math.sqrt(vx ** 2 + vy ** 2 + vz ** 2)
    return (vx / l, vy / l, vz / l)

pos_s = []
for line in positions_str.split("\n"):
    if not line: continue
    p_id = (int(line[0]), int(line[3]))
    p_s = line.index("(")
    p_e = line.index(")")
    p = list(map(float, line[p_s+1:p_e].split(",")))
    c = list(map(lambda x: x/4, p_id))
    pos_s.append((c, p))

xs = [p[0] for _, p in pos_s]
ys = [p[1] for _, p in pos_s]
cols = [(c[0],c[1],0) for c, _ in pos_s]
cols = [tuple(map(lambda x: x if x > 0 else -x, costheta_unmap(vec2(xs[i], ys[i])))) for i in range(len(xs))]
cols = [tuple(map(lambda x: x if x > 0 else 0, oct_decode(xs[i] * 2 - 1, ys[i] * 2 - 1))) for i in range(len(xs))]
cols2 = [tuple(map(lambda x: x if x > 0 else 0, oct_unmap(xs[i], ys[i]))) for i in range(len(xs))]

fig, (ax1, ax2) = plt.subplots(2)
ax1.scatter(xs, ys, c=cols)
ax2.scatter(xs, ys, c=cols2)
plt.show()
