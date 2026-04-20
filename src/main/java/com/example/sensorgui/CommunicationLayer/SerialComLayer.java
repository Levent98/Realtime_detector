package com.example.sensorgui.CommunicationLayer;

import com.example.sensorgui.ModelLayer.SensorData;
import com.fazecast.jSerialComm.SerialPort;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.util.Optional;

public class SerialComLayer implements SensorCommunication {

    private final String portName;
    private final int baudrate;
    private SerialPort serialPort;

    public SerialComLayer(String portName, int baud) {
        this.portName = portName;
        this.baudrate = baud;
    }

    @Override
    public void connect() throws IOException {
        serialPort = SerialPort.getCommPort(portName);
        serialPort.setBaudRate(baudrate);
        serialPort.setNumDataBits(8);
        serialPort.setNumStopBits(SerialPort.ONE_STOP_BIT);
        serialPort.setParity(SerialPort.NO_PARITY);
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, 1000, 100);

        if (!serialPort.openPort()) {
            throw new IOException("Failed to open serial port " + portName);
        }
    }

    @Override
    public Optional<SensorData> readData() {
        if (serialPort == null || !serialPort.isOpen()) {
            return Optional.empty();
        }

        int available = serialPort.bytesAvailable();
        if (available <= 0) {
            return Optional.empty();
        }

        byte[] buffer = new byte[available];
        int read = serialPort.readBytes(buffer, buffer.length);
        if (read <= 0) {
            return Optional.empty();
        }

        String dataStr = new String(buffer, StandardCharsets.UTF_8).trim();
        if (dataStr.isEmpty()) {
            return Optional.empty();
        }

        try {
            double value = Double.parseDouble(dataStr);
            SensorData data = new SensorData("serialSensor1", value, "°C", LocalDateTime.now());
            data.setSourceProtocol("SERIAL");
            return Optional.of(data);
        } catch (NumberFormatException e) {
            return Optional.empty();
        }
    }

    @Override
    public void sendCommand(String command) {
        if (serialPort != null && serialPort.isOpen()) {
            byte[] bytes = command.getBytes(StandardCharsets.UTF_8);
            serialPort.writeBytes(bytes, bytes.length);
        }
    }

    @Override
    public void disconnect() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }
}
