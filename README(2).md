# Real-Time Fraud Detection and Risk Scoring Platform

A hybrid **C++ + Python** system that accepts transaction data through REST APIs, runs fraud scoring using trained machine learning models, stores the full result in **SQLite**, and returns a risk-based decision to the client.

This project is designed to look and behave like a real fraud monitoring service:
- a client sends a transaction,
- the backend scores it in real time,
- the system assigns a risk level,
- and the result is stored for later review and auditing.

---

## 1. Project goal

The main goal of this project is to detect suspicious transactions as early as possible and convert the model output into an easy-to-understand **risk score**.

Instead of only saying **fraud** or **not fraud**, the platform also provides:
- a **fraud probability**,
- a **risk score out of 100**,
- an **alert level** (`LOW`, `MEDIUM`, `HIGH`),
- model-level probabilities from each ML model,
- and a small explainability output.

This makes the project more useful for real-world decision making and also more impressive in interviews because it combines:
- machine learning,
- REST API design,
- C++ backend engineering,
- database persistence,
- and system architecture.

---

## 2. What the system does

The platform exposes a small HTTP server written in **C++**. It accepts transaction JSON through a REST endpoint, forwards the request to a Python prediction script, stores everything in SQLite, and returns a structured JSON response.

The flow is:

1. A client sends transaction data to `POST /transactions`
2. The C++ backend parses the request
3. The backend writes the transaction to a temporary JSON file
4. A Python inference script loads the trained models
5. The script produces:
   - fraud probability
   - risk score
   - alert level
   - model consensus
   - explainability output
6. The C++ backend stores the result in SQLite
7. The API returns the final JSON response to the client

---

## 3. Core features

- Real-time transaction scoring through REST APIs
- Weighted ensemble of three machine learning models
- Risk score conversion from model probability
- Automatic alert generation based on thresholds
- SQLite persistence for transactions, predictions, and alerts
- GET APIs for querying transaction history and alerts
- Simple C++ backend with separate service classes
- Python-based training and inference pipeline
- Model metadata and preprocessing statistics stored as JSON
- Operational explainability for anonymized fraud features

---

## 4. Tech stack

### Machine Learning / Data Science
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

### Storage and metadata
- SQLite database
- JSON files for model metadata and preprocessing statistics
- Pickle/joblib model artifacts

---

## 5. Repository structure

```text
.
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
│   ├── creditcard.csv
│   └── README.md
├── sample_requests.http
├── requirements.txt
├── Makefile
└── README.md
```

---

## 6. Dataset used

The project uses the well-known **Kaggle Credit Card Fraud Detection** dataset.

### Dataset columns
- `Time`
- `V1` to `V28`
- `Amount`
- `Class`

### Target meaning
- `Class = 0` → legitimate transaction
- `Class = 1` → fraudulent transaction

### Dataset size
From the stored metadata:
- Total samples: **284,807**
- Fraud samples: **492**
- Legitimate samples: **284,315**

This dataset is highly imbalanced, which is exactly why fraud detection is a challenging problem and why accuracy alone is not enough.

---

## 7. End-to-end architecture

```text
Client
  |
  v
C++ REST API Server
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
Trained ML models
```

### Why this architecture is used
The design splits responsibilities cleanly:

- **C++** handles networking, request routing, and system orchestration
- **Python** handles machine learning inference
- **SQLite** handles persistence and audit logs

This makes the system simple, modular, and easy to explain.

---

## 8. Detailed workflow

### Step 1: Request comes in
The client sends a JSON transaction to:

`POST /transactions`

The payload can include:
- `transaction_id`
- `merchant`
- `timestamp`
- `Time`
- `V1` to `V28`
- `Amount`

### Step 2: C++ parses the payload
The backend uses helper functions in `json_utils.h` to extract:
- string fields
- numeric fields
- the 28 transaction features

These values are copied into a `TransactionInput` structure.

### Step 3: Temporary input file is created
The C++ backend writes the transaction to a temporary JSON file.

### Step 4: Python inference script is executed
The backend calls:

```bash
python3 scripts/predict.py --input <temp_input.json> --output <temp_output.json> --models models
```

### Step 5: Model scoring happens
The Python script:
- loads the trained models,
- standardizes `Time` and `Amount`,
- gets probabilities from each model,
- computes the weighted ensemble,
- derives the risk score and alert level,
- generates explainability data.

### Step 6: Result is parsed in C++
The backend reads the returned JSON and extracts:
- fraud probability
- risk score
- alert level

### Step 7: Alert generation
The alert service converts the risk score into a business-friendly alert level:
- `LOW`
- `MEDIUM`
- `HIGH`

### Step 8: Database persistence
The transaction, prediction, and alert are saved in SQLite in one transaction.

### Step 9: Response is returned
The backend sends a JSON response with the final scoring result.

---

## 9. Machine learning pipeline

The ML pipeline is intentionally simple and practical so that it is easy to understand in interviews.

### 9.1 Preprocessing

Only two features are standardized:
- `Time`
- `Amount`

The other features (`V1` to `V28`) are used as-is.

Why?
- In the original Kaggle dataset, `V1` to `V28` are already PCA-transformed features.
- Standardizing them again is not necessary in this pipeline.
- `Time` and `Amount` are the two raw numeric features that benefit from scaling.

### Stored preprocessing statistics
The project stores the dataset-level mean and standard deviation for:
- `Time`
- `Amount`

These are saved in `models/preprocess_stats.json` and used during inference.

### 9.2 Train/test split
Training uses:
- **80% training**
- **20% testing**
- `stratify=y`
- `random_state=42`

Stratification is important because the data is highly imbalanced.

### 9.3 Models used
The system trains three models:

#### Logistic Regression
- Fast baseline model
- Works well as a linear signal detector
- Trained with `class_weight="balanced"`

#### Random Forest
- Captures non-linear relationships
- More robust than a simple linear model
- Trained with `class_weight="balanced"`

#### XGBoost
- Strong performer on tabular data
- Handles imbalance well with `scale_pos_weight`
- Often gives the best predictive power in this ensemble

### 9.4 Ensemble strategy
The final prediction is a **weighted voting ensemble**:

- Logistic Regression → **0.2**
- Random Forest → **0.3**
- XGBoost → **0.5**

The final fraud probability is:

```text
0.2 × LR probability
+ 0.3 × RF probability
+ 0.5 × XGBoost probability
```

This gives more importance to XGBoost while still keeping the other models in the decision.

### 9.5 Risk score
The ensemble fraud probability is converted into a score out of 100:

```text
risk_score = ensemble_probability × 100
```

This makes the result easier to present in a dashboard or alerting system.

### 9.6 Alert levels
The risk score is mapped to alert severity:

- `LOW` → score below 50
- `MEDIUM` → score from 50 to 79.99
- `HIGH` → score 80 or above

---

## 10. Model performance

The stored evaluation metrics are:

- **Accuracy:** 0.9993328885923949
- **Precision:** 0.7777777777777778
- **Recall:** 0.8571428571428571
- **F1 Score:** 0.8155339805825242
- **ROC-AUC:** 0.9729731087708016

### How to interpret these numbers

#### Accuracy
Accuracy is extremely high, but in fraud detection this metric alone can be misleading because fraud cases are very rare.

#### Precision
Precision tells us how many flagged transactions were actually fraud.

#### Recall
Recall tells us how many fraud cases were successfully detected.

#### F1 Score
F1 balances precision and recall. It is useful when the dataset is imbalanced.

#### ROC-AUC
ROC-AUC measures how well the model separates fraud from legitimate transactions across thresholds.

### Why these metrics matter
For fraud detection, recall and ROC-AUC are especially important because missing a fraud case is usually worse than flagging one extra case for review.

---

## 11. Prediction output format

The Python inference script returns JSON in this structure:

- `transaction_id`
- `fraud_probability`
- `risk_score`
- `alert_level`
- `model_probabilities`
- `ensemble_weights`
- `consensus`
- `explanations`
- `interpretation`

### Example meaning of fields

#### `model_probabilities`
Shows the individual probability predicted by each model.

#### `consensus`
Shows whether each model individually leans toward:
- `FRAUD`
- `LEGITIMATE`

#### `explanations`
Returns the top feature contributions according to the ensemble’s operational explanation logic.

#### `interpretation`
Explains that the features are anonymized and PCA-transformed, so the output is model-centric rather than business-semantic.

---

## 12. Explainability approach

This project does not try to claim human-readable causes like:
> “This transaction was flagged because the merchant is suspicious”

That would not be accurate for this dataset.

Instead, the system gives **operational explainability** based on:
- the importance signals from Logistic Regression,
- feature importances from Random Forest,
- feature importances from XGBoost,
- and the magnitude of feature values.

Because the Kaggle features `V1` to `V28` are anonymized PCA components, the project keeps the explanation honest and technical:
- it explains **which features influenced the score most**
- but not **what those features mean in business terms**

This is a realistic and careful approach.

---

## 13. C++ backend design

The backend is intentionally split into small classes so that each class has one clear responsibility.

### 13.1 `main.cpp`
This is the entry point of the application.

It:
- sets the database path,
- sets the schema path,
- sets the model directory,
- sets the prediction script path,
- starts the HTTP server.

### 13.2 `HttpServer`
This class:
- opens the socket,
- listens on the configured port,
- accepts client connections,
- parses the HTTP request,
- routes the request to the correct service method,
- returns the HTTP response.

Supported endpoints:
- `GET /health`
- `POST /transactions`
- `GET /transactions/{id}`
- `GET /alerts`

The server uses:
- POSIX sockets
- `accept()`
- `recv()`
- `send()`
- one thread per client connection using `std::thread`

### 13.3 `FraudDetectionService`
This is the orchestration layer.

It coordinates:
- request parsing,
- prediction,
- alert generation,
- database persistence,
- and response assembly.

This class is the main business logic layer of the project.

### 13.4 `PredictionService`
This class is responsible for ML inference.

It:
- writes the input transaction to a temporary JSON file,
- runs the Python script using `std::system`,
- reads back the result JSON,
- and returns it to the fraud detection service.

This is how the C++ backend talks to the Python model layer.

### 13.5 `AlertService`
This class turns a numeric risk score into a business alert:
- score >= 80 → `HIGH`
- score >= 50 → `MEDIUM`
- otherwise → `LOW`

It also generates a short message for each alert level.

### 13.6 `Database`
This class wraps SQLite access.

It is responsible for:
- opening and closing the database connection,
- initializing the schema,
- saving transactions, predictions, and alerts,
- fetching a transaction by ID,
- fetching all alerts.

The class also uses a `std::mutex` to protect database operations.

### 13.7 `json_utils.h`
This header contains lightweight JSON helpers:
- simple string escaping
- basic field extraction from request JSON
- transaction serialization
- prediction summary parsing

It is a small utility layer used throughout the backend.

### 13.8 `transaction.h`
This file defines the main data structures:
- `TransactionInput`
- `PredictionSummary`
- `AlertRecord`

These structures make the code easier to read and maintain.

---

## 14. Why SQLite is used

SQLite is used as the persistent storage layer for three main reasons:

### 1. Simple local storage
It does not require a separate database server, so the project is easy to run on a laptop or in a demo environment.

### 2. Fast read/write for this use case
For a project demo or interview project, SQLite is enough to store:
- transactions
- predictions
- alerts

### 3. Easy audit trail
Fraud systems need traceability. SQLite makes it easy to keep a record of:
- what request came in,
- what the model predicted,
- and whether an alert was raised.

---

## 15. SQLite schema explanation

The schema is defined in `sql/schema.sql`.

### `transactions`
Stores the original request.

Columns:
- `transaction_id` — primary key
- `request_json` — full request payload
- `created_at` — timestamp

### `predictions`
Stores the model output.

Columns:
- `prediction_id` — auto-increment primary key
- `transaction_id` — unique foreign key
- `fraud_probability`
- `risk_score`
- `alert_level`
- `model_probabilities_json`
- `explanations_json`
- `prediction_json`
- `created_at`

### `alerts`
Stores generated alerts.

Columns:
- `alert_id` — auto-increment primary key
- `transaction_id` — foreign key
- `alert_level`
- `message`
- `created_at`

### Why three tables
Keeping them separate makes the design cleaner:

- `transactions` = input data
- `predictions` = ML output
- `alerts` = business action

This is a normalized and interview-friendly design.

---

## 16. REST API design

### `GET /health`
Checks whether the service is alive.

#### Example response
```json
{
  "status": "ok",
  "service": "Fraud Detection Platform"
}
```

---

### `POST /transactions`
Submits a transaction for scoring.

#### Example request
```json
{
  "transaction_id": "txn_1001",
  "merchant": "Amazon",
  "timestamp": "2026-06-25T10:30:00Z",
  "Time": 0,
  "V1": 1.02,
  "V2": -0.15,
  "V3": 0.32,
  "V4": -1.05,
  "V5": 0.44,
  "V6": 0.11,
  "V7": 0.28,
  "V8": -0.03,
  "V9": 0.17,
  "V10": -0.12,
  "V11": 0.08,
  "V12": -0.21,
  "V13": 0.05,
  "V14": -0.73,
  "V15": 0.04,
  "V16": -0.19,
  "V17": 0.09,
  "V18": 0.02,
  "V19": -0.07,
  "V20": 0.18,
  "V21": -0.03,
  "V22": 0.04,
  "V23": 0.01,
  "V24": -0.02,
  "V25": 0.06,
  "V26": -0.01,
  "V27": 0.03,
  "V28": 0.02,
  "Amount": 219.45
}
```

#### Example response
```json
{
  "status": "success",
  "persisted": true,
  "prediction": {
    "transaction_id": "txn_1001",
    "fraud_probability": 0.123456,
    "risk_score": 12.35,
    "alert_level": "LOW",
    "model_probabilities": {
      "logistic_regression": 0.112233,
      "random_forest": 0.140011,
      "xgboost": 0.128844
    },
    "consensus": {
      "logistic_regression": "LEGITIMATE",
      "random_forest": "LEGITIMATE",
      "xgboost": "LEGITIMATE"
    },
    "explanations": [
      {
        "feature": "V14",
        "contribution": 0.81234,
        "note": "High absolute contribution in the ensemble score"
      }
    ],
    "interpretation": "Operational explainability based on weighted model contributions and anonymized feature influence. Kaggle features V1-V28 are PCA-transformed, so the explanations are model-centric rather than human-semantic."
  }
}
```

---

### `GET /transactions/{id}`
Fetches a stored transaction and its prediction.

#### Example
`GET /transactions/txn_1001`

#### Typical response fields
- `transaction_id`
- `request_json`
- `prediction_json`
- `fraud_probability`
- `risk_score`
- `alert_level`

---

### `GET /alerts`
Returns all generated alerts.

#### Example response
```json
{
  "alerts": [
    {
      "transaction_id": "txn_1001",
      "alert_level": "HIGH",
      "message": "High-risk transaction flagged for immediate review.",
      "created_at": "2026-06-25 10:35:21"
    }
  ]
}
```

---

## 17. Training pipeline

The training script is `scripts/train_model.py`.

### What it does
1. Loads `creditcard.csv`
2. Checks that the target column `Class` exists
3. Standardizes `Time` and `Amount`
4. Splits the data into train and test sets
5. Trains three classifiers:
   - Logistic Regression
   - Random Forest
   - XGBoost
6. Computes the ensemble prediction
7. Calculates metrics
8. Saves models and metadata to the `models/` folder

### Output files
- `logistic_regression.pkl`
- `random_forest.pkl`
- `xgboost.pkl`
- `scaler.pkl`
- `metrics.json`
- `model_metadata.json`
- `preprocess_stats.json`

---

## 18. Inference pipeline

The inference script is `scripts/predict.py`.

### What it does
1. Reads the input JSON transaction
2. Builds the feature row in the correct order
3. Standardizes `Time` and `Amount`
4. Loads the three trained models
5. Gets the probability from each model
6. Computes the weighted ensemble probability
7. Converts it into a risk score
8. Assigns an alert level
9. Produces a small explanation report

### Important point
The inference script is the only place where the trained models are actually used for scoring. The C++ backend calls this script rather than reimplementing the ML logic in C++.

---

## 19. Why the ML inference is done in Python instead of C++

This is a practical design choice.

### Python is better for ML tasks because
- the models were trained in Python,
- joblib/pickle loading is easy,
- scikit-learn and XGBoost support is native,
- feature processing is quick to implement.

### C++ is better for backend orchestration because
- it is fast,
- it gives more control over sockets and memory,
- it is suitable for a custom server,
- and it shows strong systems programming skills in an interview.

This split keeps the project easy to maintain and easy to explain.

---

## 20. Object-oriented design principles used

The code uses OOP in a clean and interview-friendly way.

### Encapsulation
Each class keeps its own data and behavior:
- `HttpServer` handles networking
- `Database` handles persistence
- `PredictionService` handles ML inference
- `AlertService` handles alert rules
- `FraudDetectionService` orchestrates the flow

### Single Responsibility Principle
Each class does one job only.

### Separation of concerns
Backend logic, prediction logic, and storage logic are kept separate.

### Modularity
The system is split into small files, so every piece can be understood independently.

### Reusability
Common helpers like JSON parsing and file utilities are placed in shared headers.

---

## 21. Build and run

### Prerequisites
You need:
- `g++` with C++17 support
- SQLite3 development libraries
- Python 3
- the Python packages in `requirements.txt`

### Python dependencies
```bash
pip install -r requirements.txt
```

### Build the backend
```bash
make
```

This produces the executable:

```bash
./fraudshield
```

### Run the server
By default, the server starts on port `8080`.

You can also provide a custom port:

```bash
./fraudshield 9000
```

### Re-train the models
```bash
python3 scripts/train_model.py --csv data/creditcard.csv --output models
```

### Test the APIs
The file `sample_requests.http` contains ready-to-use example requests.

---

## 22. Files that matter most in interviews

If someone wants to understand the project quickly, the most important files are:

- `cpp/main.cpp` → starts the application
- `cpp/http_server.cpp` → HTTP handling and routing
- `cpp/fraud_detection_service.cpp` → main workflow
- `cpp/prediction_service.cpp` → Python inference bridge
- `cpp/database.cpp` → SQLite persistence
- `scripts/train_model.py` → model training
- `scripts/predict.py` → prediction logic
- `sql/schema.sql` → database design
- `models/metrics.json` → final evaluation metrics

---

## 23. Limitations

The project is intentionally kept compact, so there are a few limitations:

- It uses a custom lightweight HTTP server instead of a full web framework
- It uses temporary files to communicate with the Python inference script
- The explainability is operational because the dataset features are anonymized
- SQLite is ideal for a demo or small deployment, but not for large-scale distributed workloads

These are acceptable trade-offs for a project meant to demonstrate full-stack engineering and ML integration.

---

## 24. Possible future improvements

- Replace the custom socket server with a production-grade C++ web framework
- Add authentication and role-based access control
- Expose pagination and filtering for alerts
- Add a proper async job queue for prediction requests
- Store richer explainability metadata in SQLite
- Add dashboards for transaction monitoring
- Add automated tests for API and model pipeline
- Replace temporary JSON files with direct IPC or an embedded model service

---

## 25. Summary

This project is a complete fraud detection pipeline that combines:
- **machine learning**
- **C++ backend engineering**
- **REST API design**
- **SQLite persistence**
- **risk scoring**
- **alert generation**
- **model explainability**

It shows how a transaction can move from a raw API payload to a final fraud score and a stored audit trail. The system is deliberately designed to be simple enough to understand, but complete enough to look like a real-world fraud monitoring platform.

