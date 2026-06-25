import argparse
import json
import os
from pathlib import Path

import joblib
import numpy as np
import pandas as pd
from xgboost import XGBClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, f1_score, precision_score, recall_score, roc_auc_score
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler


FEATURE_ORDER = ["Time"] + [f"V{i}" for i in range(1, 29)] + ["Amount"]


def load_dataset(csv_path: str) -> pd.DataFrame:
    df = pd.read_csv(csv_path)
    if "Class" not in df.columns:
        raise ValueError("Expected target column 'Class' was not found.")
    df = df.dropna(subset=["Class"]).copy()
    return df


def scale_time_amount(df: pd.DataFrame):
    time_scaler = StandardScaler()
    amount_scaler = StandardScaler()

    df = df.copy()
    df["Time"] = time_scaler.fit_transform(df[["Time"]])
    df["Amount"] = amount_scaler.fit_transform(df[["Amount"]])

    stats = {
        "Time": {"mean": float(time_scaler.mean_[0]), "std": float(np.sqrt(time_scaler.var_[0]))},
        "Amount": {"mean": float(amount_scaler.mean_[0]), "std": float(np.sqrt(amount_scaler.var_[0]))},
        "feature_order": FEATURE_ORDER,
        "target": "Class",
        "note": "Only Time and Amount are standardized; the remaining PCA features are used as-is."
    }
    return df, stats


def train(csv_path: str, output_dir: str):
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)

    df = load_dataset(csv_path)
    df, stats = scale_time_amount(df)

    X = df.drop(columns=["Class"])
    y = df["Class"].astype(int)

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )

    # Logistic Regression
    lr = LogisticRegression(class_weight="balanced", max_iter=2000, n_jobs=-1)
    lr.fit(X_train, y_train)

    # Random Forest
    rf = RandomForestClassifier(
        n_estimators=200,
        max_depth=None,
        min_samples_split=2,
        min_samples_leaf=1,
        class_weight="balanced",
        random_state=42,
        n_jobs=-1
    )
    rf.fit(X_train, y_train)

    # XGBoost
    fraud = int(y_train.sum())
    legit = int(len(y_train) - fraud)
    scale_pos_weight = legit / max(fraud, 1)

    xgb = XGBClassifier(
        n_estimators=250,
        max_depth=6,
        learning_rate=0.05,
        subsample=0.8,
        colsample_bytree=0.8,
        objective="binary:logistic",
        eval_metric="logloss",
        scale_pos_weight=scale_pos_weight,
        random_state=42,
        n_jobs=-1,
    )
    xgb.fit(X_train, y_train)

    # Ensemble
    lr_p = lr.predict_proba(X_test)[:, 1]
    rf_p = rf.predict_proba(X_test)[:, 1]
    xgb_p = xgb.predict_proba(X_test)[:, 1]
    ensemble_p = 0.2 * lr_p + 0.3 * rf_p + 0.5 * xgb_p
    ensemble_pred = (ensemble_p >= 0.5).astype(int)

    metrics = {
        "accuracy": float(accuracy_score(y_test, ensemble_pred)),
        "precision": float(precision_score(y_test, ensemble_pred, zero_division=0)),
        "recall": float(recall_score(y_test, ensemble_pred, zero_division=0)),
        "f1_score": float(f1_score(y_test, ensemble_pred, zero_division=0)),
        "roc_auc": float(roc_auc_score(y_test, ensemble_p)),
    }

    metadata = {
        "dataset": "Kaggle Credit Card Fraud Detection",
        "total_samples": int(len(df)),
        "fraud_samples": int(df["Class"].sum()),
        "legitimate_samples": int((df["Class"] == 0).sum()),
        "models": ["Logistic Regression", "Random Forest", "XGBoost"],
        "ensemble_type": "Weighted Voting",
        "weights": {
            "Logistic Regression": 0.2,
            "Random Forest": 0.3,
            "XGBoost": 0.5
        },
        "feature_order": FEATURE_ORDER,
        "target": "Class",
        "preprocessing": {
            "Time": "standardized",
            "Amount": "standardized",
            "OtherFeatures": "used as-is"
        }
    }

    joblib.dump(lr, out / "logistic_regression.pkl")
    joblib.dump(rf, out / "random_forest.pkl")
    joblib.dump(xgb, out / "xgboost.pkl")
    joblib.dump({"Time": stats["Time"], "Amount": stats["Amount"]}, out / "scaler.pkl")

    with open(out / "metrics.json", "w") as f:
        json.dump(metrics, f, indent=2)
    with open(out / "model_metadata.json", "w") as f:
        json.dump(metadata, f, indent=2)
    with open(out / "preprocess_stats.json", "w") as f:
        json.dump(stats, f, indent=2)

    print("Training complete.")
    print(json.dumps(metrics, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True, help="Path to creditcard.csv")
    parser.add_argument("--output", default="models", help="Output directory")
    args = parser.parse_args()
    train(args.csv, args.output)
