import flip_evaluator as flip
import matplotlib.pyplot as plt
import numpy as np
import csv

IMAGES_DIR = "./build/bin/Release/"

FIELDS_OF_INTEREST = {
        "image_name"          : str,
        "rc_minmax_probes"    : lambda x: x == "true",
        "rc_resolution_factor": int,
        "frame_time"          : float,
        "avg_frame_time"      : float,
        "frame_time_gpu"          : float,
        "avg_frame_time_gpu"      : float,
       }

def plots(data, title_suffix):
    # plot frame time (y) vs resolution factor (x)
    # for both minmax probes and not minmax probes

    # plot mean error (y) vs resolution factor (x)
    # for both minmax probes and not minmax probes
    
    # organize data

    data.sort(key=lambda row: row["rc_resolution_factor"])

    frame_times_bilateral = []
    avg_frame_times_bilateral = []
    gpu_frame_times_bilateral = []
    gpu_avg_frame_times_bilateral = []
    mean_errors_bilateral = []
    resolution_factors_bilateral = []
    frame_times_minmax = []
    avg_frame_times_minmax = []
    gpu_frame_times_minmax = []
    gpu_avg_frame_times_minmax = []
    mean_errors_minmax = []
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
        # -- bilateral --
        else:
            frame_times_bilateral.append(round(row["frame_time"] * 1000,4))
            avg_frame_times_bilateral.append(round(row["avg_frame_time"] * 1000,4))
            gpu_avg_frame_times_bilateral.append(round(row["avg_frame_time_gpu"],4))
            gpu_frame_times_bilateral.append(round(row["frame_time_gpu"],4))
            mean_errors_bilateral.append(round(row["mean_flip_error"],4))
            resolution_factors_bilateral.append(row["rc_resolution_factor"])
    
    assert (resolution_factors_minmax == resolution_factors_bilateral) # make sure they're properly sorted

    label_suffixes = ("Bilateral", "Min+Max")
    resolution_factor_categories = tuple(map(lambda x : "1 probe per 4x4 pixels" if x == -1 else "1 probe per 2x2 pixels" if x == 0 else "1 probe per pixel", resolution_factors_minmax))

    # plot frame times

    # setup plt
    fig, ax1 = plt.subplots(layout="constrained")
    width = 0.25
    x = np.arange(3)

    for i, measurement in enumerate((gpu_avg_frame_times_bilateral, gpu_avg_frame_times_minmax)):
        offset = i * width
        rects = ax1.bar(x + offset, measurement, width, label=label_suffixes[i])
        ax1.bar_label(rects, padding=2)
    # labels
    ax1.set_ylabel("Average GPU Frame Time (ms)")
    ax1.set_title(f"Average GPU Frame times by probe grid resolution ({title_suffix})")
    ax1.set_xticks(x + width * 0.5, resolution_factor_categories)
    ax1.legend()
    
    fig, ax3 = plt.subplots(layout="constrained")
    for i, measurement in enumerate((avg_frame_times_bilateral, avg_frame_times_minmax)):
        offset = i * width
        rects = ax3.bar(x + offset, measurement, width, label=label_suffixes[i])
        ax3.bar_label(rects, padding=2)
    # labels
    ax3.set_ylabel("Average CPU Frame Time (ms)")
    ax3.set_title(f"Average CPU frame times by probe grid resolution ({title_suffix})")
    ax3.set_xticks(x + width * 0.5, resolution_factor_categories)
    ax3.legend()

    # plot flip errors
    fig, ax2 = plt.subplots(layout="constrained")

    for i, measurement in enumerate((mean_errors_bilateral, mean_errors_minmax)):
        offset = i * width
        rects = ax2.bar(x + offset, measurement, width, label=label_suffixes[i])
        ax2.bar_label(rects, padding=2)
    #labels
    ax2.set_ylabel("Mean FLIP Error")
    ax2.set_title(f"Mean FLIP error by probe grid resolution ({title_suffix})")
    ax2.set_xticks(x + width * 0.5, resolution_factor_categories)
    ax2.legend()


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
    for i, row in enumerate(outdicts):
        
        if "Sponza" in row["image_name"]:
            ref = sponzaref
            sponza_data.append(row)
        else:
            ref = cornellref
            cornel_data.append(row)
        test = flip.load(IMAGES_DIR + row["image_name"])
        errormap, meanerror, params = flip.evaluate(ref, test, "HDR")

        outdicts[i]["mean_flip_error"] = meanerror
        outdicts[i]["error_map"] = errormap
        
        print(" | ".join([f"{key}: {value}" for key, value in outdicts[i].items() if key != "error_map"]))

        # fig, axs = plt.subplots()
        # axs.imshow(errormap)
        # axs.set_title(row["image_name"])
    # print(" | ".join([f"{key}: {value}" for row in sponza_data for key, value in row.items() if key != "error_map"]))
    plots(sponza_data, "Sponza")
    plots(cornel_data, "Cornel Box")
    plt.show()

if __name__ == "__main__":
    main()
