package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.CommunicationLayer.SerialComLayer;
import com.example.sensorgui.ModelLayer.SensorData;

import java.util.Optional;

public class SerialService implements SensorService {

    private final SerialComLayer serial;

    public SerialService(String portName, int baudRate) {
        this.serial = new SerialComLayer(portName, baudRate);
    }

    @Override
    public void connect() throws Exception {
        serial.connect();
    }

    @Override
    public Optional<SensorData> getSensorData() throws Exception {
        return serial.readData();
    }

    @Override
    public void sendCommand(String command) throws Exception {
        serial.sendCommand(command);
    }

    @Override
    public void disconnect() throws Exception {
        serial.disconnect();
    }
}
