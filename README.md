# Real-Time Fraud Detection and Risk Scoring Platform

## Overview

This project is a **real-time fraud detection backend** that combines a trained machine learning ensemble with a C++17 service layer and SQLite persistence.

It is designed to simulate a practical fintech workflow:

1. A transaction arrives through a REST API.
2. The backend validates and serializes the request.
3. A Python inference layer loads the trained models and produces a fraud probability.
4. The C++ service converts that probability into a risk score and alert level.
5. The result is stored in SQLite for auditability.
6. A JSON response is returned to the client.

The project is intentionally built to demonstrate:

- Machine Learning
- Backend Development
- REST API Design
- SQLite Database Design
- Object-Oriented Programming
- Design Patterns
- System Design
- Explainability and Alerting

---

## Why this project is strong for SDE internships

This is not just a notebook that prints an accuracy score. It is a small but complete system with clear separation of concerns:

- **ML pipeline** trains and exports the models.
- **Prediction service** performs inference on new transactions.
- **C++ backend** exposes REST endpoints and orchestrates the workflow.
- **SQLite** stores transactions, predictions, and alerts.
- **Explainability** and **risk scoring** make the system more production-like.

That gives interviewers multiple angles to ask questions about:

- how the model works,
- how the backend works,
- how the database is designed,
- why certain metrics were chosen,
- and how the system would scale.

---

## Tech Stack

### Machine Learning
- Python
- pandas
- NumPy
- scikit-learn
- XGBoost
- joblib

### Backend
- C++17
- POSIX sockets
- `std::thread`
- SQLite3

### Data & Storage
- Kaggle Credit Card Fraud Detection dataset
- SQLite database
- JSON-based model metadata and preprocessing statistics

---

## Dataset

The project uses the Kaggle **Credit Card Fraud Detection** dataset.

### Dataset structure
- `Time`
- `V1` to `V28`
- `Amount`
- `Class`

### Target label
- `Class = 0` → legitimate transaction
- `Class = 1` → fraud transaction

### Why this dataset is used
The dataset is a standard benchmark for fraud detection because it is:
- highly imbalanced,
- realistic,
- small enough to train quickly,
- and widely recognized in interviews.

---

## Final model performance

The ensemble achieved the following metrics:

- **Accuracy:** 0.9993328885923949
- **Precision:** 0.7777777777777778
- **Recall:** 0.8571428571428571
- **F1 Score:** 0.8155339805825242
- **ROC-AUC:** 0.9729731087708016

### How to interpret these metrics

Accuracy is very high, but in fraud detection it is not the main metric because the classes are heavily imbalanced. The more important numbers are:

- **Recall**: how many fraud transactions were caught
- **Precision**: how many flagged transactions were truly fraud
- **ROC-AUC**: how well the model separates fraud from legitimate transactions

For this reason, the project is designed to optimize fraud detection quality rather than raw accuracy.

---

## Repository structure

```text
Real_Time_Fraud_Detection_Platform/
├── cpp/
│   ├── main.cpp
│   ├── http_server.h
│   ├── http_server.cpp
│   ├── fraud_detection_service.h
│   ├── fraud_detection_service.cpp
│   ├── prediction_service.h
│   ├── prediction_service.cpp
│   ├── alert_service.h
│   ├── alert_service.cpp
│   ├── database.h
│   ├── database.cpp
│   ├── transaction.h
│   └── json_utils.h
├── scripts/
│   ├── train_model.py
│   └── predict.py
├── models/
│   ├── logistic_regression.pkl
│   ├── random_forest.pkl
│   ├── xgboost.pkl
│   ├── scaler.pkl
│   ├── metrics.json
│   ├── model_metadata.json
│   └── preprocess_stats.json
├── sql/
│   └── schema.sql
├── data/
│   └── README.md
├── sample_requests.http
├── requirements.txt
├── Makefile
└── README.md
```

---

## High-level architecture

```text
Client
  |
  v
REST API Layer (C++)
  |
  v
FraudDetectionService
  |
  +--------------------+
  |                    |
  v                    v
PredictionService      SQLite Database
  |                    |
  v                    v
Python inference       transactions / predictions / alerts
  |
  v
Trained models (.pkl)
```

---

## End-to-end workflow

### 1. Transaction request arrives
A client sends a `POST /transactions` request containing:
- `transaction_id`
- `Time`
- `V1` to `V28`
- `Amount`
- optional metadata like `merchant` and `timestamp`

### 2. Request is parsed and validated
The C++ service extracts the transaction fields and builds a transaction object.

### 3. Inference layer is called
The C++ backend writes the transaction to a temporary JSON file and invokes:

- `scripts/predict.py`

This script loads:
- `logistic_regression.pkl`
- `random_forest.pkl`
- `xgboost.pkl`
- `preprocess_stats.json`

### 4. Models produce probabilities
Each model returns a fraud probability.

### 5. Weighted ensemble is computed
The final probability is computed using weighted voting:

- Logistic Regression → 0.2
- Random Forest → 0.3
- XGBoost → 0.5

### 6. Risk score is generated
The final fraud probability is converted to a score out of 100.

### 7. Alert level is assigned
Risk score thresholds are used:

- `LOW`  → score < 50
- `MEDIUM` → 50 to 79.99
- `HIGH` → 80+

### 8. Explainability is generated
Because the Kaggle features are PCA-transformed and anonymized, the project provides **operational explainability** rather than human-semantic reasons. The system reports:
- model consensus,
- top weighted feature contributions,
- and transaction-level risk factors.

### 9. Results are stored in SQLite
The backend stores:
- the original transaction,
- the prediction output,
- and alerts when applicable.

### 10. JSON response is returned
The API returns the prediction summary to the client.

---

## ML pipeline details

### Preprocessing
The dataset is preprocessed by standardizing:
- `Time`
- `Amount`

The remaining PCA-transformed features (`V1` to `V28`) are used as-is.

### Models used
The ensemble uses three models:

#### 1. Logistic Regression
Useful for fast linear signal detection and baseline interpretability.

#### 2. Random Forest
Useful for non-linear interactions and robustness.

#### 3. XGBoost
Useful for strong tabular performance and class imbalance handling.

### Why an ensemble
Fraud detection is noisy and imbalanced. Different models make different mistakes. Combining them usually gives a more robust predictor than a single model.

---

## Why `predict.py` is part of the architecture

The trained models are Python pickle artifacts. Instead of rewriting inference logic in C++, the project keeps the ML inference in Python and lets the C++ backend orchestrate the request flow.

This is a common and practical design in hybrid systems:

- C++: service control, APIs, persistence
- Python: model inference and ML tooling

That keeps the backend simple while still preserving the trained model artifacts you created.

---

## SQLite schema design

The database contains three tables.

### `transactions`
Stores the original request payload for auditability.

### `predictions`
Stores the fraud probability, risk score, alert level, and the full prediction JSON.

### `alerts`
Stores alerts generated for medium/high-risk transactions.

### Why this design
This separation keeps the system normalized and makes it easy to:
- inspect transaction history,
- review model output,
- and query alert records independently.

---

## REST API design

### `GET /health`
Returns a basic service health response.

### `POST /transactions`
Accepts a transaction payload and returns:
- fraud probability
- risk score
- alert level
- model consensus
- explainability information

### `GET /transactions/{id}`
Returns the stored request and prediction associated with a transaction id.

### `GET /alerts`
Returns the list of generated alerts.

---

## OOP design

The backend is split into focused classes.

### `TransactionInput`
Represents a single incoming transaction.

### `PredictionSummary`
Represents the model inference result.

### `AlertRecord`
Represents a generated alert.

### `Database`
Handles SQLite connection management and SQL execution.

### `PredictionService`
Loads model files and invokes the Python inference script.

### `AlertService`
Converts risk scores into alert severity and alert text.

### `FraudDetectionService`
Coordinates the full workflow:
- parse request
- call model
- store results
- return response

### `HttpServer`
Implements the REST-style HTTP interface and dispatches requests to the service layer.

---

## Design principles used

### Encapsulation
Each class keeps its own state and exposes only the methods needed by other components.

### Abstraction
The rest of the system does not need to know how SQLite queries or model inference are implemented internally.

### Separation of concerns
- API handling
- prediction
- alerts
- database logic
- JSON parsing

are all separated into different files.

### Single Responsibility Principle
Each class does one main job.

### Open/Closed Principle
New models or alert thresholds can be added without redesigning the whole backend.

### Composition
The service classes are composed together rather than merged into one monolithic file.

---

## Why the explainability is operational instead of semantic

The Kaggle dataset uses PCA-transformed features `V1` to `V28`. That means the features are not naturally human-readable as:
- salary,
- age,
- merchant category,
- or location.

Because of that, the system does not claim to explain fraud using semantic reasons like “high travel spend” or “suspicious merchant type.” Instead it provides:

- weighted model contributions,
- high-impact feature identifiers,
- and confidence-based reasoning.

That is the correct and honest way to explain a model trained on anonymized features.

---

## How to run

### 1. Install Python dependencies
```bash
pip install -r requirements.txt
```

### 2. Build the C++ backend
```bash
make
```

### 3. Start the server
```bash
./fraudshield
```

You can optionally specify a different port:
```bash
./fraudshield 8080
```

### 4. Call the APIs
Use `sample_requests.http` or any REST client to test the endpoints.

---

## Interview talking points

If asked to explain the project, a strong answer is:

> A transaction arrives at the C++ REST layer, gets parsed into a transaction object, then is forwarded to a Python inference service that loads the trained Logistic Regression, Random Forest, and XGBoost models. Their outputs are combined using a weighted ensemble to generate fraud probability and a risk score. The backend then assigns an alert level, generates operational explainability, stores the transaction and prediction in SQLite, and returns the JSON response to the client.

If asked why this project is good:
- it combines ML and backend,
- it uses a real public dataset,
- it stores results in a database,
- and it demonstrates system design, OOP, and model deployment concepts.

---

## Future improvements

- Move from shell-based Python inference to a direct model server
- Add JWT authentication
- Add rate limiting
- Add Redis caching
- Add asynchronous queue-based alerting
- Add a dashboard
- Add Docker support
- Add deployment to cloud

---

## Final note

This project is intentionally structured to be explainable in an SDE interview. It is not just a machine learning notebook, and it is not just a backend skeleton. It is a full pipeline:

- dataset
- preprocessing
- model training
- model export
- inference
- risk scoring
- alert generation
- SQLite persistence
- REST endpoints
- OOP-based backend architecture
```
