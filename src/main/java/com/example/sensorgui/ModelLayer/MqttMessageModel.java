package com.example.sensorgui.ModelLayer;

import java.time.LocalDateTime;

public class MqttMessageModel {
    private final String topic;
    private final String payload;
    private final LocalDateTime timestamp;

    public MqttMessageModel(String topic, String payload) {
        this.topic = topic;
        this.payload = payload;
        this.timestamp = LocalDateTime.now();
    }

    public String getTopic() {
        return topic;
    }

    public String getPayload() {
        return payload;
    }

    public LocalDateTime getTimestamp() {
        return timestamp;
    }
}
