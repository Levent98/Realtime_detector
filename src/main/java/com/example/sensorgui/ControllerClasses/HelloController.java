package com.example.sensorgui.ControllerClasses;

import com.example.sensorgui.BusinessLayer.*;
import com.example.sensorgui.ModelLayer.SensorData;
import com.example.sensorgui.Utils.NetworkUtils;
import com.fazecast.jSerialComm.SerialPort;
import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.*;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.URL;
import java.time.LocalDateTime;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HelloController implements Initializable {

    @FXML private Label welcomeText;
    @FXML public Button ConnectButton;
    @FXML public Button RefreshButton;
    @FXML public Button PublishButton;
    @FXML public Button ScanButton;
    @FXML public Button StopButton;
    @FXML private TextArea LogText;
    @FXML private ComboBox<String> DeviceSelect;
    @FXML private ProgressBar progressBar;
    @FXML public Button DisconnectButton;

    @FXML private ComboBox<Integer> modbusSlaveSelect;
    @FXML private ComboBox<String> modbusFunctionSelect;
    @FXML private TextField modbusRegisterInput;
    @FXML private TextField modbusCountInput;
    @FXML private TextArea modbusResponseArea;
    @FXML private RadioButton modbusRtuRadio;
    @FXML private RadioButton modbusTcpRadio;
    @FXML private TextField modbusHostInput;
    @FXML private TextField modbusPortInput;

    @FXML private TextField mqttBrokerInput;
    @FXML private TextField mqttClientIdInput;
    @FXML private TextField mqttTopicInput;
    @FXML private TextArea mqttPayloadInput;
    @FXML private TextArea mqttResponseArea;

    private final int SENSOR_PORT = 9001;
    private final int TIMEOUT_MS = 250;
    private final int MAX_THREADS = 4;
    private ExecutorService scanPool;
    private volatile boolean scanning = false;
    private volatile boolean readingData = false;

    private SensorService currentService;
    private ModbusCapableService currentModbusService;
    private MqttCapableService currentMqttService;

    @Override
    public void initialize(URL location, ResourceBundle resources) {
        LogText.setEditable(false);
        loadSerialPorts();

        if (progressBar != null) progressBar.setProgress(0);
        if (StopButton != null) StopButton.setDisable(true);

        modbusSlaveSelect.getItems().addAll(1,2,3,4,5,6,7,8,9,10);
        modbusSlaveSelect.getSelectionModel().selectFirst();

        modbusFunctionSelect.getItems().addAll(
                "01 - Read Coils",
                "02 - Read Discrete Inputs",
                "03 - Read Holding Register",
                "04 - Read Input Register"
        );
        modbusFunctionSelect.getSelectionModel().selectFirst();

        modbusRegisterInput.setText("6");
        modbusCountInput.setText("1");
        modbusPortInput.setText("502");
        mqttBrokerInput.setText("tcp://localhost:1883");
        mqttClientIdInput.setText("sensor-gui-client");
        mqttTopicInput.setText("prosense/device1/data");
        mqttPayloadInput.setText("{\"temperature\":25.3,\"humidity\":48.1,\"timestamp\":\"" + LocalDateTime.now() + "\"}");
    }

    @FXML
    private void DisconnectButtonClicked() {
        stopReading();

        try {
            if (currentService != null) {
                currentService.disconnect();
                append("✅ Current service disconnected.");
            }
        } catch (Exception e) {
            append("❌ Disconnect error: " + e.getMessage());
        } finally {
            currentService = null;
            currentModbusService = null;
            currentMqttService = null;
        }
    }

    @FXML
    private void ModbusConnectAction() {
        try {
            DisconnectButtonClicked();

            if (modbusTcpRadio != null && modbusTcpRadio.isSelected()) {
                String host = modbusHostInput.getText().trim();
                int port = Integer.parseInt(modbusPortInput.getText().trim());
                ModbusTCPService service = new ModbusTCPService(host, port);
                service.connect();
                currentService = service;
                currentModbusService = service;
                append("✅ Modbus TCP connected to " + host + ":" + port);
                return;
            }

            String selected = DeviceSelect.getSelectionModel().getSelectedItem();
            if (selected == null || !selected.startsWith("SERIAL:")) {
                append("⚠ Select a serial port for Modbus RTU or choose Modbus TCP.");
                return;
            }

            String portName = selected.replace("SERIAL: ", "").trim();
            ModbusRTUService service = SensorServiceFactory.createModbusRtuService(portName, 115200);
            service.connect();
            currentService = service;
            currentModbusService = service;
            append("✅ Modbus RTU connected on " + portName + " @ 115200 baud");

        } catch (Exception e) {
            append("❌ Modbus connect error: " + e.getMessage());
        }
    }

    @FXML
    private void ModbusReadAction() {
        if (currentModbusService == null) {
            modbusResponseArea.setText("No Modbus connection active.");
            return;
        }

        try {
            int slave = modbusSlaveSelect.getValue();
            int reg = Integer.parseInt(modbusRegisterInput.getText());
            int count = Integer.parseInt(modbusCountInput.getText());
            int functionCode = resolveFunctionCode(modbusFunctionSelect.getValue());

            byte[] response = currentModbusService.readRegisters(slave, functionCode, reg, count);
            String hex = bytesToHex(response);

            modbusResponseArea.setText(hex);
            append("✅ Modbus response: " + hex);

        } catch (Exception e) {
            modbusResponseArea.setText("Error: " + e.getMessage());
            append("❌ Modbus read error: " + e.getMessage());
        }
    }

    @FXML
    private void MqttConnectAction() {
        try {
            String broker = mqttBrokerInput.getText().trim();
            String clientId = mqttClientIdInput.getText().trim();
            MQTTService mqttService = SensorServiceFactory.createMqttService(broker, clientId);
            mqttService.connect();
            currentService = mqttService;
            currentMqttService = mqttService;
            append("✅ MQTT connected to " + broker);
        } catch (Exception e) {
            append("❌ MQTT connect error: " + e.getMessage());
        }
    }

    @FXML
    private void MqttPublishAction() {
        try {
            if (currentMqttService == null) {
                append("⚠ MQTT not connected.");
                return;
            }

            String topic = mqttTopicInput.getText().trim();
            String payload = mqttPayloadInput.getText();
            currentMqttService.publish(topic, payload);
            append("✅ MQTT publish -> " + topic);
        } catch (Exception e) {
            append("❌ MQTT publish error: " + e.getMessage());
        }
    }

    @FXML
    private void MqttSubscribeAction() {
        try {
            if (!(currentService instanceof MQTTService mqttService)) {
                append("⚠ MQTT service not connected.");
                return;
            }

            String topic = mqttTopicInput.getText().trim();
            mqttService.subscribe(topic, (receivedTopic, message) -> {
                String payload = new String(message.getPayload());
                Platform.runLater(() -> {
                    mqttResponseArea.appendText("[" + receivedTopic + "] " + payload + "\n");
                    append("📩 MQTT message received from " + receivedTopic);
                });
            });

            append("✅ Subscribed to " + topic);
        } catch (Exception e) {
            append("❌ MQTT subscribe error: " + e.getMessage());
        }
    }

    @FXML
    public void Scanbttnonaction() {
        startScan();
    }

    @FXML
    public void Stopbttnonaction() {
        stopScan();
    }

    private void loadSerialPorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        DeviceSelect.getItems().clear();
        DeviceSelect.getItems().add("None");

        for (SerialPort port : ports) {
            DeviceSelect.getItems().add("SERIAL: " + port.getSystemPortName());
        }

        DeviceSelect.getSelectionModel().selectFirst();
    }

    public void append(String s) {
        Platform.runLater(() -> LogText.appendText(s + "\n"));
    }

    private void startScan() {
        DeviceSelect.getItems().removeIf(item -> item.startsWith("TCP:"));
        append("Starting safe TCP scan...");

        Platform.runLater(() -> {
            DeviceSelect.getItems().add("TCP: 127.0.0.1:" + SENSOR_PORT);
            append("Added localhost for testing");
        });

        String localIp = NetworkUtils.getLocalIp();
        if (localIp == null) {
            append("Could not detect local IP.");
            return;
        }

        String subnet = localIp.substring(0, localIp.lastIndexOf('.') + 1);
        append("Using subnet: " + subnet);

        scanning = true;
        if (ScanButton != null) ScanButton.setDisable(true);
        if (StopButton != null) StopButton.setDisable(false);

        final int total = 254;
        final int scanDelayMs = 40;
        scanPool = Executors.newFixedThreadPool(MAX_THREADS);

        for (int i = 1; i <= 254; i++) {
            final int host = i;
            scanPool.submit(() -> {
                if (!scanning) return;

                String ip = subnet + host;
                try {
                    Thread.sleep(scanDelayMs);
                    if (checkPort(ip, SENSOR_PORT) && validateSensor(ip)) {
                        Platform.runLater(() -> {
                            DeviceSelect.getItems().add("TCP: " + ip + ":" + SENSOR_PORT);
                            append("Validated sensor: " + ip);
                        });
                    }
                } catch (Exception ignored) {
                }

                Platform.runLater(() -> {
                    if (progressBar != null) {
                        progressBar.setProgress((double) host / total);
                    }
                });
            });
        }

        scanPool.shutdown();
    }

    private void stopScan() {
        scanning = false;
        if (StopButton != null) StopButton.setDisable(true);
        if (ScanButton != null) ScanButton.setDisable(false);
        if (progressBar != null) progressBar.setProgress(0);

        if (scanPool != null && !scanPool.isShutdown()) {
            scanPool.shutdownNow();
        }

        append("Scan stopped.");
    }

    private boolean checkPort(String ip, int port) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(ip, port), TIMEOUT_MS);
            return true;
        } catch (IOException ignored) {
            return false;
        }
    }

    private boolean validateSensor(String ip) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(ip, SENSOR_PORT), TIMEOUT_MS);
            socket.getOutputStream().write("HELLO\n".getBytes());
            socket.getOutputStream().flush();

            byte[] buffer = new byte[64];
            int read = socket.getInputStream().read(buffer);
            if (read > 0) {
                String response = new String(buffer, 0, read).trim();
                return response.equals("SENSOR_OK");
            }
        } catch (IOException ignored) {
        }
        return false;
    }

    private void connectSelected() {
        String selected = DeviceSelect.getSelectionModel().getSelectedItem();
        if (selected == null || selected.equals("None")) {
            append("No device selected!");
            return;
        }

        try {
            stopReading();
            currentService = parseAndCreateService(selected);
            currentService.connect();
            append("✅ Connected to: " + selected);
            readingData = true;
            startContinuousReading();
        } catch (Exception e) {
            append("❌ Connection error: " + e.getMessage());
        }
    }

    private void startContinuousReading() {
        Thread readingThread = new Thread(() -> {
            int count = 0;
            while (readingData && currentService != null) {
                try {
                    Optional<SensorData> data = currentService.getSensorData();
                    if (data.isPresent()) {
                        count++;
                        int currentNumber = count;
                        SensorData currentData = data.get();
                        Platform.runLater(() -> append("📊 Data " + currentNumber + ": " + currentData));
                    }
                    Thread.sleep(1000);
                } catch (Exception e) {
                    Platform.runLater(() -> append("❌ Read error: " + e.getMessage()));
                    break;
                }
            }
        });

        readingThread.setDaemon(true);
        readingThread.start();
    }

    private void stopReading() {
        readingData = false;
    }

    private SensorService parseAndCreateService(String line) {
        if (line.startsWith("TCP:")) {
            String[] parts = line.replace("TCP:", "").trim().split(":");
            String ip = parts[0];
            int port = Integer.parseInt(parts[1]);
            return SensorServiceFactory.createService(SensorServiceFactory.ServiceType.TCP, ip, port);
        } else if (line.startsWith("SERIAL:")) {
            String portName = line.replace("SERIAL: ", "").trim();
            return SensorServiceFactory.createService(SensorServiceFactory.ServiceType.SERIAL, portName, 115200);
        } else {
            throw new IllegalArgumentException("Unknown device type: " + line);
        }
    }

    private int resolveFunctionCode(String selectedFunction) {
        if (selectedFunction == null) return 0x03;
        if (selectedFunction.startsWith("01")) return 0x01;
        if (selectedFunction.startsWith("02")) return 0x02;
        if (selectedFunction.startsWith("03")) return 0x03;
        if (selectedFunction.startsWith("04")) return 0x04;
        return 0x03;
    }

    private String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }
        return sb.toString().trim();
    }

    @FXML
    protected void PublishbuttonClicked() {
        MqttPublishAction();
    }

    @FXML
    protected void RefreshbuttonClicked() {
        loadSerialPorts();
        append("Device list refreshed!");
    }

    @FXML
    protected void ConnectbuttonClicked() {
        connectSelected();
    }
}
