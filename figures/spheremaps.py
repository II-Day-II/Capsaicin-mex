import numpy as np
import math
import matplotlib.pyplot as plt

def prep_ax(ax):
    ax.grid(False)
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_zticks([])
    ax.axis('off')
    ax.set_xlim(-1, 1)
    ax.set_ylim(-1, 1)
    ax.set_zlim(-1, 1)
    ax.margins(0)
    ax.set_aspect('equal')
    # ax.use_sticky_edges = True
    # ax.autoscale(enable=None, axis='both', tight=True)

def normalize(v):
    l = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    return (v[0] / l, v[1] / l, v[2] / l)

def distance(a, b):
    return math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2)

def fibonacci_sphere(samples=1000):

    points = []
    phi = math.pi * (math.sqrt(5.) - 1.)  # golden angle in radians

    for i in range(samples):
        y = 1 - (i / float(samples - 1)) * 2  # y goes from 1 to -1
        radius = math.sqrt(1 - y * y)  # radius at y

        theta = phi * i  # golden angle increment

        x = math.cos(theta) * radius
        z = math.sin(theta) * radius

        points.append((x, y, z))

    return points

def costheta(samples):
    side = int(math.sqrt(samples))
    points = []
    for x in range(side):
        for y in range(side):
            uvx = (x + 0.5) / side
            uvy = (y + 0.5) / side
            phi = uvx * 2 * math.pi
            theta = uvy * math.pi
            points.append((math.sin(theta)*math.sin(phi), math.cos(phi) * math.sin(theta), math.cos(theta)))
    return points

def octahedral(samples):
    side = int(math.sqrt(samples))
    points = []
    for x in range(side):
        for y in range(side):
            uvx = (x + 0.5) / side
            uvy = (y + 0.5) / side

            uvx = 2 * uvx - 1
            uvy = 2 * uvy - 1

            vx = uvx
            vy = uvy
            vz = 1 - abs(vx) - abs(vy)
            if vz < 0:
                vxn = math.copysign(1 - abs(vy), vx)
                vyn = math.copysign(1 - abs(vx), vy)
            else:
                vxn = vx
                vyn = vy
            points.append(normalize((vxn, vyn, vz)))
    return points

def ea_octahedral(samples):
    side = int(math.sqrt(samples))
    points = []
    for x in range(side):
        for y in range(side):
            uvx = (x + 0.5) / side
            uvy = (y + 0.5) / side

            uvx = 2 * uvx - 1
            uvy = 2 * uvy - 1

            vx = abs(uvx)
            vy = abs(uvy)
            sdist = 1 - (vx+vy)
            r = 1 - abs(sdist)
            phi = ((vy-vx)/r+1) * 0.785398
            r_sqr = r * r
            z = math.copysign(1 - r_sqr, sdist)
            cos_phi = math.copysign(math.cos(phi), uvx)
            sin_phi = math.copysign(math.sin(phi), uvy)
            r_scl = r * math.sqrt(2 - r_sqr)

            vxn = cos_phi * r_scl
            vyn = sin_phi * r_scl
            vz = z

            points.append(normalize((vxn, vyn, vz)))

    return points

def riesz_s_energy(points, s):
    tot = 0
    for i in range(len(points)):
        for j in range(i+1, len(points)):
            x = points[i]
            y = points[j]
            if s > 0:
                tot += distance(x, y) ** -s 
            elif s == 0:
                tot += -math.log(distance(x, y))
    return tot

def main():
    c0_points = 16 
    c1_points = 4 * c0_points
    #c0 = fibonacci_sphere(c0_points)
    #c1 = fibonacci_sphere(c1_points)
    #c0 = costheta(c0_points)
    #c1 = costheta(c1_points)
    #c0 = octahedral(c0_points)
    #c1 = octahedral(c1_points)
    # c0 = ea_octahedral(c0_points)
    # c1 = ea_octahedral(c1_points)
    
    funcs = (
            fibonacci_sphere, 
            costheta, 
            octahedral, 
            ea_octahedral)
    
    for sm in funcs[:]:
        c0 = sm(c0_points)
        c1 = sm(c1_points)
    
        # print(riesz_s_energy(c0, 1) / len(c0))
        # print(riesz_s_energy(c1, 1) / len(c1))
    
        fig = plt.figure()
        fig.set_layout_engine('tight')
        ax = fig.add_subplot(111, projection='3d')
        
        OFFSET = 0.000
        
        ax.scatter([i[0] - (OFFSET if i[0] > 0 else -OFFSET) for i in c0], [i[1] - (OFFSET if i[1] > 0 else -OFFSET) for i in c0], [i[2] - (OFFSET if i[2] > 0 else -OFFSET) for i in c0], color='b')
        ax.scatter([i[0] for i in c1], [i[1] for i in c1], [i[2] for i in c1], color='y')
        
        ax.plot([i[0] for i in c0][:], [i[1] for i in c0][:], np.array([i[2] for i in c0][:]), color='c', alpha=0.2)
        ax.plot([i[0] for i in c1][:], [i[1] for i in c1][:], np.array([i[2] for i in c1][:]), color='y', alpha=0.2)
    
        
        prep_ax(ax)
    
    plt.show()

if __name__ == "__main__":
    main()
