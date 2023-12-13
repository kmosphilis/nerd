from __future__ import annotations

import os
import pickle
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from queue import PriorityQueue

import numpy as np
import pandas as pd
from sklearn.preprocessing import OrdinalEncoder
from sklearn.tree import DecisionTreeClassifier

import foldrm as frm
from xgboost import XGBClassifier

sys.path.insert(1, "plotting_scripts/")
from create_results_plot import calculate_correct_abstained_incorrect


def correct_abstained_incorrect_score(
    y: np.ndarray, y_hat: np.ndarray
) -> tuple[float, float, float]:
    assert y.shape == y_hat.shape

    y_with_labels = np.argwhere(y != "")
    y = y[y_with_labels]
    y_hat = y_hat[y_with_labels]

    abstained = (y_hat == None).sum()
    return (
        (y == y_hat).sum() / y.shape[0],
        abstained / y.shape[0],
        (y != y_hat).sum() / y.shape[0],
    )


@dataclass
class Nerd:
    test_location: Path
    instance: str

    def predict(self, labels: Path, X_test: Path) -> None:
        if (
            subprocess.run(
                f"{Path(__file__).resolve().parent.parent/'bin'/'evaluation'} "
                f"{(self.test_location/'info').resolve()} "
                f"{(self.test_location/self.instance)} {labels.resolve()} "
                f"{X_test.resolve()} true",
                shell=True,
            ).returncode
            != 0
        ):
            exit(7)


@dataclass(eq=False)
class Point:
    error: int = 0
    location: int = -1
    train_results: tuple[float, float, float] = (0, 0, 0)
    test_results: tuple[float, float, float] = (0, 0, 0)
    left_parent: Point | None = None
    right_parent: Point | None = None

    def __post_init__(self):
        if self.left_parent is not None and self.right_parent is not None:
            self.location = round(
                (self.right_parent.location + self.left_parent.location) / 2
            )

    def to_csv(self) -> str:
        return (
            f"{self.location}, {self.error}, {self.train_results}, {self.test_results}"
        )

    def __str__(self) -> str:
        return (
            f"Location: {self.location}, Train results: {self.train_results}, "
            f"Test results: {self.test_results}"
        )

    def __eq__(self, other):
        if not isinstance(other, Point):
            raise ValueError
        return self.location == other.location

    def __lt__(self, other):
        if not isinstance(other, Point):
            raise ValueError
        return self.error < other.error

    def __gt__(self, other):
        if not isinstance(other, Point):
            raise ValueError
        return self.error > other.error

    def __le__(self, other):
        return self.__eq__(other) or self.__lt__(other)

    def __ge__(self, other):
        return self.__eq__(other) or self.__gt__(other)


def calculate_train_test_mean_acc(
    algorithm: str,
    trials: list[Path],
    instance_to_evaluate: str,
    label: str,
    dataset_location: Path | None,
) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
    train_cor = []
    train_abst = []
    train_inc = []
    test_cor = []
    test_abst = []
    test_inc = []

    for trial in trials:
        current_training_data: Path = None
        current_testing_data: Path = None

        model: frm.Classifier | DecisionTreeClassifier | XGBClassifier | Nerd = None
        if algorithm != "nerd":
            with (trial / "info").open("r") as f:
                for line in f.readlines():
                    if "training_dataset=" in line:
                        current_training_data = Path(
                            line.strip().removeprefix("training_dataset=")
                        )
                    elif "testing_dataset=" in line:
                        current_testing_data = Path(
                            line.strip().removeprefix("testing_dataset=")
                        )

            if current_training_data is None:
                print("Training file is missing.")
                exit(6)
            elif not current_training_data.exists():
                print(f"'{current_training_data.name}' does not exist.")
                exit(6)

            if current_testing_data is None:
                print("Testing file is missing.")
                exit(7)
            elif not current_testing_data.exists():
                print(f"'{current_testing_data.name}' does not exist.")
                exit(7)

            if (
                subprocess.run(
                    f"tar -x -I 'zstd -d' -C {trial} "
                    f"-f {trial}/models.tar.zst {instance_to_evaluate}",
                    shell=True,
                ).returncode
                != 0
            ):
                exit(-1)

            if algorithm == "xgboost":
                model = XGBClassifier()
                model.load_model(trial / instance_to_evaluate)
            else:
                with (trial / instance_to_evaluate).open("rb") as f:
                    model = pickle.load(f)

            os.remove(trial / instance_to_evaluate)
            train = pd.read_csv(current_training_data)
            test = pd.read_csv(current_testing_data)

            y_train = train[label].replace(np.nan, "").values
            X_train = train.drop(columns=label).replace(np.nan, "").values
            y_test = test[label].replace(np.nan, "").values
            X_test = test.drop(columns=label).replace(np.nan, "").values

            if algorithm != "foldrm":
                data_encoder = OrdinalEncoder()

                label_encoding = {}
                for i in range(y_train.shape[0]):
                    if (
                        not str(y_train[i]).isdigit()
                        and not y_train[i] in label_encoding.keys()
                    ):
                        current_label = y_train[i]
                        label_encoding[current_label] = len(label_encoding)
                        y_train[y_train == current_label] = label_encoding[
                            current_label
                        ]
                        y_test[y_test == current_label] = label_encoding[current_label]

                y_train = y_train.astype(int)
                y_test = y_test.astype(int)
                X_train = data_encoder.fit_transform(X_train)
                X_test = data_encoder.transform(X_test)

            cor, abst, inc = correct_abstained_incorrect_score(
                y_train, np.array(model.predict(X_train))
            )
            train_cor.append(cor)
            train_abst.append(abst)
            train_inc.append(inc)

            cor, abst, inc = correct_abstained_incorrect_score(
                y_test, np.array(model.predict(X_test))
            )
            test_cor.append(cor)
            test_abst.append(abst)
            test_inc.append(inc)

        else:
            model = Nerd(trial, instance_to_evaluate)

            if (trial / "kbs.tar.zst").exists():
                if (
                    subprocess.run(
                        f"tar -x -I 'zstd -d' -C {trial} "
                        f"-f {trial}/kbs.tar.zst {instance_to_evaluate}",
                        shell=True,
                    ).returncode
                    != 0
                ):
                    exit(-1)

            trial_no = int(re.findall(r"\d+", trial.name)[-1])

            model.predict(
                dataset_location / "labels.txt",
                dataset_location / f"testing_dataset{trial_no}.csv",
            )

            labels = []
            with (dataset_location / "labels.txt").open("r") as f:
                labels = f.readline().strip().split(" ")

            if (
                not (trial / "results" / "training-dataset.txt").exists()
                or not (trial / "results" / "testing-dataset.txt").exists()
            ):
                if (
                    subprocess.run(
                        f"{Path(__file__).resolve().parent.parent/'bin'/'extract_observations'} "
                        f"{trial/'info'} {dataset_location/f'testing_dataset{trial_no}.csv'} "
                        f"true",
                        shell=True,
                    ).returncode
                    != 0
                ):
                    exit(9)

            train: list[str] = []
            with (trial / "results" / "training-dataset.txt").open("r") as f:
                for l in f.readlines():
                    train.append(l.strip().split(" "))

            test: list[str] = []
            with (trial / "results" / "testing-dataset.txt").open("r") as f:
                for l in f.readlines():
                    test.append(l.strip().split(" "))

            assert (trial / "results" / instance_to_evaluate / "train.txt").exists()
            cor, abst, cor = calculate_correct_abstained_incorrect(
                train,
                trial / "results" / instance_to_evaluate / "train.txt",
                labels,
            )
            train_cor.append(cor)
            train_abst.append(abst)
            train_inc.append(inc)

            cor, abst, cor = calculate_correct_abstained_incorrect(
                test,
                trial / "results" / instance_to_evaluate / "test.txt",
                labels,
            )
            test_cor.append(cor)
            test_abst.append(abst)
            test_inc.append(inc)

    return (
        np.mean(train_cor),
        np.mean(train_abst),
        np.mean(inc),
    ), (
        np.mean(test_cor),
        np.mean(test_abst),
        np.mean(test_inc),
    )


def main(
    algorithm: str,
    experiment_path: Path,
    dataset_location: Path | None,
    label: str,
    points_to_draw: int = 50,
):
    points_drawn: list[Point] = []
    queue = PriorityQueue()
    results = dict()

    if algorithm == "foldrm" or algorithm == "decision_tree" or algorithm == "xgboost":
        trials = list(sorted(experiment_path.glob("trial*")))
        for trial in trials:
            if trial.is_dir():
                if not (trial / "models.tar.zst").exists():
                    exit(2)

        for trial in trials:
            results[trial.name] = subprocess.run(
                f"tar -t -I 'zstd -d' -f {trial.absolute()}/models.tar.zst | sort -V",
                text=True,
                capture_output=True,
                shell=True,
            ).stdout.splitlines()

    else:
        trials = list(sorted(experiment_path.glob("timestamp*")))
        for trial in trials:
            zst = trial / "kbs.tar.zst"

            if zst.exists():
                results[trial.name] = subprocess.run(
                    f"tar -t -I 'zstd -d' -f {trial.absolute()}/kbs.tar.zst | "
                    "sort -V",
                    text=True,
                    capture_output=True,
                    shell=True,
                ).stdout.splitlines()

            else:
                results[trial.name] = list(
                    filter(
                        lambda x: ".temp" not in x,
                        subprocess.run(
                            f"find {trial.absolute()} -regex '.*nd' "
                            "-type f -printf '%f\n' | sort -V",
                            text=True,
                            capture_output=True,
                            shell=True,
                        ).stdout.splitlines(),
                    )
                )

    values = list(results.values())

    for i in range(1, len(values)):
        if len(values[i - 1]) != len(values[i]):
            exit(3)

    start = Point(location=0)
    end = Point(location=len(values[0]))

    train_results, test_results = calculate_train_test_mean_acc(
        algorithm,
        trials,
        values[0][-1],
        label,
        dataset_location,
    )

    end.train_results = train_results
    end.test_results = test_results

    linear_values = np.linspace(0, test_results[0], len(values[0]), endpoint=True)

    middle = Point(
        left_parent=start,
        right_parent=end,
    )

    train_results, test_results = calculate_train_test_mean_acc(
        algorithm,
        trials,
        values[0][middle.location],
        label,
        dataset_location,
    )

    middle.error = -abs(test_results[0] - linear_values[middle.location])
    middle.train_results = train_results
    middle.test_results = test_results

    points_drawn.append(start)
    points_drawn.append(end)
    points_drawn.append(middle)

    queue.put(start)
    queue.put(end)
    queue.put(middle)

    print(f"Chosen: {start}\nChosen: {end}")

    point_to_draw: Point = queue.get()

    while len(points_drawn) != points_to_draw:
        print(f"Chosen: {point_to_draw}")
        new_left_point = Point(
            left_parent=point_to_draw.left_parent, right_parent=point_to_draw
        )
        train_results, test_results = calculate_train_test_mean_acc(
            algorithm,
            trials,
            values[0][new_left_point.location],
            label,
            dataset_location,
        )
        new_left_point.error = -abs(
            test_results[0] - linear_values[new_left_point.location]
        )
        new_left_point.train_results = train_results
        new_left_point.test_results = test_results

        if new_left_point not in points_drawn:
            queue.put(new_left_point)

        new_right_point = Point(
            left_parent=point_to_draw, right_parent=point_to_draw.right_parent
        )
        train_results, _ = calculate_train_test_mean_acc(
            algorithm,
            trials,
            values[0][new_right_point.location],
            label,
            dataset_location,
        )
        new_right_point.error = -abs(
            test_results[0] - linear_values[new_right_point.location]
        )

        new_right_point.train_results = train_results
        new_left_point.test_results = test_results

        if new_right_point not in points_drawn:
            queue.put(new_right_point)

        point_to_draw: Point = queue.get()
        points_drawn.append(point_to_draw)

    with (experiment_path / f"plotting-points_{points_to_draw}.txt").open("w") as f:
        for point in points_drawn:
            f.write(f"{point.to_csv()}\n")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(
            "Provide the algorithms, the directory of the experiments, the dataset "
            "directory, and the label of the dataset."
        )
        exit(1)

    algorithm = sys.argv[1]
    experiment_path = Path(sys.argv[2])

    data_path = Path(sys.argv[3])
    label = sys.argv[4]

    main(
        algorithm,
        experiment_path,
        data_path,
        label,
    )
