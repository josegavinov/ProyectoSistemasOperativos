CC = gcc
CFLAGS = -Wall -pthread -O2
BUILD = build

all: $(BUILD)/broker $(BUILD)/gateway $(BUILD)/publisher $(BUILD)/subscriber

$(BUILD)/broker: src/broker.c
	$(CC) $(CFLAGS) -o $(BUILD)/broker src/broker.c

$(BUILD)/gateway: src/gateway.c
	$(CC) $(CFLAGS) -o $(BUILD)/gateway src/gateway.c

$(BUILD)/publisher: src/publisher.c
	$(CC) $(CFLAGS) -o $(BUILD)/publisher src/publisher.c

$(BUILD)/subscriber: src/subscriber.c
	$(CC) $(CFLAGS) -o $(BUILD)/subscriber src/subscriber.c

clean:
	rm -f $(BUILD)/*
