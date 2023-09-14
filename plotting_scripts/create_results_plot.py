import sys
import re
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

def calculate_correct_abstained_incorrect(observation: list, filepath: Path, labels: list):
    correct = 0; abstained = 0; incorrect = 0
    labels = set(labels)

    with filepath.open() as file:
        content = file.readlines()
        if len(observation) != len(content):
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

    return correct / len(observation), abstained / len(observation), incorrect / len(observation)

def create_plot(path: Path, labels: Path, max_iterations: int | None):
    directory = []
    if 'timestamp' in path.name:
        directory.append(path)
    else:
        directory = list(path.glob("timestamp*"))

    total_kbs = []
    training_ratios = []
    testing_ratios = []
    active_rules = []
    kb_result_directories = []
    iterations = []

    for i, current_dir in enumerate(directory):
        results = current_dir/"results"
        if not results.exists() or not results.is_dir():
            exit(2)

        training_file = results/"training-dataset.txt"
        training_dataset = []
        testing_file = results/"testing-dataset.txt"
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

        kb_result_directories.append(sorted(results.glob("iteration_*-instance_*")))

        total_kbs.append(len(kb_result_directories[i]))
        total = re.findall('\d+', kb_result_directories[i][-1].name)
        iterations.append(int(total[0]))

        if max_iterations is not None and iterations[i] >= max_iterations:
            iterations[i] = max_iterations
            kb_result_directories[i] = [item for item in kb_result_directories[i]
                                        if max_iterations >= int(re.findall("\d+", item.name)[0])]
            total_kbs[i] = len(kb_result_directories[i])

        training_ratios.append(np.full((total_kbs[i], 4), np.nan))
        testing_ratios.append(np.full((total_kbs[i], 4), np.nan))

        for index, kb in enumerate(kb_result_directories[i]):
            training_inferences = kb/"train.txt"
            testing_inferences = kb/"test.txt"

            correct, abstained, incorrect = calculate_correct_abstained_incorrect(training_dataset,
                                                                                  training_inferences,
                                                                                  _labels)

            training_ratios[i][index][0] = correct
            training_ratios[i][index][1] = abstained
            training_ratios[i][index][2] = incorrect
            training_ratios[i][index][3] = np.divide(correct, correct + incorrect,
                                                     where=(correct + incorrect) != 0)

            correct, abstained, incorrect = calculate_correct_abstained_incorrect(testing_dataset,
                                                                                  testing_inferences,
                                                                                  _labels)

            testing_ratios[i][index][0] = correct
            testing_ratios[i][index][1] = abstained
            testing_ratios[i][index][2] = incorrect
            testing_ratios[i][index][3] = np.divide(correct, correct + incorrect,
                                                     where=(correct + incorrect) != 0)

        active_rules.append(np.zeros(total_kbs[i], int))

        for index, kb in enumerate(kb_result_directories[i]):
            file = current_dir/kb.name
            with file.open() as f:

                while ("knowledge_base" not in f.readline()):
                    True

                threshold = float(re.findall("\d+.\d+",f.readline())[0])
                if "rules" in f.readline():
                    for line in f.readlines():
                        if (float(re.findall("\d+.\d+", line)[-1]) >= threshold):
                            active_rules[i][index] += 1
                        else:
                            break

    fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(16, 9), layout='constrained', sharex='all')

    info_file: Path = directory[0]/"info"
    filename = ""
    with info_file.open() as info:
        title = ""
        run = ""
        for line in info:
            if line.startswith('run='):
                run = line
            elif line.startswith('t='):
                filename += f"{line.strip()}, "
                title += f"Threshold: {float(line.strip().removeprefix('t='))}, "
            elif line.startswith('p='):
                filename += f"{line.strip()}, "
                title += f"Promotion: {float(line.strip().removeprefix('p='))}, "
            elif line.startswith('d='):
                filename += f"{line.strip()}, "
                title += f"Demotion: {float(line.strip().removeprefix('d='))}, "
            elif line.startswith('b='):
                filename += f"{line.strip()}, "
                title += f"Max breadth per rule: {int(line.strip().removeprefix('b='))}, "
            elif line.startswith('r='):
                filename += f"{line.strip()}, "
                title += f"Max rules per instance: {int(line.strip().removeprefix('r='))}, "
            elif line.startswith('o='):
                filename += f"{line.strip()}, "
                value = line.strip().removeprefix('o=').lower() == 'true'
                if value:
                    title += "Partial observation, "
                else:
                    title += "Full observation, "
            elif line.startswith('c='):
                filename += f"{line.strip()}, "
                value = line.strip().removeprefix('c=').lower() == 'true'
                if value:
                    title += "Classic, "
                else:
                    title += "Backward chaining, "
            elif line.startswith('di='):
                filename += f"{line.strip()}, "
                value = line.strip().removeprefix('di=').lower() == 'true'
                if value:
                    title += "Increasing demotion, "
                else:
                    title += "Decreasing demotion, "
            elif line.startswith('h='):
                filename += f"{line.strip()}, "
                value = line.strip().removeprefix('h=').lower() == 'true'
                if value:
                    title += "With labels, "
                else:
                    title += "Without labels, "

        if max_iterations is not None:
            filename += f"e={max_iterations}, "

        if len(directory) == 1:
            filename = filename + run
            fig.suptitle(f"{title}Trial: {run.strip().removeprefix('run=')}")
        else:
            filename = filename.removesuffix(', ')
            fig.suptitle(title.removesuffix(', '))
    fig.supxlabel("Iterations")

    biggest_index = np.argmax(total_kbs)

    x = np.linspace(1, total_kbs[biggest_index], training_ratios[biggest_index].shape[0], True)
    x_tick_sizes = np.zeros(max(iterations), int)
    for kb in kb_result_directories[biggest_index]:
        x_tick_sizes[int(re.findall(r"(\d+)", kb.name)[0]) - 1] += 1
    zero_indication = x_tick_sizes != 0
    zero_indication[0] = 1
    x_ticks = x_tick_sizes[zero_indication]
    x_tick_labels = []

    for i, indication in enumerate(zero_indication):
        if (indication):
            x_tick_labels.append(i + 1)

    x_ticks = np.cumsum(x_ticks)

    for i, (ratio, name) in enumerate(zip([training_ratios, testing_ratios], ["Training", "Testing"])):
        if len(ratio) == 1:
            axes[i].plot(x, ratio[0][:, 0], label='Correct', color='b')
            axes[i].plot(x, ratio[0][:, 1], label='Abstain', color='g')
            axes[i].plot(x, ratio[0][:, 2], label='Incorrect', color='r')
            axes[i].plot(x, ratio[0][:, 3], label='Accuracy', color='m')
        else:
            ratio = np.array(ratio)
            median = np.median(ratio, axis=0)
            mean = np.mean(ratio, axis=0)
            q1 = np.percentile(ratio, 25, axis=0)
            q3 = np.percentile(ratio, 75, axis=0)
            axes[i].plot(x, median[:, 0], label='Correct median', color='b')
            axes[i].plot(x, mean[:, 0], label='Correct mean', linestyle=':', color='b')
            axes[i].fill_between(x, q1[:, 0], q3[:, 0], label='Correct Q1 - Q3', color='b', alpha=0.1)
            axes[i].plot(x, median[:, 1], label='Abstain median', color='g')
            axes[i].plot(x, mean[:, 1], label='Abstain mean', linestyle=':', color='g')
            axes[i].fill_between(x, q1[:, 1], q3[:, 1], label='Abstain Q1 - Q3', color='g', alpha=0.1)
            axes[i].plot(x, median[:, 2], label='Incorrect median', color='r')
            axes[i].plot(x, mean[:, 2], label='Incorrect mean', linestyle=':', color='r')
            axes[i].fill_between(x, q1[:, 2], q3[:, 2], label='Incorrect Q1 - Q3', color='r', alpha=0.1)
            axes[i].plot(x, median[:, 3], label='Accuracy median', color='m')
            axes[i].plot(x, mean[:, 3], label='Accuracy mean', linestyle=':', color='m')
            axes[i].fill_between(x, q1[:, 3], q3[:, 3], label='Accuracy Q1 - Q3', color='m', alpha=0.1)
        axes[i].set_ylabel(f"{name} Performance")
        axes[i].set_ylim(0, 1)
        axes[i].grid(axis='y', alpha=0.5)
        axes[i].grid(axis='x', color='k', alpha=1)
        axes[i].autoscale(True, axis= 'x', tight=True)
        axes[i].set_xticks(x_ticks, x_tick_labels)
        axes[i].legend(ncols=4)

    if (len(active_rules) == 1):
        axes[2].plot(x, active_rules[0], color='g')
    else:
        median = np.median(active_rules, axis=0)
        mean = np.mean(active_rules, axis=0)
        q1 = np.percentile(active_rules, 25, axis=0)
        q3 = np.percentile(active_rules, 75, axis=0)
        axes[2].plot(x, median, label='Median', color='k')
        axes[2].plot(x, mean, label='Mean', linestyle=':', color='k')
        axes[2].fill_between(x, q1, q3, label='Q1 - Q3', color='k', alpha=0.1)
        axes[2].legend(ncols=3)
    axes[2].set_ylabel("Active Rules")
    axes[2].grid(axis='y', alpha=0.5)
    axes[2].grid(axis='x', color='k', alpha=1)
    axes[2].autoscale(True, axis='x', tight=True)
    axes[2].set_xticks(x_ticks, x_tick_labels)

    plots_directory = None
    if len(active_rules) == 1:
        plots_directory = path/"plots"
    else:
        plots_directory = path
    plots_directory.mkdir(mode=0o740, exist_ok=True)
    fig.savefig(plots_directory/f"{filename}.pdf", bbox_inches='tight', dpi=150)

def main():
    if len(sys.argv) < 3 :
        print("Please include the path to the test or a directory with many test, the path to the "
              "labels file, and optional the max number of iterations to evaluate.")
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

if __name__ == '__main__':
    main()
