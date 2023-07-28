import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def create_plot(directory: Path):
    info_file = directory/"info"
    train_results = directory/"train_results"
    test_results = directory/"test_results"
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

    with train_results.open() as train:
        results = []
        lines = train.readlines()
        for line in lines:
            result = line.strip().split(' ')
            results.append([float(n) for n in result])

        results.sort()
        results = pd.DataFrame(results)
        groups = results.groupby(0)

        for i, group in enumerate(groups):
            result = group[1].to_numpy()

            x = np.linspace(result[0, 0] - 1, result[0, 0], result.shape[0])
            axes[0].plot(x, result[:, 2], label='correct' if i == 0 else None, color='b')
            axes[0].plot(x, result[:, 3], label='abstain' if i == 0 else None, color='g')
            axes[0].plot(x, 1 - result[:, 2] - result[:, 3], label='incorrect' if i == 0 else None, color='r')

        axes[0].set_xticks(range(1, groups.ngroups + 1))

    axes[0].set_ylabel("Training")
    axes[0].grid(axis='y', alpha=0.5)
    axes[0].grid(axis='x', color='k', alpha=1)
    axes[0].autoscale(True, axis= 'x', tight=True)
    axes[0].legend()

    with test_results.open() as test:
        results = []
        lines = test.readlines()
        for line in lines:
            result = line.strip().split(' ')
            results.append([float(n) for n in result])

        results.sort()
        results = pd.DataFrame(results)
        groups = results.groupby(0)

        for i, group in enumerate(groups):
            result = group[1].to_numpy()

            x = np.linspace(result[0, 0] - 1, result[0, 0], result.shape[0])
            axes[1].plot(x, result[:, 2], label='correct' if i == 0 else None, color='b')
            axes[1].plot(x, result[:, 3], label='abstain' if i == 0 else None, color='g')
            axes[1].plot(x, 1 - result[:, 2] - result[:, 3], label='incorrect' if i == 0 else None, color='r')

        axes[1].set_xticks(range(1, groups.ngroups + 1))

    axes[1].set_ylabel("Testing")
    axes[1].grid(axis='y', alpha=0.5)
    axes[1].grid(axis='x', color='k', alpha=1)
    axes[1].autoscale(True, axis='x', tight=True)
    axes[1].legend()

    plots_directory = directory/"plots"
    plots_directory.mkdir(mode=0o740, exist_ok=True)
    fig.savefig(plots_directory/"performance_plot.pdf", bbox_inches='tight', dpi=150)

def main():
    if len(sys.argv) == 1:
        print("Please include the path to the test.")
        return 1
    else:
        path = Path(sys.argv[1])
        if (path.exists() and path.is_dir()):
            create_plot(path)
        else:
            print(f"'{path}' Path does not exist.")

if __name__ == '__main__':
    main()
