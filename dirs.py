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

main()
