import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

def create_plot(directory: str):
    train_results = f"{directory}/train_results"
    test_results = f"{directory}/test_results"
    info_file = f"{directory}/info"
    fig, axes = plt.subplots(nrows=2, ncols=1, figsize=(16,9), layout='constrained')
    with open(info_file) as info:
        title = ""
        ignore = ['h=', 'seed=']
        found_taboo = False
        for line in info.readlines():
            for taboo in ignore:
                if line.find(taboo) != -1:
                    found_taboo = True
            if not found_taboo:
                title += line.strip() + ", "
            found_taboo = False
        fig.suptitle(title)
    fig.supylabel("Performance")
    fig.supxlabel("Epoch")

    with open(train_results) as train:
        results = []
        lines = train.readlines()
        for line in lines:
            result = line.strip().split(" ")
            results.append([float(n) for n in result])
        results.sort()
        results =np.array(results)

        axes[0].plot(results[:, 0], results[:, 1], label="correct", color="g", marker="+")
        axes[0].plot(results[:, 0], results[:, 2], label="abstain", color="b", marker='x')
        axes[0].plot(results[:, 0], 1 - results[:, 1], label="incorrect", color="r", marker="+")
        axes[0].legend()
        axes[0].set_title("Training")
    with open(test_results) as test:
        results = []
        lines = test.readlines()
        for line in lines:
            result = line.strip("\n").split(" ")
            results.append([float(n) for n in result])
        results.sort()
        results =np.array(results)
        axes[1].plot(results[:, 0], results[:, 1], label="correct", color="g", marker="+")
        axes[1].plot(results[:, 0], results[:, 2], label="abstain", color="b", marker="x")
        axes[1].plot(results[:, 0], 1 - results[:, 1], label="incorrect", color="r", marker="+")
        axes[1].legend()
        axes[1].set_title("Testing")

    fig.savefig(f"{directory}plot.png", bbox_inches='tight', dpi=150)

def main():
    if len(sys.argv) == 1:
        print("Please include the path to the test.")
        return 1
    else:
        path = Path(sys.argv[1])
        if (path.exists() and path.is_dir()):
            create_plot(sys.argv[1])
        else:
            print(f"'{path}' Path does not exist.")

if __name__ == '__main__':
    main()
