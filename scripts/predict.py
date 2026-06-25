import argparse
import json
from pathlib import Path

import joblib
import numpy as np
import pandas as pd


FEATURE_ORDER = ["Time"] + [f"V{i}" for i in range(1, 29)] + ["Amount"]


def load_json(path: Path):
    with open(path, "r") as f:
        return json.load(f)


def build_row(payload: dict, feature_order):
    if "features" in payload and isinstance(payload["features"], list):
        arr = payload["features"]
        if len(arr) != len(feature_order):
            raise ValueError(f"'features' must contain {len(feature_order)} values.")
        row = {f: float(v) for f, v in zip(feature_order, arr)}
        return row

    row = {}
    for f in feature_order:
        row[f] = float(payload.get(f, 0.0))
    return row


def standardize_row(row: dict, stats: dict):
    row = dict(row)
    for key in ["Time", "Amount"]:
        mean = float(stats[key]["mean"])
        std = float(stats[key]["std"])
        std = std if std != 0 else 1.0
        row[key] = (float(row[key]) - mean) / std
    return row


def explain_transaction(row_std, lr, rf, xgb, feature_order):
    # Anonymized features: provide operational explainability, not semantic labels.
    lr_coef = np.abs(lr.coef_[0])
    if hasattr(rf, "feature_importances_"):
        rf_imp = np.asarray(rf.feature_importances_, dtype=float)
    else:
        rf_imp = np.zeros(len(feature_order), dtype=float)
    if hasattr(xgb, "feature_importances_"):
        xgb_imp = np.asarray(xgb.feature_importances_, dtype=float)
    else:
        xgb_imp = np.zeros(len(feature_order), dtype=float)

    def norm(v):
        s = float(v.sum())
        return v / s if s > 0 else v

    combined_importance = 0.35 * norm(lr_coef) + 0.30 * norm(rf_imp) + 0.35 * norm(xgb_imp)
    x = np.asarray([row_std[f] for f in feature_order], dtype=float)
    contributions = np.abs(combined_importance * x)

    top_idx = np.argsort(contributions)[::-1][:5]
    explanations = []
    for i in top_idx:
        explanations.append({
            "feature": feature_order[i],
            "contribution": round(float(contributions[i]), 6),
            "note": "High absolute contribution in the ensemble score"
        })

    return explanations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="Path to input JSON transaction file")
    parser.add_argument("--output", required=True, help="Path to output JSON file")
    parser.add_argument("--models", default="models", help="Model directory")
    args = parser.parse_args()

    models_dir = Path(args.models)
    payload = load_json(Path(args.input))
    meta_path = models_dir / "model_metadata.json"
    stats_path = models_dir / "preprocess_stats.json"

    metadata = load_json(meta_path)
    stats = load_json(stats_path)

    feature_order = metadata.get("feature_order", FEATURE_ORDER)
    row = build_row(payload, feature_order)
    row_std = standardize_row(row, stats)

    X = pd.DataFrame([row_std], columns=feature_order)

    lr = joblib.load(models_dir / "logistic_regression.pkl")
    rf = joblib.load(models_dir / "random_forest.pkl")
    xgb = joblib.load(models_dir / "xgboost.pkl")

    lr_prob = float(lr.predict_proba(X)[:, 1][0])
    rf_prob = float(rf.predict_proba(X)[:, 1][0])
    xgb_prob = float(xgb.predict_proba(X)[:, 1][0])

    ensemble_prob = 0.2 * lr_prob + 0.3 * rf_prob + 0.5 * xgb_prob
    risk_score = round(ensemble_prob * 100.0, 2)

    if risk_score >= 80:
        alert_level = "HIGH"
    elif risk_score >= 50:
        alert_level = "MEDIUM"
    else:
        alert_level = "LOW"

    explanations = explain_transaction(row_std, lr, rf, xgb, feature_order)
    consensus = {
        "logistic_regression": "FRAUD" if lr_prob >= 0.5 else "LEGITIMATE",
        "random_forest": "FRAUD" if rf_prob >= 0.5 else "LEGITIMATE",
        "xgboost": "FRAUD" if xgb_prob >= 0.5 else "LEGITIMATE"
    }

    output = {
        "transaction_id": payload.get("transaction_id", ""),
        "fraud_probability": round(float(ensemble_prob), 6),
        "risk_score": risk_score,
        "alert_level": alert_level,
        "model_probabilities": {
            "logistic_regression": round(lr_prob, 6),
            "random_forest": round(rf_prob, 6),
            "xgboost": round(xgb_prob, 6)
        },
        "ensemble_weights": {
            "logistic_regression": 0.2,
            "random_forest": 0.3,
            "xgboost": 0.5
        },
        "consensus": consensus,
        "explanations": explanations,
        "interpretation": (
            "Operational explainability based on weighted model contributions "
            "and anonymized feature influence. Kaggle features V1-V28 are PCA-transformed, "
            "so the explanations are model-centric rather than human-semantic."
        )
    }

    with open(args.output, "w") as f:
        json.dump(output, f, indent=2)


if __name__ == "__main__":
    main()
