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

sys.path.insert(0, str((Path(__file__).parent.parent / "plotting_scripts/").resolve()))
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
        abs((y != y_hat).sum() - abstained) / y.shape[0],
    )


@dataclass
class AprioriRule:
    body: list[str]
    head: list[str]


@dataclass(init=False)
class Apriori:
    rules: list[AprioriRule]

    def __init__(self, rules: list[str]) -> None:
        self.rules = []
        for rule in rules:
            body, head = [
                item.strip().removesuffix("}").removeprefix("{").split(",")
                for item in rule.split("->")
            ]

            self.rules.append(AprioriRule(body, head))

    def predict(self, data: list[list[str]]) -> list[list[str]]:
        inferred: list[list[str]] = [[] for _ in range(len(data))]

        for instance_index, instance in enumerate(data):
            combined = instance
            for rule in self.rules:
                if set(combined).issuperset(set(rule.body)):
                    combined.extend(rule.head)
                    inferred[instance_index].extend(rule.head)

        return inferred

    def score(self, data: list[list[str]], y: list[str]) -> tuple[float, float, float]:
        predictions = self.predict(data)

        correct = 0
        incorrect = 0
        abstained = 0
        missing = 0
        possible_labels = list(set(y))
        try:
            possible_labels.remove("")
        except ValueError:
            pass

        for index, y_hat in enumerate(predictions):
            if y[index] == "":
                missing += 1
            elif y[index] in y_hat:
                correct += 1
            elif set(y_hat).intersection(set(possible_labels)):
                incorrect += 1
            else:
                abstained += 1

        cor = correct / (len(y) - missing)
        abst = abstained / (len(y) - missing)
        inc = incorrect / (len(y) - missing)

        return cor, abst, inc


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
        if self.error == other.error:
            return self.location < other.location
        return self.error < other.error

    def __gt__(self, other):
        if not isinstance(other, Point):
            raise ValueError
        if self.error == other.error:
            return self.location > other.error
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

        model: frm.Classifier | DecisionTreeClassifier | XGBClassifier | Nerd | Apriori = (
            None
        )
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
            elif algorithm == "apriori":
                with (trial / instance_to_evaluate).open("r") as f:
                    rules = []
                    for line in f.readlines():
                        rules.append(line.strip())
                    model = Apriori(rules)
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

            if algorithm == "apriori":
                # model.predict()
                X_train = X_train.tolist()
                y_train = y_train.tolist()
                X_test = X_test.tolist()
                y_test = y_test.tolist()
                columns = train.columns.tolist()

                for dataset, labels in zip([X_train, X_test], [y_train, y_test]):
                    for tr_index, transaction in enumerate(dataset):
                        indices_to_remove: list[int] = []
                        for index, literal in enumerate(transaction):
                            if literal == "":
                                indices_to_remove.append(index)
                            else:
                                dataset[tr_index][index] = f"{columns[index]}_{literal}"

                        for index in list(reversed(indices_to_remove)):
                            del dataset[tr_index][index]

                        if labels[tr_index] != "":
                            labels[tr_index] = f"{label}_{labels[tr_index]}"

                cor, abst, inc = model.score(X_train, y_train)

                train_cor.append(cor)
                train_abst.append(abst)
                train_inc.append(inc)

                cor, abst, inc = model.score(X_test, y_test)

                test_cor.append(cor)
                test_abst.append(abst)
                test_inc.append(inc)
            else:
                if algorithm != "foldrm":
                    data_encoder = OrdinalEncoder()

                    label_encoding = {}
                    label_to_endode = np.append(y_train, y_test)
                    for i in range(len(label_to_endode)):
                        if (
                            not str(label_to_endode[i]).isdigit()
                            and not label_to_endode[i] in label_encoding.keys()
                        ):
                            current_label = label_to_endode[i]
                            label_encoding[current_label] = len(label_encoding)
                            y_train[y_train == current_label] = label_encoding[
                                current_label
                            ]
                            y_test[y_test == current_label] = label_encoding[
                                current_label
                            ]

                    y_train = y_train.astype(int)
                    y_test = y_test.astype(int)
                    data_encoder.fit(
                        np.concatenate((X_train, X_test), axis=0).astype(str)
                    )
                    X_train = data_encoder.transform(X_train.astype(str))
                    X_test = data_encoder.transform(X_test.astype(str))

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
            cor, abst, inc = calculate_correct_abstained_incorrect(
                train,
                trial / "results" / instance_to_evaluate / "train.txt",
                labels,
            )
            train_cor.append(cor)
            train_abst.append(abst)
            train_inc.append(inc)

            cor, abst, inc = calculate_correct_abstained_incorrect(
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
        np.mean(train_inc),
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

    if algorithm != "nerd":
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

    point_to_draw: Point = queue.get()

    while len(points_drawn) != points_to_draw:
        print(f"Chosen: {point_to_draw}", flush=True)
        new_left_point = Point(
            left_parent=point_to_draw.left_parent, right_parent=point_to_draw
        )

        if new_left_point.location != -1 and new_left_point not in points_drawn:
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

            queue.put(new_left_point)

        new_right_point = Point(
            left_parent=point_to_draw, right_parent=point_to_draw.right_parent
        )

        if new_right_point.location != -1 and new_right_point not in points_drawn:
            train_results, test_results = calculate_train_test_mean_acc(
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
            new_right_point.test_results = test_results

            queue.put(new_right_point)

        point_to_draw: Point = queue.get()
        if point_to_draw not in points_drawn:
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
