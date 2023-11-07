import sys
import os
from pathlib import Path
import multiprocessing as mp

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import plotly.graph_objects as go
from plotly.subplots import make_subplots

from create_results_plot import calculate_training_testing_actives


def calculate_instance_requirements(directory: Path, labels: Path, iteration: int):
    print(f"current experiment: {directory.name}", flush=True)
    for trial in Path(directory).iterdir():
        if not "logs" in trial.name and not (trial / "kbs.tar.zst").exists():
            return None
    else:
        (
            training_ratios,
            testing_ratios,
            active_rules,
        ) = calculate_training_testing_actives(
            directory, labels, max_iterations=iteration, only_final=True
        )

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

    return [
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


def create_parallel_hyperparameters(
    directories: list | None, labels: Path, iteration: int, results: Path | None = None
):
    assert directories != results
    data = None
    if results is not None:
        data = pd.read_csv(results.absolute())
    elif directories is not None:
        with mp.Pool(mp.cpu_count()) as pool:
            data = list(
                pool.starmap(
                    calculate_instance_requirements,
                    [(directory, labels, iteration) for directory in directories],
                )
            )

        data = pd.DataFrame(
            list(filter(lambda x: x is not None, data)),
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

    data.sort_values(by=["Testing", "Training"], inplace=True, ascending=False)

    dimension = []
    active_rules = []

    for index, value in enumerate(
        list(zip([False, True], ["Full Observation", "Partial Observation"]))
    ):
        current_data = data.loc[data["Partial Observation"] == value[0]]

        if index == 0:
            testing_accuracy = current_data["Testing"]
            dimension.extend(
                [
                    dict(
                        range=[
                            current_data["Threshold"].min() - 1,
                            current_data["Threshold"].max() + 1,
                        ],
                        label="Threshold",
                        values=current_data["Threshold"],
                        tickvals=current_data["Threshold"].unique(),
                    ),
                    dict(
                        range=[
                            current_data["Promotion"].min() - 1,
                            current_data["Promotion"].max() + 1,
                        ],
                        label="Promotion",
                        values=current_data["Promotion"],
                    ),
                    dict(
                        range=[
                            current_data["Demotion"].min() - 1,
                            current_data["Demotion"].max() + 1,
                        ],
                        label="Demotion",
                        values=current_data["Demotion"],
                    ),
                    dict(
                        range=[
                            current_data["Max Breadth"].min(),
                            current_data["Max Breadth"].max(),
                        ],
                        label="Max Breadth",
                        values=current_data["Max Breadth"],
                        tickvals=current_data["Max Breadth"].unique(),
                    ),
                    dict(
                        range=[
                            current_data["Max Rules"].min(),
                            current_data["Max Rules"].max(),
                        ],
                        label="Max Rules",
                        values=current_data["Max Rules"],
                        tickvals=current_data["Max Rules"].unique(),
                    ),
                ]
            )

        dimension.append(
            dict(
                range=[current_data["Training"].min(), 1],
                label=f"{value[1]} Training Accuracy",
                values=current_data["Training"],
            )
        )
        dimension.append(
            dict(
                range=[current_data["Testing"].min(), 1],
                label=f"{value[1]} Testing Accuracy",
                values=current_data["Testing"],
            )
        )
        active_rules.append(
            dict(
                range=[
                    current_data["Active Rules"].min(),
                    current_data["Active Rules"].max(),
                ],
                label=f"{value[1]} Active Rules",
                values=current_data["Active Rules"],
            )
        )

    dimension.extend(active_rules)

    fig = go.Figure(
        go.Parcoords(
            labelangle=-15,
            line=dict(color=testing_accuracy, colorscale="Plasma"),
            dimensions=dimension,
        )
    )

    directory = Path(os.getcwd())
    directory.mkdir(0o740, True, True)
    fig.update_layout(
        autosize=False,
        height=1200,
        width=2000,
    )

    config = {"scrollZoom": True}

    if results is None:
        data.to_csv(directory / "result.csv", index=False)

    fig.write_html(file=directory / "result.html", config=config)
    fig.write_image(file=directory / "result.pdf", scale=2)


def main():
    if len(sys.argv) < 3:
        print(
            "Provide a labels file, the number of the iteration to plot, and at the "
            "end provide a .csv file with the results, or one or more directories to"
            "perform an evaluation."
        )
        exit(1)
    else:
        labels = Path(sys.argv[1])

        if labels.exists():
            if labels.is_file():
                iteration = sys.argv[2]
                if iteration.isdigit() and (iteration := int(iteration)) > 0:
                    directories = []
                    results = None
                    for directory in sys.argv[2:]:
                        current_dir = Path(directory)
                        if current_dir.exists():
                            if current_dir.is_dir():
                                directories.append(current_dir)
                            else:
                                if len(directories) != 0:
                                    print(f"'{directory}' is not a valid directory.")
                                else:
                                    if current_dir.is_file():
                                        results = current_dir
                                    else:
                                        print(f"'{directory}' is not a valid file.")
                                break

                        else:
                            print(f"'{directory}' does not exist")
                            break
                    else:
                        create_parallel_hyperparameters(
                            directories, labels, iteration=iteration
                        )
                        exit(0)

                    if results is not None:
                        create_parallel_hyperparameters(
                            None, labels, results=results, iteration=iteration
                        )
                        exit(0)
                else:
                    print(f"'{iteration}' is not a valid number.")
            else:
                print(f"'{labels.name}' is not a valid file.")
        else:
            print(f"'{labels.name}' does not exist.")
        exit(1)


if __name__ == "__main__":
    main()
