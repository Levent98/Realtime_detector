package com.example.sensorgui.CommunicationLayer;

import com.example.sensorgui.ModelLayer.SensorData;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.util.Optional;

/**
 * Line-based plain text TCP client.
 * This class is not Modbus TCP.
 */
public class TCPComLayer implements SensorCommunication {

    private final String host;
    private final int port;

    private Socket socket;
    private BufferedReader reader;
    private BufferedWriter writer;

    private static final int CONNECT_TIMEOUT_MS = 3000;
    private static final int READ_TIMEOUT_MS = 5000;

    public TCPComLayer(String host, int port) {
        this.host = host;
        this.port = port;
    }

    @Override
    public void connect() throws IOException {
        socket = new Socket();
        socket.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT_MS);
        socket.setSoTimeout(READ_TIMEOUT_MS);

        reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
        writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8));
    }

    @Override
    public Optional<SensorData> readData() {
        if (reader == null) {
            throw new IllegalStateException("Connection not opened. Call connect() first.");
        }

        try {
            String line = reader.readLine();
            if (line == null) {
                return Optional.empty();
            }

            double value = Double.parseDouble(line.trim());
            SensorData data = new SensorData("tcpSensor1", value, "°C", LocalDateTime.now());
            data.setSourceProtocol("TCP_TEXT");
            return Optional.of(data);
        } catch (IOException | NumberFormatException e) {
            return Optional.empty();
        }
    }

    @Override
    public void sendCommand(String command) {
        if (writer == null) {
            throw new IllegalStateException("Connection not opened. Call connect() first.");
        }

        try {
            writer.write(command);
            writer.newLine();
            writer.flush();
        } catch (IOException e) {
            throw new RuntimeException("Failed to send command", e);
        }
    }

    @Override
    public void disconnect() {
        try {
            if (reader != null) reader.close();
        } catch (IOException ignored) {
        }

        try {
            if (writer != null) writer.close();
        } catch (IOException ignored) {
        }

        try {
            if (socket != null && !socket.isClosed()) socket.close();
        } catch (IOException ignored) {
        }

        socket = null;
        reader = null;
        writer = null;
    }
}
