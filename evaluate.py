import flip_evaluator as flip
import matplotlib.pyplot as plt
import numpy as np
import csv
import subprocess
import os

IMAGES_DIR = "./build/bin/Release/"

OUTPUT_DIR = "./figures/"

FIELDS_OF_INTEREST = {
        "image_name"          : str,
        "rc_minmax_probes"    : lambda x: x == "true",
        "rc_resolution_factor": int,
        "frame_time"          : float,
        "avg_frame_time"      : float,
        "frame_time_gpu"          : float,
        "avg_frame_time_gpu"      : float,
       }

def do_figure(bilateral_data, minmax_data, title, y_label, x_labels, categories):
    fig, ax1 = plt.subplots(layout="constrained")
    width = 0.25
    x = np.arange(3)

    for i, measurement in enumerate((bilateral_data, minmax_data)):
        offset = i * width
        rects = ax1.bar(x + offset, measurement, width, label=x_labels[i])
        ax1.bar_label(rects, padding=2)
    # labels
    ax1.set_ylabel(y_label)
    ax1.set_title(title)
    ax1.set_xticks(x + width * 0.5, categories)
    ax1.legend()
    return [fig]

def plots(data, title_suffix):
    # organize data
    data.sort(key=lambda row: row["rc_resolution_factor"])

    frame_times_bilateral = []
    avg_frame_times_bilateral = []
    gpu_frame_times_bilateral = []
    gpu_avg_frame_times_bilateral = []
    mean_errors_bilateral = []
    ssim_bilateral = []
    resolution_factors_bilateral = []
    frame_times_minmax = []
    avg_frame_times_minmax = []
    gpu_frame_times_minmax = []
    gpu_avg_frame_times_minmax = []
    mean_errors_minmax = []
    ssim_minmax = []
    resolution_factors_minmax = []
    for row in data:
        # -- minmax --
        if row["rc_minmax_probes"]:
            frame_times_minmax.append(round(row["frame_time"] * 1000,4))
            avg_frame_times_minmax.append(round(row["avg_frame_time"] * 1000,4))
            gpu_avg_frame_times_minmax.append(round(row["avg_frame_time_gpu"],4))
            gpu_frame_times_minmax.append(round(row["frame_time_gpu"],4))
            mean_errors_minmax.append(round(row["mean_flip_error"],4))
            resolution_factors_minmax.append(row["rc_resolution_factor"])
            ssim_minmax.append(row["ssim_error"])
        # -- bilateral --
        else:
            frame_times_bilateral.append(round(row["frame_time"] * 1000,4))
            avg_frame_times_bilateral.append(round(row["avg_frame_time"] * 1000,4))
            gpu_avg_frame_times_bilateral.append(round(row["avg_frame_time_gpu"],4))
            gpu_frame_times_bilateral.append(round(row["frame_time_gpu"],4))
            mean_errors_bilateral.append(round(row["mean_flip_error"],4))
            resolution_factors_bilateral.append(row["rc_resolution_factor"])
            ssim_bilateral.append(row["ssim_error"])
    
    assert (resolution_factors_minmax == resolution_factors_bilateral) # make sure they're properly sorted

    label_suffixes = ("Bilateral", "Min+Max")
    resolution_factor_categories = tuple(map(lambda x : "1 probe per 4x4 pixels" if x == -1 else "1 probe per 2x2 pixels" if x == 0 else "1 probe per pixel", resolution_factors_minmax))
    
    generated_figs = []
    # plot frame times
    generated_figs += do_figure(
            gpu_avg_frame_times_bilateral,
            gpu_avg_frame_times_minmax,
            f"Average GPU frame times by probe grid resolution ({title_suffix})",
            "Average GPU frame time (ms)",
            label_suffixes, 
            resolution_factor_categories)

    generated_figs += do_figure(
            avg_frame_times_bilateral,
            avg_frame_times_minmax,
            f"Average CPU frame times by probe grid resolution ({title_suffix})",
            "Average CPU frame time (ms)",
            label_suffixes,
            resolution_factor_categories)
    
    # plot flip errors
    f = chr(0xa7fb)
    generated_figs += do_figure(
            mean_errors_bilateral, 
            mean_errors_minmax,
            f"Mean {f}LIP error by probe grid resolution ({title_suffix})",
            f"Mean {f}LIP Error",
            label_suffixes,
            resolution_factor_categories)

    # plot ssim
    generated_figs += do_figure(
            ssim_bilateral,
            ssim_minmax,
            f"Image similarity by probe grid resolution ({title_suffix})",
            "Image similarity (SSIM)",
            label_suffixes,
            resolution_factor_categories)
    return generated_figs
    
ssims = 0
def run_ssim(ref_path, test_path, out_directory):
    global ssims
    if not os.path.exists(out_directory):
        os.makedirs(out_directory)
    out_file = f"{out_directory}/{ssims}.png" 
    res = subprocess.run(["magick", "compare", "-metric", "SSIM",
                          ref_path, test_path, out_file],
                         capture_output=True, text=True)
    ssims += 1
    flat, normalized = res.stderr.split("(")
    flat = flat.strip()
    normalized = normalized.strip(")")
    return float(normalized), out_file


def main():
    with open(IMAGES_DIR + "dump/captures.csv", "r", newline='') as csvfile:
        data = csv.DictReader(csvfile)
        data_list = list(data)
    
    sponza_ref_path = IMAGES_DIR + next(name for row in data_list if "Sponza" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornel_ref_path = IMAGES_DIR + next(name for row in data_list if "cornel" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornellref = flip.load(cornel_ref_path)
    sponzaref = flip.load(sponza_ref_path)
    
    outdicts = [
            {
                key : FIELDS_OF_INTEREST[key](value) 
                for key, value in row.items() 
                    if key in FIELDS_OF_INTEREST.keys()
            } 
        for row in data_list 
            if "ReferencePathTracer" not in row["image_name"]
        ]

    sponza_data = []
    cornel_data = []
    #print(outdicts)
    figures = []
    for i, row in enumerate(outdicts):
        
        # split sponza and cornel measurements
        if "Sponza" in row["image_name"]:
            scene_str = "Sponza"
            ref = sponzaref
            ref_path = sponza_ref_path
            sponza_data.append(row)
        else:
            scene_str = "Cornel Box"
            ref = cornellref
            ref_path = cornel_ref_path
            cornel_data.append(row)
        
        # run flip evaluation
        test_path = IMAGES_DIR + row["image_name"]
        test = flip.load(test_path)
        errormap, meanerror, params = flip.evaluate(ref, test, "HDR")

        outdicts[i]["mean_flip_error"] = meanerror
        outdicts[i]["error_map"] = errormap
        
        # run ssim evaluation
        ssim_error, ssim_error_file = run_ssim(ref_path, test_path, IMAGES_DIR + "ssim_dumps")
        outdicts[i]["ssim_error"] = ssim_error
        outdicts[i]["ssim_error_file"] = ssim_error_file

        # print(" | ".join([f"{key}: {value}" for key, value in outdicts[i].items() if key != "error_map"]))

        fig, axs = plt.subplots(layout="constrained")
        axs.imshow(errormap)
        res_str = lambda x: "quarter res" if x == -1 else "half res" if x == 0 else "full res"
        title = scene_str + "-" + ("Min+Max" if row["rc_minmax_probes"] else "Bilateral") + "-" + "(" + res_str(row["rc_resolution_factor"]) + ")"
        axs.set_title(title)
        axs.axis('off')

        figures.append(fig)

    # print(" | ".join([f"{key}: {value}" for row in sponza_data for key, value in row.items() if key != "error_map"]))
    figures += plots(sponza_data, "Sponza")
    figures += plots(cornel_data, "Cornel Box")
    for f in figures:
        f.savefig(OUTPUT_DIR + f.get_axes()[0].get_title() + ".png", bbox_inches='tight')
    # plt.show()

if __name__ == "__main__":
    main()
