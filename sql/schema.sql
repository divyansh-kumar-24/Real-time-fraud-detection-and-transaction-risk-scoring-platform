PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS transactions (
    transaction_id TEXT PRIMARY KEY,
    request_json TEXT NOT NULL,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS predictions (
    prediction_id INTEGER PRIMARY KEY AUTOINCREMENT,
    transaction_id TEXT NOT NULL UNIQUE,
    fraud_probability REAL NOT NULL,
    risk_score REAL NOT NULL,
    alert_level TEXT NOT NULL,
    model_probabilities_json TEXT NOT NULL,
    explanations_json TEXT NOT NULL,
    prediction_json TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (transaction_id) REFERENCES transactions(transaction_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS alerts (
    alert_id INTEGER PRIMARY KEY AUTOINCREMENT,
    transaction_id TEXT NOT NULL,
    alert_level TEXT NOT NULL,
    message TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (transaction_id) REFERENCES transactions(transaction_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_predictions_transaction_id ON predictions(transaction_id);
CREATE INDEX IF NOT EXISTS idx_alerts_transaction_id ON alerts(transaction_id);
