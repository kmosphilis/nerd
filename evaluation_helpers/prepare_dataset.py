import sys
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split

SEEDS = [0, 42, 314, 271, 3141592]


def partial_observation(rng: np.random.Generator, data: pd.DataFrame) -> pd.DataFrame:
    for i in range(len(data.values)):
        literals_to_hide = rng.integers(1, len(data.values[i]), endpoint=True)
        if literals_to_hide < len(data.values[i]):
            literals_to_remove = np.sort(
                rng.integers(0, len(data.values[i]), size=literals_to_hide)
            )
            for j in literals_to_remove[::-1]:
                data.iloc[i].iloc[j] = ""

    return data


def main(dataset: Path, label: str):
    data = pd.read_csv(dataset)
    assert label in data.columns

    for index, current_seed in enumerate(SEEDS):
        rng = np.random.default_rng(seed=np.random.PCG64(current_seed))

        training, testing = train_test_split(
            data,
            test_size=0.2,
            random_state=current_seed,
            shuffle=True,
            stratify=data[label].values,
        )

        training.to_csv(dataset.parent / f"training_dataset{index}.csv", index=False)
        testing.to_csv(dataset.parent / f"testing_dataset{index}.csv", index=False)

        partial_training = partial_observation(rng, training)

        partial_training.to_csv(
            dataset.parent / f"partial_training_dataset{index}.csv", index=False
        )


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("The dataset to be converted and its label name are required.")
        exit(1)

    dataset = Path(sys.argv[1])
    if dataset.exists():
        if dataset.is_file():
            main(dataset, sys.argv[2])
            exit(0)
        else:
            print(f"'{dataset.name}' is not a file.")
    else:
        print(f"'{dataset.name}' does not exist.")

    exit(2)
