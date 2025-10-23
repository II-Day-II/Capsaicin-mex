import tonemap
import evaluate
import matplotlib.pyplot as plt
import flip_evaluator as flip
import csv
from skimage.metrics import structural_similarity as ssim

FIELDS_OF_INTEREST = evaluate.FIELDS_OF_INTEREST
IMAGES_DIR = evaluate.IMAGES_DIR

CORNELL_EXPOSURE = 3
SPONZA_EXPOSURE = 6

def plots(data, title):
    data.sort(key=lambda row: row["rc_resolution_factor"])
    flip_bilateral = []
    ssim_bilateral = []
    flip_minmax = []
    ssim_minmax = []
    resolution_factors_bilateral = []
    resolution_factors_minmax = []
    for row in data:
        if row["rc_minmax_probes"]:
            flip_minmax.append(round(row["mean_flip_error"], 4))
            ssim_minmax.append(round(row["ssim_error"], 4))
            resolution_factors_minmax.append(row["rc_resolution_factor"])
        else:
            flip_bilateral.append(round(row["mean_flip_error"],4))
            ssim_bilateral.append(round(row["ssim_error"], 4))
            resolution_factors_bilateral.append(row["rc_resolution_factor"])
    
    assert (resolution_factors_minmax == resolution_factors_bilateral)
    label_suffixes = ("Bilateral", "Min+Max")
    resolution_factor_categories = tuple(map(lambda x : "1 probe per 4x4 pixels" if x == -1 else "1 probe per 2x2 pixels" if x == 0 else "1 probe per pixel", resolution_factors_minmax))

    f = chr(0xa7fb)
    generated_figs = []
    generated_figs += evaluate.do_figure(
            flip_bilateral, 
            flip_minmax,
            f"Mean {f}LIP error by probe grid resolution ({title}) - TONEMAPPED",
            f"Mean {f}LIP Error",
            label_suffixes,
            resolution_factor_categories,
            interval_max=0.6)
    generated_figs += evaluate.do_figure(
            ssim_bilateral,
            ssim_minmax,
            f"Image similarity by probe grid resolution ({title}) - TONEMAPPED",
            "Image similarity (SSIM)",
            label_suffixes,
            resolution_factor_categories,
            interval_max=1)
    return generated_figs

def run_ssim(ref, test):
    mssim, error_map = ssim(test, ref, full=True, data_range=test.max() - test.min(), channel_axis=2, sigma=1.5, gaussian_weights=True, use_sample_covariance=False, K1=0.01, K2=0.03)
    return mssim, error_map

def main():
    with open(IMAGES_DIR + "dump/captures.csv", "r", newline='') as csvfile:
        data = csv.DictReader(csvfile)
        data_list = list(data)
    
    sponza_ref_path = IMAGES_DIR + next(name for row in data_list if "Sponza" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornel_ref_path = IMAGES_DIR + next(name for row in data_list if "cornel" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornellref = flip.load(cornel_ref_path)
    sponzaref = flip.load(sponza_ref_path)
    cornellref = tonemap.kernel(cornellref, CORNELL_EXPOSURE)
    sponzaref = tonemap.kernel(sponzaref, SPONZA_EXPOSURE)

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
            scene_str = "Cornell Box"
            ref = cornellref
            ref_path = cornel_ref_path
            cornel_data.append(row)
        
        # run flip evaluation
        test_path = IMAGES_DIR + row["image_name"]
        test = flip.load(test_path)
        test = tonemap.kernel(test, SPONZA_EXPOSURE if "Sponza" in scene_str else CORNELL_EXPOSURE)
        errormap, meanerror, params = flip.evaluate(ref, test, "LDR")

        outdicts[i]["mean_flip_error"] = meanerror
        outdicts[i]["error_map"] = errormap
        
        # run ssim evaluation
        ssim_error, ssim_error_map = run_ssim(ref, test)
        outdicts[i]["ssim_error"] = ssim_error
        outdicts[i]["ssim_error_map"] = ssim_error_map

        # print(" | ".join([f"{key}: {value}" for key, value in outdicts[i].items() if key != "error_map"]))

        fig, axs = plt.subplots(layout="constrained")
        axs.imshow(errormap)
        res_str = lambda x: "half res" if x == -1 else "full res" if x == 0 else "double res"
        title = f"{chr(0xa7fb)}LIP error - " + scene_str + "-" + ("Min+Max" if row["rc_minmax_probes"] else "Bilateral") + "-" + "(" + res_str(row["rc_resolution_factor"]) + ")"
        axs.set_title(title + " - TONEMAPPED")
        axs.axis('off')

        # fig, axs = plt.subplots(layout="constrained")
        # axs.imshow(ssim_error_map)
        # res_str = lambda x: "half res" if x == -1 else "full res" if x == 0 else "double res"
        # title = "Structured Image Similarity - " + scene_str + "-" + ("Min+Max" if row["rc_minmax_probes"] else "Bilateral") + "-" + "(" + res_str(row["rc_resolution_factor"]) + ")"
        # axs.set_title(title + " - TONEMAPPED")
        # axs.axis('off')

        figures.append(fig)

    # print(" | ".join([f"{key}: {value}" for row in sponza_data for key, value in row.items() if key != "error_map"]))
    figures += plots(sponza_data, "Sponza")
    figures += plots(cornel_data, "Cornell Box")
    # for f in figures:
    #     f.savefig((evaluate.OUTPUT_DIR + f.get_axes()[0].get_title()).replace(" ", "_") + ".png", bbox_inches='tight')
    plt.show()


if __name__ == "__main__":
    main()
