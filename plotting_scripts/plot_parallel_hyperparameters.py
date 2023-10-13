import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import plotly.graph_objects as go

from create_results_plot import calculate_training_testing_actives


def create_parallel_hyperparameters(directories: list, labels: Path):
    data = []

    for i, directory in enumerate(directories):
        (
            training_ratios,
            testing_ratios,
            active_rules,
        ) = calculate_training_testing_actives(directory, labels, None)

        final_training_score = training_ratios[:, -1][:, 3]
        final_testing_score = testing_ratios[:, -1][:, 3]
        final_active_rules = active_rules[:, -1]
        info_file = list(directory.glob("*=0/info"))[0]

        with info_file.open() as info:
            for line in info:
                line = line.strip()
                if line.startswith("t="):
                    threshold = float(line.removeprefix("t="))
                elif line.startswith("p="):
                    promotion = float(line.removeprefix("p="))
                elif line.startswith("d="):
                    demotion = float(line.removeprefix("d="))
                elif line.startswith("b="):
                    breadth = int(line.removeprefix("b="))
                elif line.startswith("r="):
                    rules = int(line.removeprefix("r="))
                elif line.startswith("o="):
                    partial_observation = line.removeprefix("o=").lower() == "true"

        data.append(
            [
                threshold,
                promotion,
                demotion,
                breadth,
                rules,
                partial_observation,
                np.mean(final_training_score),
                np.mean(final_testing_score),
                np.mean(final_active_rules),
            ]
        )

    data = pd.DataFrame(
        data,
        columns=[
            "Threshold",
            "Promotion",
            "Demotion",
            "Max Breadth",
            "Max Rules",
            "Partial Observation",
            "Training",
            "Testing",
            "Active Rules",
        ],
    )

    fig = go.Figure(
        data=go.Parcoords(
            line=dict(color=data["Testing"], colorscale="Plasma"),
            dimensions=list(
                [
                    dict(
                        range=[
                            data["Threshold"].min() - 1,
                            data["Threshold"].max() + 1,
                        ],
                        label="Threshold",
                        values=data["Threshold"],
                    ),
                    dict(
                        range=[
                            data["Promotion"].min() - 1,
                            data["Promotion"].max() + 1,
                        ],
                        label="Promotion",
                        values=data["Promotion"],
                    ),
                    dict(
                        range=[data["Demotion"].min() - 1, data["Demotion"].max() + 1],
                        label="Demotion",
                        values=data["Demotion"],
                    ),
                    dict(
                        range=[
                            data["Max Breadth"].min() - 1,
                            data["Max Breadth"].max() + 1,
                        ],
                        label="Max Breadth",
                        values=data["Max Breadth"],
                        tickvals=np.arange(
                            data["Max Breadth"].min() - 1, data["Max Breadth"].max() + 2
                        ),
                    ),
                    dict(
                        range=[
                            data["Max Rules"].min() - 1,
                            data["Max Rules"].max() + 1,
                        ],
                        label="Max Rules",
                        values=data["Max Rules"],
                        tickvals=np.arange(
                            data["Max Rules"].min() - 1, data["Max Rules"].max() + 2
                        ),
                    ),
                    dict(
                        range=[-1, 2],
                        label="Partial Observation",
                        values=data["Partial Observation"],
                        tickvals=data["Partial Observation"].unique(),
                        ticktext=["True", "False"],
                    ),
                    dict(
                        range=[
                            data["Active Rules"].min() - 1,
                            data["Active Rules"].max() + 1,
                        ],
                        label="Active Rules",
                        values=data["Active Rules"],
                    ),
                    dict(
                        range=[data["Training"].min() - 0.05, 1],
                        label="Training Accuracy",
                        values=data["Training"],
                    ),
                    dict(
                        range=[data["Testing"].min() - 0.05, 1],
                        label="Testing Accuracy",
                        values=data["Testing"],
                    ),
                ]
            ),
        )
    )

    fig.show()


def main():
    if len(sys.argv) < 2:
        print("One or more directories, and at the end a labels file are required.")
        exit(1)
    else:
        labels = Path(sys.argv[-1])

        if labels.exists():
            if labels.is_file():
                directories = []
                for directory in sys.argv[1:-1]:
                    current_dir = Path(directory)
                    if current_dir.exists():
                        if current_dir.is_dir():
                            directories.append(current_dir)
                        else:
                            print(f"'{directory}' is not a valid directory.")
                            break

                    else:
                        print(f"'{directory}' is does not exist")
                        break
                create_parallel_hyperparameters(directories, labels)
                exit(0)
            else:
                print(f"'{labels.name}' is not a valid file.")
        else:
            print(f"'{labels.name}' does not exist.")
        exit(1)


if __name__ == "__main__":
    main()
