CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
LDFLAGS = -lsqlite3 -pthread

CPP_DIR = cpp
BIN = fraudshield

SRC = \
	$(CPP_DIR)/main.cpp \
	$(CPP_DIR)/http_server.cpp \
	$(CPP_DIR)/fraud_detection_service.cpp \
	$(CPP_DIR)/prediction_service.cpp \
	$(CPP_DIR)/alert_service.cpp \
	$(CPP_DIR)/database.cpp

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -I$(CPP_DIR) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN) fraudshield.db
