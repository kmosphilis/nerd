import sys
import re
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

def calculate_correct_abstained(observation: list, filepath: Path, labels: list):
    correct = 0; abstained = 0

    with filepath.open() as file:
        content = file.readlines()
        if (len(observation) != len(content)):
            exit(3)
        for i, current_line in enumerate(content):
            label_to_find = set(observation[i]).intersection(labels)
            label_found = set(current_line.strip().split(" ")).intersection(labels)

            if (label_to_find == label_found):
                correct +=1
            elif (not label_found):
                abstained += 1

    return correct / len(observation), abstained / len(observation)

def create_plot(directory: Path, labels: Path):
    info_file = directory/"info"
    results = directory/"results"
    if (not results.exists() or not results.is_dir()):
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

    kb_result_directories = sorted(results.glob("iteration_*-instance_*"))

    total = re.findall('\d+', kb_result_directories[-1].name)
    iterations = int(total[0])
    instances = int(total[1])
    if (len(total) > iterations * instances):
        instances = int(re.findall('\d+', kb_result_directories[-(instances + 1)].name)[1])

    training_ratios = np.full((iterations * instances, 2), np.nan)
    testing_ratios = np.full((iterations * instances, 2), np.nan)

    for kb in kb_result_directories:
        training_inferences = kb/"train.txt"
        testing_inferences = kb/"test.txt"

        indices = re.findall('\d+', kb.name)
        iteration_index = int(indices[0]) - 1
        instance_index = int(indices[1]) - 1
        current_index = (iteration_index * instances) + instance_index

        correct, abstained = calculate_correct_abstained(training_dataset, training_inferences,
                                                         _labels)

        training_ratios[current_index][0] = correct
        training_ratios[current_index][1] = abstained

        correct, abstained = calculate_correct_abstained(testing_dataset, testing_inferences,
                                                         _labels)

        testing_ratios[current_index][0] = correct
        testing_ratios[current_index][1] = abstained

    fig, axes = plt.subplots(nrows=2, ncols=1, figsize=(16, 9), layout='constrained')

    with info_file.open() as info:
        title = ""
        ignore = ['h=']
        found_taboo = False
        for line in info.readlines():
            for taboo in ignore:
                if line.find(taboo) != -1:
                    found_taboo = True
            if not found_taboo:
                title += line.strip() + ', '
            found_taboo = False
        fig.suptitle(title.removesuffix(', '))
    fig.supylabel("Performance")
    fig.supxlabel("Iterations")

    x = np.linspace(1, instances * iterations, training_ratios.shape[0], True)

    axes[0].plot(x, training_ratios[:, 0], label='correct', color='b')
    axes[0].plot(x, training_ratios[:, 1], label='abstain', color='g')
    axes[0].plot(x, 1 - training_ratios[:, 0] - training_ratios[:, 1], label='incorrect', color='r')
    axes[0].set_ylabel("Training")
    axes[0].grid(axis='y', alpha=0.5)
    axes[0].grid(axis='x', color='k', alpha=1)
    axes[0].autoscale(True, axis= 'x', tight=True)
    axes[0].set_xticks(range(1, iterations * instances, instances), range(1, iterations + 1))
    axes[0].legend()

    axes[1].plot(x, testing_ratios[:, 0], label='correct', color='b')
    axes[1].plot(x, testing_ratios[:, 1], label='abstain', color='g')
    axes[1].plot(x, 1 - testing_ratios[:, 0] - testing_ratios[:, 1], label='incorrect', color='r')
    axes[1].set_ylabel("Testing")
    axes[1].grid(axis='y', alpha=0.5)
    axes[1].grid(axis='x', color='k', alpha=1)
    axes[1].autoscale(True, axis='x', tight=True)
    axes[1].set_xticks(range(1, iterations * instances, instances), range(1, iterations + 1))
    axes[1].legend()

    plots_directory = directory/"plots"
    plots_directory.mkdir(mode=0o740, exist_ok=True)
    fig.savefig(plots_directory/"performance_plot.pdf", bbox_inches='tight', dpi=150)

def main():
    if len(sys.argv) != 3:
        print("Please include the path to the test, and the path to the labels file.")
        return 1
    else:
        path = Path(sys.argv[1])
        if (path.exists() and path.is_dir()):
            labels = None
            if (len(sys.argv) == 3):
                _labels = Path(sys.argv[2])
                if (_labels.exists() and _labels.is_file()):
                    labels = _labels
            create_plot(path, labels)
        else:
            print(f"'{path}' Path does not exist.")

if __name__ == '__main__':
    main()
