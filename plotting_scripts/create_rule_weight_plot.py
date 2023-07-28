import sys
import re
import math
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

def plot_rule_journey(directory: Path, nerd_file: Path, rules_to_examine: int):
    info_file = directory/"info"
    knowledge_base_file = nerd_file
    threshold = None

    with knowledge_base_file.open() as k:
        found_rules = False
        while (not found_rules):
            current_line = k.readline().strip()
            if current_line == 'rules:':
                found_rules = True
            if  'threshold:' in current_line:
                threshold = float(re.findall('\d+\.\d+', current_line)[0])

        if (not found_rules):
            exit(-1)

        rules = []
        for i in range(rules_to_examine):
            rules.append(k.readline().rsplit('(', 1)[0].strip())

    nerd_files = sorted(directory.glob('iteration_*-instance_*.nd'))
    total = re.findall('\d+', nerd_files[-1].name)
    iterations = int(total[0])
    instances = int(total[1])
    if len(nerd_files) != (iterations * instances):
        instances = int(re.findall('\d+', nerd_files[-(instances + 1)].name)[1])

    rule_weights_over_time = np.full((rules_to_examine, len(nerd_files)), np.nan)
    current_file = 0

    for file in nerd_files:
        with file.open() as current_kb:
            for line in current_kb:
                found = 0
                for rule_i, rule in enumerate(rules):
                    if rule in line:
                        found += 1
                        rule_weights_over_time[rule_i, current_file] = float(
                            re.findall('\d+\.\d+', line.split("(")[-1].strip())[0])

                        if found == len(rules):
                            break
        current_file += 1

    title = ""
    with info_file.open() as info:
        ignore = ['h=']
        found_taboo = False
        for line in info.readlines():
            for taboo in ignore:
                if line.find(taboo) != -1:
                    found_taboo = True
            if not found_taboo:
                title += line.strip() + ', '
            found_taboo = False
        title.removesuffix(', ')

    plots_directory = directory/"plots"
    plots_directory.mkdir(mode=0o740, exist_ok=True)
    learning_period = np.linspace(0, rule_weights_over_time.shape[1],
                                  rule_weights_over_time.shape[1])
    for fig_number in range(0, math.ceil(len(rules) / 4)):
        fig, axes = plt.subplots(2, 2, figsize=(16, 9), layout='constrained')
        fig.suptitle(title)
        fig.supxlabel("Iterations")
        fig.supylabel("Weight")

        for i in range(0, 4):
            x = i // 2; y = i % 2
            axes[x, y].plot(learning_period, rule_weights_over_time[i + fig_number * 4, :],
                            color='b', label='weight')
            axes[x, y].plot(learning_period, np.full(learning_period.shape, threshold), color='g',
                            linewidth=1, label=f'threshold={threshold}')
            axes[x, y].set_title(rule)
            axes[x, y].set_xlim((0, len(nerd_files)))
            axes[x, y].set_xticks(range(0, (iterations + 1) * instances, instances),
                                range(0, iterations + 1))
            axes[x, y].grid()
            axes[x, y].autoscale(True, tight=True)
            axes[x, y].legend()

        fig.savefig(plots_directory/f"rules_{(fig_number * 4) + 1}-{(fig_number * 4) + 4}_"
                    "weight-history.pdf", bbox_inches='tight')

def main():
    if len(sys.argv) == 3:
        print("Please include the path to the test directory, the .nd file to find the weight "
              "history of the rules, and the number of rules to find.")
        exit(1)
    else:
        path = Path(sys.argv[1])
        file = Path(sys.argv[2])
        rules_to_find = int(sys.argv[3])
        if (path.exists() and path.is_dir()):
            if (sys.argv[2].endswith(".nd")):
                if (file.exists() and file.is_file()):
                    if (rules_to_find > 0):
                        plot_rule_journey(path, file, rules_to_find)
                    else:
                        print(f"rules to find must be bigger than 0.")
                        exit(5)
                else:
                    print(f"'{file}' does not exists.")
                    exit(4)
            else:
                print(f".nd file expected and got '{file}' instead.")
                exit(3)
        else:
            print(f"'{path}' directory does not exist.")
            exit(2)

if __name__ == '__main__':
    main()
