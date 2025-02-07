#import matplotlib.pyplot as plt
import math

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

def index_to_uv_offset(idx, counts):
    s_idx = vec2(idx % counts.x, idx // counts.y) - vec2(1/counts.x, 1/counts.y)
    return s_idx / counts

def direction(pos : vec2, cascade_id : int, num_cascades : int, resolution : vec2):
    probe_size = vec2(math.pow(2, cascade_id))
    probe_pos = pos / probe_size
    probe_center = probe_pos * probe_size + 0.5
    pos_in_probe = vec2(pos.x % probe_size.x, pos.y % probe_size.y)

    dir_count_per_pixel = 4*4

    for d in range(dir_count_per_pixel):
        dir_counts = vec2(math.sqrt(dir_count_per_pixel))
        dir_offset = index_to_uv_offset(d, dir_counts)
        uv_in_probe = (pos_in_probe + 0.5 + dir_offset) / probe_size
        print(uv_in_probe)

def main():
    cascade_count = 5
    cascade_id = 1
    resolution = vec2(8, 8)
    for x in range(resolution.x):
        for y in range(resolution.y):
            pos = vec2(x, y)
            print(f"pos=({x}, {y})")
            direction(pos, cascade_id, cascade_count, resolution)

main()
