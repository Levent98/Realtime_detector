package com.example.sensorgui.CommunicationLayer;

import org.eclipse.paho.client.mqttv3.*;

import java.nio.charset.StandardCharsets;

public class MQTTComLayer {

    private final String brokerUrl;
    private final String clientId;
    private MqttClient client;

    public MQTTComLayer(String brokerUrl, String clientId) {
        this.brokerUrl = brokerUrl;
        this.clientId = clientId;
    }

    public void connect() throws Exception {
        client = new MqttClient(brokerUrl, clientId);
        MqttConnectOptions options = new MqttConnectOptions();
        options.setAutomaticReconnect(true);
        options.setCleanSession(true);
        client.connect(options);
    }

    public void publish(String topic, String payload) throws Exception {
        if (client == null || !client.isConnected()) {
            throw new IllegalStateException("MQTT not connected");
        }

        MqttMessage message = new MqttMessage(payload.getBytes(StandardCharsets.UTF_8));
        message.setQos(1);
        client.publish(topic, message);
    }

    public void subscribe(String topic, IMqttMessageListener listener) throws Exception {
        if (client == null || !client.isConnected()) {
            throw new IllegalStateException("MQTT not connected");
        }
        client.subscribe(topic, listener);
    }

    public boolean isConnected() {
        return client != null && client.isConnected();
    }

    public void disconnect() throws Exception {
        if (client != null && client.isConnected()) {
            client.disconnect();
        }
    }
}
