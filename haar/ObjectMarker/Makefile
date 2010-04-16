BIN_DIR=Release
EXEC=${BIN_DIR}/ObjectMarker
SRC_DIR=ObjectMarker
CODE=ObjectMarker.cpp

${EXEC}: ${SRC_DIR}/${CODE}
	mkdir -p ${BIN_DIR}
	g++ ${SRC_DIR}/${CODE} `pkg-config --cflags --libs opencv` -o ${EXEC}
