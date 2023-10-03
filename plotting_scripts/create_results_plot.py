import sys
import re
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


def calculate_correct_abstained_incorrect(
    observation: list, filepath: Path, labels: list
):
    OBSERVATION_LEN = len(observation)
    if OBSERVATION_LEN == 0:
        return (0, 0, 0)

    correct = 0
    abstained = 0
    incorrect = 0
    labels = set(labels)

    with filepath.open() as file:
        content = file.readlines()
        if OBSERVATION_LEN != len(content):
            exit(3)
        for i, current_line in enumerate(content):
            label_to_find = set(observation[i]).intersection(labels)
            label_found = set(current_line.strip().split(" ")).intersection(labels)

            if label_to_find == label_found:
                correct += 1
            elif not label_found:
                abstained += 1
            else:
                incorrect += 1

    return (
        correct / OBSERVATION_LEN,
        abstained / OBSERVATION_LEN,
        incorrect / OBSERVATION_LEN,
    )


def create_plot(path: Path, labels: Path, max_iterations: int | None):
    directory = []
    if "timestamp" in path.name:
        directory.append(path)
    else:
        directory = list(path.glob("timestamp*"))

    total_kbs = []
    training_ratios = []
    testing_ratios = []
    active_rules = []
    iterations_and_instances = []

    for i, current_dir in enumerate(directory):
        results = current_dir / "results"
        if not results.exists() or not results.is_dir():
            exit(2)

        training_file = results / "training-dataset.txt"
        training_dataset = []
        testing_file = results / "testing-dataset.txt"
        testing_dataset = []

        with training_file.open() as training:
            for current_line in training:
                training_dataset.append(current_line.strip().split(" "))

        with testing_file.open() as testing:
            for current_line in testing:
                testing_dataset.append(current_line.strip().split(" "))

        _labels = []
        with labels.open() as y:
            _labels = y.readline().strip().split(" ")

        kb_result_directories = sorted(results.glob("iteration_*-instance_*"))

        total_kbs.append(len(kb_result_directories))

        training_ratios.append(np.full((total_kbs[i], 4), np.nan))
        testing_ratios.append(np.full((total_kbs[i], 4), np.nan))
        iterations_and_instances.append([])
        for index, kb in enumerate(kb_result_directories):
            training_inferences = kb / "train.txt"
            testing_inferences = kb / "test.txt"
            kb_indices = re.findall(r"\d+", kb.name)
            iterations_and_instances[i].append([int(kb_indices[0]), int(kb_indices[1])])

            correct, abstained, incorrect = calculate_correct_abstained_incorrect(
                training_dataset, training_inferences, _labels
            )

            training_ratios[i][index][0] = correct
            training_ratios[i][index][1] = abstained
            training_ratios[i][index][2] = incorrect
            training_ratios[i][index][3] = np.divide(
                correct, correct + incorrect, where=(correct + incorrect) != 0
            )

            correct, abstained, incorrect = calculate_correct_abstained_incorrect(
                testing_dataset, testing_inferences, _labels
            )

            testing_ratios[i][index][0] = correct
            testing_ratios[i][index][1] = abstained
            testing_ratios[i][index][2] = incorrect
            testing_ratios[i][index][3] = np.divide(
                correct, correct + incorrect, where=(correct + incorrect) != 0
            )

        active_rules.append(np.zeros(total_kbs[i], int))

        for index, kb in enumerate(kb_result_directories):
            file = current_dir / kb.name
            with file.open() as f:
                while "knowledge_base" not in f.readline():
                    True

                threshold = float(re.findall(r"\d+.\d+", f.readline())[0])
                if "rules" in f.readline():
                    for line in f.readlines():
                        if float(re.findall(r"\d+.\d+", line)[-1]) >= threshold:
                            active_rules[i][index] += 1
                        else:
                            break

    iterations_and_instances = np.array(iterations_and_instances)

    fig, axes = plt.subplots(
        nrows=3, ncols=1, figsize=(16, 9), layout="constrained", sharex="all"
    )

    info_file: Path = directory[0] / "info"
    filename = ""
    with info_file.open() as info:
        title = ""
        run = ""
        current_value = None
        for line in info:
            if line.startswith("run="):
                run = line
            elif line.startswith("t="):
                current_value = float(line.strip().removeprefix("t="))
                filename += f"t{current_value}, "
                title += f"Threshold: {current_value}, "
            elif line.startswith("p="):
                current_value = float(line.strip().removeprefix("p="))
                filename += f"p{current_value}, "
                title += f"Promotion: {current_value}, "
            elif line.startswith("d="):
                current_value = float(line.strip().removeprefix("d="))
                filename += f"d{current_value}, "
                title += f"Demotion: {current_value}, "
            elif line.startswith("b="):
                current_value = int(line.strip().removeprefix("b="))
                filename += f"b{current_value}, "
                title += f"Max breadth per rule: {current_value}, "
            elif line.startswith("r="):
                current_value = int(line.strip().removeprefix("r="))
                filename += f"r{current_value}, "
                title += f"Max rules per instance: {current_value}, "
            elif line.startswith("o="):
                current_value = line.strip().removeprefix("o=").lower() == "true"
                filename += f"o{current_value}, "
                if current_value:
                    title += "Partial observation, "
                else:
                    title += "Full observation, "
            elif line.startswith("c="):
                current_value = line.strip().removeprefix("c=").lower() == "true"
                filename += f"c{current_value}, "
                if current_value:
                    title += "Classic, "
                else:
                    title += "Backward chaining, "
            elif line.startswith("di="):
                current_value = line.strip().removeprefix("di=").lower() == "true"
                filename += f"di{current_value}, "
                if current_value:
                    title += "Increasing demotion, "
                else:
                    title += "Decreasing demotion, "
            elif line.startswith("h="):
                current_value = line.strip().removeprefix("h=").lower() == "true"
                filename += f"h{current_value}, "
                if current_value:
                    title += "With labels, "
                else:
                    title += "Without labels, "

        if max_iterations is not None:
            filename += f"e{max_iterations}, "

        if len(directory) == 1:
            filename = filename + run
            fig.suptitle(f"{title}Trial: {run.strip().removeprefix('run=')}")
        else:
            filename = filename.removesuffix(", ")
            fig.suptitle(title.removesuffix(", "))
    fig.supxlabel("Iteration")

    biggest_index = np.argmax(total_kbs)
    x = np.arange(1, total_kbs[biggest_index] + 1)

    for i, (ratio, name) in enumerate(
        zip([training_ratios, testing_ratios], ["Training", "Testing"])
    ):
        if len(ratio) == 1:
            axes[i].plot(x, ratio[0][:, 0], label="Correct", color="b")
            axes[i].plot(x, ratio[0][:, 1], label="Abstain", color="g")
            axes[i].plot(x, ratio[0][:, 2], label="Incorrect", color="r")
            axes[i].plot(x, ratio[0][:, 3], label="Accuracy", color="m")
            acc_at_end: float = ratio[0][:, 3][-1]
            axes[i].annotate(
                text=f"{acc_at_end:.2f}",
                xy=(x[-1], acc_at_end),
                xycoords="data",
                xytext=(2.0, 0.0),
                textcoords="offset points",
                color="m",
                horizontalalignment="left",
                verticalalignment="center",
                fontsize=10.0,
            )
        else:
            ratio = np.array(ratio)
            median = np.median(ratio, axis=0)
            mean = np.mean(ratio, axis=0)
            q1 = np.percentile(ratio, 25, axis=0)
            q3 = np.percentile(ratio, 75, axis=0)
            axes[i].plot(x, median[:, 0], label="Correct median", color="b")
            axes[i].plot(x, mean[:, 0], label="Correct mean", linestyle=":", color="b")
            axes[i].fill_between(
                x, q1[:, 0], q3[:, 0], label="Correct Q1 - Q3", color="b", alpha=0.1
            )
            axes[i].plot(x, median[:, 1], label="Abstain median", color="g")
            axes[i].plot(x, mean[:, 1], label="Abstain mean", linestyle=":", color="g")
            axes[i].fill_between(
                x, q1[:, 1], q3[:, 1], label="Abstain Q1 - Q3", color="g", alpha=0.1
            )
            axes[i].plot(x, median[:, 2], label="Incorrect median", color="r")
            axes[i].plot(
                x, mean[:, 2], label="Incorrect mean", linestyle=":", color="r"
            )
            axes[i].fill_between(
                x, q1[:, 2], q3[:, 2], label="Incorrect Q1 - Q3", color="r", alpha=0.1
            )
            axes[i].plot(x, median[:, 3], label="Accuracy median", color="m")
            axes[i].plot(x, mean[:, 3], label="Accuracy mean", linestyle=":", color="m")
            axes[i].fill_between(
                x, q1[:, 3], q3[:, 3], label="Accuracy Q1 - Q3", color="m", alpha=0.1
            )
            median_at_end: float = median[:, 3][-1]
            axes[i].annotate(
                text=f"{median_at_end:.2f}",
                xy=(x[-1], median_at_end),
                xycoords="data",
                xytext=(2.0, 0.0),
                textcoords="offset points",
                color="m",
                horizontalalignment="left",
                verticalalignment="center",
                fontsize=10.0,
            )

        axes[i].set_ylabel(f"{name} Performance")
        axes[i].set_ylim(0, 1)
        axes[i].grid(axis="y", alpha=0.5)
        axes[i].grid(axis="x", color="k", alpha=1)
        axes[i].legend(ncols=4)

    if len(active_rules) == 1:
        axes[2].plot(x, active_rules[0], color="k")
        axes[2].annotate(
            text=f"{active_rules[0][-1]:.0f}",
            xy=(x[-1], active_rules[0][-1]),
            xycoords="data",
            xytext=(2.0, 0.0),
            textcoords="offset points",
            color="k",
            horizontalalignment="left",
            verticalalignment="center",
            fontsize=10.0,
        )

    else:
        median = np.median(active_rules, axis=0)
        mean = np.mean(active_rules, axis=0)
        q1 = np.percentile(active_rules, 25, axis=0)
        q3 = np.percentile(active_rules, 75, axis=0)
        axes[2].plot(x, median, label="Median", color="k")
        axes[2].plot(x, mean, label="Mean", linestyle=":", color="k")
        axes[2].fill_between(x, q1, q3, label="Q1 - Q3", color="k", alpha=0.1)
        axes[2].legend(ncols=3)
        median_at_end: float = median[-1]
        axes[2].annotate(
            text=f"{median_at_end:.0f}",
            xy=(x[-1], median_at_end),
            xycoords="data",
            xytext=(2.0, 0.0),
            textcoords="offset points",
            color="k",
            horizontalalignment="left",
            verticalalignment="center",
            fontsize=10.0,
        )

    axes[2].set_ylabel("Active Rules")
    axes[2].grid(axis="y", alpha=0.5)
    axes[2].grid(axis="x", color="k", alpha=1)

    iterations = np.unique(iterations_and_instances[0][:, 0])
    instances = np.unique(iterations_and_instances[0][:, 1])

    axes[2].set_xlim(instances.shape[0], np.max(iterations) + instances.shape[0])
    axes[2].set_xticks(
        np.arange(
            0, (iterations.shape[0] + 1) * instances.shape[0], instances.shape[0]
        ),
        np.append([0], iterations),
    )

    for axis_index, axis in enumerate(axes):
        axis_twin = axis.twiny()
        axis.set_zorder(axis_twin.get_zorder() + axes.shape[0])
        axis.patch.set_visible(False)
        axis_twin.grid(axis="x", color="k", alpha=0.5)
        axis_twin.set_xlim(instances.shape[0], np.max(iterations) + instances.shape[0])
        axis_twin.set_xticks(
            np.arange(0, (iterations.shape[0]) * instances.shape[0] + 1, 1),
            np.append([0], iterations_and_instances[0][:, 1])
            if axis_index == 0
            else [],
        )
        if axis_index == 0:
            axis_twin.set_xlabel("Instance", fontsize=12)

    plots_directory = None
    if len(active_rules) == 1:
        plots_directory = path / "plots"
    else:
        plots_directory = path
    plots_directory.mkdir(mode=0o740, exist_ok=True)
    fig.savefig(plots_directory / f"{filename}.pdf", bbox_inches="tight", dpi=150)


def main():
    if len(sys.argv) < 3:
        print(
            "Please include the path to the test or a directory with many tests, the path to the "
            "labels file, and optional the max number of iterations to evaluate."
        )
        exit(1)
    else:
        path = Path(sys.argv[1])
        if path.exists():
            if path.is_dir():
                labels = Path(sys.argv[2])
                if labels.exists():
                    if labels.is_file():
                        max_iterations = None
                        if len(sys.argv) == 4:
                            if sys.argv[3].isdigit():
                                max_iterations = int(sys.argv[3])
                            else:
                                print(f"'{sys.argv[3]}' is not a valid number.")
                        create_plot(path, labels, max_iterations)
                        exit(0)
                    else:
                        print(f"'{labels.name}' is not a file.")
                else:
                    print(f"'{labels.name}' does not exist.")
            else:
                print(f"'{path.name}' is not a directory.")
        else:
            print(f"'{path.name} 'does not exist.")
        exit(1)


if __name__ == "__main__":
    main()
