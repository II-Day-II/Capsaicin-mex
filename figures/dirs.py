import matplotlib.pyplot as plt
import math
import numpy as np
from vector import vec2, vec3
from spheremaps import ea_octahedral, prep_ax

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
#do_single_probe(0)

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
def visualize_merge():
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
    c0, c1 = probe[0][0]
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.plot([0, c0[0]], [0, c0[1]],[0,c0[2]], c='c', alpha=0.5)
    ax.scatter(*c0, c='b')
    for m in c1:
        e = tuple(map(lambda x: x + 2*x, m))
        ax.plot([m[0], e[0]], [m[1], e[1]],[m[2],e[2]], c='y', alpha=0.5)
        ax.scatter(*m, c='y')
    ax.set_aspect('equal')
    ax.margins(0)
    ax.axis('equal')
    ax.set_xticklabels([])
    ax.set_yticklabels([])
    ax.set_zticklabels([])

    fig.set_layout_engine('tight')

    plt.show()
visualize_merge()
