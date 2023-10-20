import sys
import os
from pathlib import Path

import foldrpp as fpp
import numpy as np
import pandas as pd
from xgboost import XGBClassifier
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder, OrdinalEncoder

CREATE_MODE = 0o740
EVALUATION_POINTS = 50


# Consider using RIPPER as well.
def main(dataset, label_column):
    label_encoder = LabelEncoder()
    data_encoder = OrdinalEncoder()
    data = pd.read_csv(dataset)
    columns = np.ma.masked_equal(data.columns.values, label_column).compressed()
    fpp_classifier = fpp.Classifier(
        attrs=list(columns),
        numeric=[],
        label=label_column,
        pos=data[label_column].unique()[0],
    )
    xgboost_classifier = XGBClassifier()

    labels = label_encoder.fit_transform(data.pop(label_column).to_numpy())
    data = data_encoder.fit_transform(data.to_numpy())

    X_train, X_test, y_train, y_test = train_test_split(
        data, labels, test_size=0.2, shuffle=True
    )

    STEP = (len(X_train) - 1) / EVALUATION_POINTS
    current_index = 0

    fpp_results = {"train": [], "test": []}
    xgboost_results = {"train": [], "test": []}
    evaluated_points = []

    for _ in range(EVALUATION_POINTS + 1):
        rounded_index = round(current_index)
        print(f"{rounded_index + 1} / {len(X_train)}")

        evaluated_points.append(rounded_index + 1)

        fpp_classifier.fit(X_train[:rounded_index], y_train[:rounded_index])
        xgboost_classifier.fit(X_train[:rounded_index], y_train[:rounded_index])

        acc, _, _, _ = fpp.get_scores(fpp_classifier.predict(X_train), y_train)
        fpp_results["train"].append(f"{acc}")
        acc, _, _, _ = fpp.get_scores(fpp_classifier.predict(X_test), y_test)
        fpp_results["test"].append(f"{acc}")

        xgboost_results["train"].append(f"{xgboost_classifier.score(X_train, y_train)}")
        xgboost_results["test"].append(f"{xgboost_classifier.score(X_test, y_test)}")
        current_index += STEP

    current_path = Path(os.getcwd())
    experiments_path = current_path / "sota_experiments"
    experiments_path.mkdir(CREATE_MODE, True, True)

    for method, results in zip(["foldrpp", "xgboost"], [fpp_results, xgboost_results]):
        with (experiments_path / f"{method}_results.csv").open("w") as train:
            train.write(f"instance,training_acc,testing_acc\n")
            for evaluation_point, train_result, test_result in zip(
                evaluated_points, results["train"], results["test"]
            ):
                train.write(f"{evaluation_point},{train_result},{test_result}\n")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
