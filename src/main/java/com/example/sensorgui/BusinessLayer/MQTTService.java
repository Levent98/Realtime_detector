package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.CommunicationLayer.MQTTComLayer;
import com.example.sensorgui.ModelLayer.SensorData;
import org.eclipse.paho.client.mqttv3.IMqttMessageListener;

import java.util.Optional;

public class MQTTService implements SensorService, MqttCapableService {

    private final MQTTComLayer mqtt;

    public MQTTService(String brokerUrl, String clientId) {
        this.mqtt = new MQTTComLayer(brokerUrl, clientId);
    }

    @Override
    public void connect() throws Exception {
        mqtt.connect();
    }

    @Override
    public Optional<SensorData> getSensorData() {
        return Optional.empty();
    }

    @Override
    public void sendCommand(String command) {
        throw new UnsupportedOperationException("Use publish(topic, payload) for MQTT.");
    }

    @Override
    public void disconnect() throws Exception {
        mqtt.disconnect();
    }

    @Override
    public void publish(String topic, String payload) throws Exception {
        mqtt.publish(topic, payload);
    }

    @Override
    public void subscribe(String topic) throws Exception {
        mqtt.subscribe(topic, (receivedTopic, message) -> System.out.println("[MQTT] " + receivedTopic + " -> " + new String(message.getPayload())));
    }

    public void subscribe(String topic, IMqttMessageListener listener) throws Exception {
        mqtt.subscribe(topic, listener);
    }
}
