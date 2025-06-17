import matplotlib.pyplot as plt
import math
import numpy as np
from spheremaps import ea_octahedral

def main():
    fig, ax = plt.subplots(subplot_kw={"projection":"3d"})
    
    ax.set_aspect('equal')

    c0_points = 16
    c1_points = 4 * c0_points

    c0 = ea_octahedral(c0_points)
    c1 = ea_octahedral(c1_points)

    c0_point_index = 0
    c1_point_index = (c0_point_index+1) * 4

    x = [i[0] for i in c0][c0_point_index]
    y = [i[1] for i in c0][c0_point_index]
    z = [i[2] for i in c0][c0_point_index]

    ax.scatter(x, y, z, c='b')

    x = [i[0] for i in c1][c1_point_index:c1_point_index+c0_points]
    y = [i[1] for i in c1][c1_point_index:c1_point_index+c0_points]
    z = [i[2] for i in c1][c1_point_index:c1_point_index+c0_points]

    ax.scatter(x, y, z, c='y')
    plt.show()

if __name__ == "__main__":
    main()
