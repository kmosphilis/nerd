import os
import pickle
import subprocess
import sys
import time
from pathlib import Path

import numpy as np
import pandas as pd
import tqdm
from efficient_apriori import apriori
from sklearn.preprocessing import OrdinalEncoder
from sklearn.tree import DecisionTreeClassifier

import foldrm as frm
from xgboost import XGBClassifier

CREATE_MODE = 0o740


def main(
    training_dataset_path: Path,
    testing_dataset_path: Path,
    label: str,
    seed: int | None = None,
):
    training_data = pd.read_csv(training_dataset_path)
    testing_data = pd.read_csv(testing_dataset_path)
    assert label in training_data.columns and label in testing_data.columns

    columns = np.ma.masked_equal(training_data.columns.values, label).compressed()

    y_train = training_data[label].replace(np.nan, "").values
    X_train = training_data.drop(columns=label).replace(np.nan, "").values

    y_test = testing_data[label].replace(np.nan, "").values
    X_test = testing_data.drop(columns=label).replace(np.nan, "").values

    current_dir = Path(os.getcwd())

    apriori_dir = current_dir / "apriori"
    apriori_dir.mkdir(CREATE_MODE, exist_ok=True)
    apriori_trial: Path = None
    folder_created = False

    while not folder_created:
        try:
            apriori_trial = (
                apriori_dir / f"trial={len(list(apriori_dir.glob('trial=*')))}"
            )
            apriori_trial.mkdir(CREATE_MODE)
            folder_created = True
        except FileExistsError:
            pass

    with (apriori_trial / "info").open("w") as info:
        info.write(f"seed={seed}\n")
        info.write(f"training_dataset={training_dataset_path.resolve()}\n")
        info.write(f"testing_dataset={testing_dataset_path.resolve()}\n")

    print("Apriori")
    columns_and_label = columns.tolist()
    columns_and_label.append(label)
    for i in tqdm.tqdm(range(1, len(X_train) + 1)):
        combined = np.append(
            X_train[:i], np.reshape(y_train[:i], (y_train[:i].shape[0], 1)), axis=1
        ).tolist()

        for tr_index, transaction in enumerate(combined):
            indices_to_remove: list[int] = []
            for index, literal in enumerate(transaction):
                if literal == "":
                    indices_to_remove.append(index)
                else:
                    combined[tr_index][index] = f"{columns_and_label[index]}_{literal}"

            for index in list(reversed(indices_to_remove)):
                del combined[tr_index][index]

        combined = [tuple(item) for item in combined]
        _, rules = apriori(combined, max_length=len(columns_and_label))

        with (apriori_trial / f"iteration_{i}.txt").open("w") as file:
            rules = [f"{repr(rule)}\n" for rule in rules]
            file.writelines(rules)

    foldrm_dir = current_dir / "foldrm"
    foldrm_dir.mkdir(CREATE_MODE, exist_ok=True)
    foldrm_trial: Path = None

    folder_created = False
    while not folder_created:
        try:
            foldrm_trial = foldrm_dir / f"trial={len(list(foldrm_dir.glob('trial=*')))}"
            foldrm_trial.mkdir(CREATE_MODE)
            folder_created = True
        except FileExistsError:
            pass

    with (foldrm_trial / "info").open("w") as info:
        info.write(f"seed={seed}\n")
        info.write(f"training_dataset={training_dataset_path.resolve()}\n")
        info.write(f"testing_dataset={testing_dataset_path.resolve()}\n")

    frm_classifer = frm.Classifier(
        attrs=list(columns) + [label], numeric=[], label=label
    )
    total_time = 0

    print("FOLD-RM")
    for i in tqdm.tqdm(range(1, len(X_train) + 1)):
        combined = np.append(
            X_train[:i], np.reshape(y_train[:i], (y_train[:i].shape[0], 1)), axis=1
        ).tolist()

        starting_time = int(time.perf_counter_ns() * 1e-6)
        frm_classifer.fit(combined)
        ending_time = int(time.perf_counter_ns() * 1e-6)

        total_time += ending_time - starting_time

        with (foldrm_trial / f"iteration_{i}.pickle").open("wb") as file:
            pickle.dump(frm_classifer, file)
    print(
        f"Online version took: {total_time} ms\nOffline version took: "
        f"{ending_time - starting_time} ms"
    )

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
            y_train[y_train == current_label] = label_encoding[current_label]
            y_test[y_test == current_label] = label_encoding[current_label]

    y_train = y_train.astype(int)
    y_test = y_test.astype(int)
    data_encoder.fit(np.concatenate((X_train, X_test), axis=0).astype(str))
    X_train = data_encoder.transform(X_train.astype(str))
    X_test = data_encoder.transform(X_test.astype(str))

    decision_tree_dir = current_dir / "decision_tree"
    decision_tree_dir.mkdir(CREATE_MODE, exist_ok=True)
    decision_tree_trial: Path = None

    folder_created = False
    while not folder_created:
        try:
            decision_tree_trial = (
                decision_tree_dir
                / f"trial={len(list(decision_tree_dir.glob('trial=*')))}"
            )
            decision_tree_trial.mkdir(CREATE_MODE)
            folder_created = True
        except FileExistsError:
            pass

    with (decision_tree_trial / "info").open("w") as info:
        info.write(f"seed={seed}\n")
        info.write(f"training_dataset={training_dataset_path.resolve()}\n")
        info.write(f"testing_dataset={testing_dataset_path.resolve()}\n")

    decision_tree = DecisionTreeClassifier(random_state=seed)
    total_time = 0

    print("\nDecision Tree")
    for i in tqdm.tqdm(range(1, len(X_train) + 1)):
        starting_time = int(time.perf_counter_ns() * 1e-6)
        decision_tree.fit(X_train[:i], y_train[:i])
        ending_time = int(time.perf_counter_ns() * 1e-6)

        total_time += ending_time - starting_time

        with (decision_tree_trial / f"iteration_{i}.pickle").open("wb") as file:
            pickle.dump(decision_tree, file)
    print(
        f"Online version took: {total_time} ms\nOffline version took: "
        f"{ending_time - starting_time} ms"
    )

    xgboost_dir = current_dir / "xgboost"
    xgboost_dir.mkdir(CREATE_MODE, exist_ok=True)
    xgboost_trial: Path = None

    folder_created = False
    while not folder_created:
        try:
            xgboost_trial = (
                xgboost_dir / f"trial={len(list(xgboost_dir.glob('trial=*')))}"
            )
            xgboost_trial.mkdir(CREATE_MODE)
            folder_created = True
        except FileExistsError:
            pass

    with (xgboost_trial / "info").open("w") as info:
        info.write(f"seed={seed}\n")
        info.write(f"training_dataset={training_dataset_path.resolve()}\n")
        info.write(f"testing_dataset={testing_dataset_path.resolve()}\n")

    xgboost_classifier = XGBClassifier(random_state=seed)
    total_time = 0

    print("\nXGBoost")
    for i in tqdm.tqdm(range(1, len(X_train) + 1)):
        starting_time = int(time.perf_counter_ns() * 1e-6)
        xgboost_classifier.fit(X_train[:i], y_train[:i])
        ending_time = int(time.perf_counter_ns() * 1e-6)

        total_time += ending_time - starting_time

        file = xgboost_trial / f"iteration_{i}.json"
        xgboost_classifier.save_model(file)

    print(
        f"Online version took: {total_time} ms\nOffline version took: "
        f"{ending_time - starting_time} ms"
    )

    for file_type, dir in zip(
        [".txt", ".pickle", ".pickle", ".json"],
        [apriori_trial, foldrm_trial, decision_tree_trial, xgboost_trial],
    ):
        if (
            subprocess.run(
                f"cd {dir.absolute()} && tar -c -I 'zstd -5' -f models.tar.zst *{file_type}",
                shell=True,
            ).returncode
            == 0
        ):
            for f in dir.glob(f"*{file_type}"):
                os.remove(f)


if __name__ == "__main__":
    if 5 < len(sys.argv) or len(sys.argv) < 4:
        print(
            "A training dataset, a testing dataset, and the name of its label are "
            "required. Optionally a random seed can be given in the end."
        )
        exit(1)

    seed = None
    if len(sys.argv) == 5:
        if sys.argv[4].isdigit():
            seed = int(sys.argv[4])
        else:
            print(f"The given seed {seed} is not valid. Please provide a number")
            exit(2)

    training_dataset = Path(sys.argv[1])
    testing_dataset = Path(sys.argv[2])

    if training_dataset.exists():
        if training_dataset.is_file():
            if testing_dataset.exists():
                if testing_dataset.is_file():
                    main(training_dataset, testing_dataset, sys.argv[3], seed=seed)
                    exit(0)
                else:
                    print(f"'{testing_dataset.name}' is not a file.")
            else:
                print(f"'{testing_dataset.name}' does not exist.")
        else:
            print(f"'{training_dataset.name}' is not a file.")
    else:
        print(f"'{training_dataset.name}' does not exist.")

    exit(2)
