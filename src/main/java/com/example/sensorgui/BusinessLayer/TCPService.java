package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.CommunicationLayer.TCPComLayer;
import com.example.sensorgui.ModelLayer.SensorData;

import java.util.Optional;

public class TCPService implements SensorService {

    private final TCPComLayer tcp;

    public TCPService(String host, int port) {
        this.tcp = new TCPComLayer(host, port);
    }

    @Override
    public void connect() throws Exception {
        tcp.connect();
    }

    @Override
    public Optional<SensorData> getSensorData() {
        return tcp.readData();
    }

    @Override
    public void sendCommand(String command) {
        tcp.sendCommand(command);
    }

    @Override
    public void disconnect() {
        tcp.disconnect();
    }
}
