import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report, confusion_matrix
import matplotlib.pyplot as plt

def load_dataset_samples(path):
    df = pd.read_csv(path)
    X = df[["num_samples"]].values  # 2D array
    y = df["label"].values
    return X, y

def load_dataset_cycles(path):
    df = pd.read_csv(path)
    feature_cols = [col for col in df.columns if col.startswith("sample_")]
    X = df[feature_cols].values
    y = df["label"].values
    return X, y

def train_and_evaluate(X, y, model_name="Random Forest"):
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.25, random_state=42)
    print(f"Training samples: {len(X_train)}, Test samples: {len(X_test)}")

    if model_name == "Random Forest":
        model = RandomForestClassifier(n_estimators=100, random_state=42)
    elif model_name == "Logistic Regression":
        model = LogisticRegression(max_iter=1000, random_state=42)
    else:
        raise ValueError(f"Unknown model: {model_name}")

    model.fit(X_train, y_train)
    y_pred = model.predict(X_test)

    print(f"===== {model_name} =====")
    print(classification_report(y_test, y_pred))
    print("Confusion Matrix:")
    print(confusion_matrix(y_test, y_pred))

    return model

if __name__ == "__main__":
    # Load dataset 1 (num_samples)
    print("Training on dataset 1 (num_samples per probe)...")
    X1, y1 = load_dataset_samples("sample_count.csv")
    train_and_evaluate(X1, y1, model_name="Random Forest")
    train_and_evaluate(X1, y1, model_name="Logistic Regression")

    # Load dataset 2 (cycle timings)
    print("\nTraining on dataset 2 (full cycles per probe)...")
    X2, y2 = load_dataset_cycles("cycles_count.csv")

    # Optional: mask padding (-1) if necessary later

    train_and_evaluate(X2, y2, model_name="Random Forest")
    train_and_evaluate(X2, y2, model_name="Logistic Regression")