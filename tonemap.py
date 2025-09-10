import numpy as np
import matplotlib.pyplot as plt
import flip_evaluator as flip
import sys

IMAGE = "./wtf.exr.exr"
IMAGE2 = "./wtf2.exr"

def lerp(a, b, t):
    t = t.transpose()
    t = np.stack([t,t,t]).transpose()
    return (1.0 - t) * a + t * b

def tonemap_simple(color):
    peak = np.max(color)
    ratio = color / peak
    color = np.clip(color / (color + 1.0),0.0,1.0)
    blend_amount = luminance(color)
    return lerp((peak / (peak + 1.0)) * ratio, color, blend_amount)

def luminance(color):
    return np.dot(color, np.array([0.2126,0.7152,0.0722]))

def toSRGB(color):
    condlist = [color < 0.0031308, color >= 0.0031308]
    choicelist = [12.92 * color, 1.055 * np.pow(np.abs(color), 1.0 / 2.4) - 0.055]
    return np.select(condlist, choicelist)

def eval_log_contrast_fn(color, epsilon, log_midpoint, contrast):
    logcol = np.log2(color + epsilon)
    adjcol = log_midpoint + (logcol - log_midpoint) * contrast
    return np.fmax(np.exp2(adjcol) - epsilon, np.zeros(adjcol.shape))

def kernel(image, exposure):
    color = image
    color *= np.exp2(exposure)
    color = np.clip(tonemap_simple(color),0.0,1.0)
    color = eval_log_contrast_fn(1.2 * color, 1e-5, 0.18, 1.2)
    color = toSRGB(color)
    
    return color

def main(image_path1=None, image_path2=None):
    exposure = 3.0
    if image_path1 is None:
        image = flip.load(IMAGE)
        image2 = flip.load(IMAGE2)
        out = kernel(image, exposure)
        out2 = kernel(image2, exposure)
        fig, (ax1, ax2) = plt.subplots(2, layout='constrained')
        ax1.imshow(out)
        ax1.axis('off')
        ax2.imshow(out2)
        ax2.axis('off')
    elif image_path2 is None:
        image = flip.load(image_path1)
        out = kernel(image, exposure)
        plt.imshow(out)
        plt.axis('off')
        plt.tight_layout()
    else:
        im1 = flip.load(image_path1)
        im2 = flip.load(image_path2)
        out1 = kernel(im1,exposure)
        out2 = kernel(im2,exposure)
        fig, (ax1, ax2) = plt.subplots(2, layout='constrained')
        ax1.imshow(out1)
        ax2.imshow(out2)
        ax1.axis('off')
        ax2.axis('off')
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) == 2:
        image_path = sys.argv[1]
        main(image_path)
    elif len(sys.argv) == 3:
        im1 = sys.argv[1]
        im2 = sys.argv[2]
        main(im1, im2)
    else:
        main()
