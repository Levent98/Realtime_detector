package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.ModelLayer.SensorData;

import java.io.IOException;
import java.util.Optional;

public interface SensorService {
    void connect() throws Exception;
    Optional<SensorData> getSensorData() throws Exception;
    void sendCommand(String command) throws Exception;
    void disconnect() throws Exception;
}
