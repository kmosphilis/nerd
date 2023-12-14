import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def main(plotting_files: list[Path]):
    total_files = len(plotting_files)
    total_points = 0

    with plotting_files[0].open("r") as f:
        total_points = len(f.readlines())

    points = np.zeros((total_files, total_points), dtype=int)
    train_results = np.zeros((total_files, total_points, 3), dtype=float)
    test_results = np.zeros((total_files, total_points, 3), dtype=float)

    for file_index, file in enumerate(plotting_files):
        with file.open("r") as f:
            for i, line in enumerate(f.readlines()):
                line = [x.strip(" ()") for x in line.strip().split(",")]

                points[file_index][i] = int(line[0])
                train_results[file_index][i][0] = line[2]
                train_results[file_index][i][1] = line[3]
                train_results[file_index][i][2] = line[4]
                test_results[file_index][i][0] = line[5]
                test_results[file_index][i][1] = line[6]
                test_results[file_index][i][2] = line[7]

    fig, axes = plt.subplots(
        1,
        total_files,
        sharex=True,
        sharey=True,
        figsize=(5 * total_files, 5),
        layout="constrained",
    )
    print(points[0])
    fig.supylabel("Performance")

    final_testing_accs = np.zeros((total_files, 5), dtype=object)

    final_points = np.zeros(total_files)

    for i, file in enumerate(plotting_files):
        sorted_points = np.argsort(points[i])
        final_points[i] = points[i][sorted_points[-1]]
        final_testing_accs[i, 0] = file.parent.name.replace("_", " ").capitalize()
        final_testing_accs[i, 1:-1] = test_results[i][sorted_points[-1]]
        final_testing_accs[i][-1] = final_testing_accs[i][1] / (
            final_testing_accs[i][1] + final_testing_accs[i][-2]
        )

        training_acc = train_results[i][:, 0] / (
            train_results[i][:, 0] + train_results[i][:, 2]
        )

        axes[i].set_title(f"{file.parent.name}")
        axes[i].plot(
            points[i][sorted_points],
            train_results[i][sorted_points][:, 0],
            label="Correct",
            color="b",
        )
        axes[i].plot(
            points[i][sorted_points],
            train_results[i][sorted_points][:, 1],
            label="Abstained",
            color="g",
        )
        axes[i].plot(
            points[i][sorted_points],
            train_results[i][sorted_points][:, 2],
            label="Incorrect",
            color="r",
        )
        axes[i].plot(
            points[i][sorted_points],
            training_acc[sorted_points],
            label="Accuracy",
            color="m",
        )
        cor_at_end: float = training_acc[sorted_points[-1]]
        axes[i].annotate(
            text=f"{cor_at_end:.2f}",
            xy=(points[i][sorted_points[-1]], cor_at_end),
            xycoords="data",
            xytext=(2.0, 0.0),
            textcoords="offset points",
            color="m",
            horizontalalignment="left",
            verticalalignment="center",
            fontsize=10.0,
        )
        axes[i].set_xlim(right=points[i][sorted_points[-1]])

        axes[i].legend()

    reference_point = np.min(final_points)

    for i, current_point in enumerate(final_points.tolist()):
        if current_point == reference_point:
            ticks = np.linspace(0, reference_point, num=5, dtype=int, endpoint=True)
            axes[i].set_xticks(ticks)
            axes[i].set_xlabel("Instances")
        else:
            ticks = list(range(0, current_point + 1, reference_point))
            axes[i].set_xticks(ticks, list(range(0, len(ticks) + 1)))
            axes[i].grid(visible=True, axis="x", linewidth=1)
            axes[i].set_xlabel("Iterations")

    fig.savefig("results.pdf")
    pd.DataFrame(
        final_testing_accs[:, 1:],
        index=final_testing_accs[:, 0],
        columns=["Correct", "Abstained", "Incorrect", "Accuracy"],
    ).to_latex("results.tex")

    plt.show()


if __name__ == "__main__":
    if not len(sys.argv) > 1:
        print("Provide data point files to plot.")
        exit(1)

    plotting_files: list[Path] = []
    for arg in sys.argv[1:]:
        current_arg = Path(arg)
        if current_arg.exists() and current_arg.is_file():
            plotting_files.append(current_arg)
        else:
            print(f"{current_arg.name} does not exists or is not a file.")
    else:
        main(plotting_files)
        exit(0)
