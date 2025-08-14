import flip_evaluator as flip
import matplotlib.pyplot as plt
import csv

IMAGES_DIR = "./build/bin/Release/"

fields_of_interest = (
       "image_name",
       "rc_minmax_probes",
       "rc_resolution_factor",
       "frame_time",
       "avg_frame_time",
       )

def main():
    with open(IMAGES_DIR + "dump/captures.csv", "r", newline='') as csvfile:
        data = csv.DictReader(csvfile)
        data_list = list(data)
    sponza_path = IMAGES_DIR + next(name for row in data_list if "Sponza" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornel_path = IMAGES_DIR + next(name for row in data_list if "cornel" in (name := row["image_name"]) and "ReferencePathTracer" in name)
    cornellref = flip.load(cornel_path)
    sponzaref = flip.load(sponza_path)
    for row in data_list:
        if "ReferencePathTracer" in row["image_name"]:
            continue
        ref = sponzaref if "Sponza" in row["image_name"] else cornellref
        test = flip.load(IMAGES_DIR + row["image_name"])
        errormap, meanerror, params = flip.evaluate(ref, test, "HDR")
        print(" | ".join([f"{field}: {row[field]}" for field in fields_of_interest]))
        print(f"Mean FLIP error: {meanerror}")
        fig, axs = plt.subplots()
        axs.imshow(errormap)
        axs.set_label(row["image_name"])
    plt.show()

if __name__ == "__main__":
    main()
