import sys
import re
import math
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

def plot_rule_journey(directory: Path, nerd_file: Path, rules_to_examine: int | range):
    info_file = directory/"info"
    knowledge_base_file = nerd_file
    threshold = None

    with knowledge_base_file.open() as k:
        found_rules = False
        while not found_rules:
            current_line = k.readline().strip()
            if current_line == 'rules:':
                found_rules = True
            if  'threshold:' in current_line:
                threshold = float(re.findall('\d+\.\d+', current_line)[0])

        if not found_rules:
            exit(-1)

        rules = []
        final_rule = rules_to_examine
        if isinstance(rules_to_examine, range):
            final_rule = rules_to_examine.stop - 1
            for i in rules_to_examine:
                rules.append(k.readline().rsplit('(', 1)[0].strip())
        else:
            for i in range(rules_to_examine):
                k.readline()
            rules.append(k.readline().rsplit('(', 1)[0].strip())

    nerd_files = sorted(directory.glob('iteration_*-instance_*.nd'))
    total = re.findall('\d+', nerd_files[-1].name)
    iterations = int(total[0])
    instances = int(total[1])
    if len(nerd_files) != (iterations * instances):
        instances = int(re.findall('\d+', nerd_files[-(instances + 1)].name)[1])

    rule_weights_over_time = np.full((final_rule, len(nerd_files)), np.nan)
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

    if isinstance(rules_to_examine, int):
        fig, axis = plt.subplots(1, 1, figsize=(16, 9), layout='constrained')
        axis.plot(learning_period, rule_weights_over_time[0], color='b', label='weight')
        axis.plot(learning_period, np.full(learning_period.shape, threshold), color='g',
                        linewidth=1, label=f'threshold={threshold}')
        fig.suptitle(rules[0])
        axis.set_xlim((0, len(nerd_files)))
        axis.set_xticks(range(0, (iterations + 1) * instances, instances),
                            range(0, iterations + 1))
        axis.grid()
        axis.autoscale(True, tight=True)
        axis.legend()

        fig.savefig(plots_directory/f"rule{rule}_"
                    "weight-history.pdf", bbox_inches='tight')

    else:
        for fig_number in range(0, math.ceil(len(rules) / 4)):
            fig, axes = plt.subplots(2, 2, figsize=(16, 9), layout='constrained')
            fig.suptitle(title)
            fig.supxlabel("Iterations")
            fig.supylabel("Weight")

            for i in range(0, min(4, len(rules) - (fig_number * 4))):
                x, y = divmod(i, 2)
                axes[x, y].plot(learning_period, rule_weights_over_time[i + fig_number * 4, :],
                                color='b', label='weight')
                axes[x, y].plot(learning_period, np.full(learning_period.shape, threshold), color='g',
                                linewidth=1, label=f'threshold={threshold}')
                axes[x, y].set_title(rules[i + fig_number * 4])
                axes[x, y].set_xlim((0, len(nerd_files)))
                axes[x, y].set_xticks(range(0, (iterations + 1) * instances, instances),
                                    range(0, iterations + 1))
                axes[x, y].grid()
                axes[x, y].autoscale(True, tight=True)
                axes[x, y].legend()

            fig.savefig(plots_directory/f"rules_{(fig_number * 4) + 1}-{min((fig_number * 4) + 4, len(rules))}_"
                        "weight-history.pdf", bbox_inches='tight')

def main():
    if len(sys.argv) != 4:
        print("Please include the path to the test directory, the .nd file to find the weight "
              "history of the rules, and the rule or the rule range to find (n-m).")
        exit(1)
    else:
        path = Path(sys.argv[1])
        file = Path(sys.argv[2])
        if path.exists() and path.is_dir():
            if sys.argv[2].endswith(".nd"):
                if file.exists() and file.is_file():
                    arg = sys.argv[3]
                    if '-' in arg:
                        _range = arg.split('-')
                        for i, n in enumerate(_range):
                            if not n.isdigit() or int(n) < 1:
                                print(f"{n} is not a valid number.")
                                exit(1)
                            else:
                                _range[i] = int(n)
                        if _range[0] < _range[1]:
                            plot_rule_journey(path, file, range(_range[0], _range[1] + 1))
                            exit(0)
                        print("Insert a valid range.")
                    else:
                        if arg.isdigit():
                            arg = int(arg)
                            if arg > 0:
                                plot_rule_journey(path, file, arg)
                                exit(0)
                        print(f"{arg} is not a valid number.")
                else:
                    print(f"'{file}' does not exists.")
            else:
                print(f".nd file expected and got '{file}' instead.")
        else:
            print(f"'{path}' directory does not exist.")
        exit(1)

if __name__ == '__main__':
    main()
