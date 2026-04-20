package com.example.sensorgui.BusinessLayer;

public interface MqttCapableService {
    void publish(String topic, String payload) throws Exception;
    void subscribe(String topic) throws Exception;
}
