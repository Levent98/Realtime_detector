package com.example.sensorgui.BusinessLayer;

public class SensorServiceFactory {

    public enum ServiceType {
        TCP,
        SERIAL,
        MODBUS_RTU,
        MODBUS_TCP,
        MQTT
    }

    public static SensorService createService(ServiceType type, String param1, int param2) {
        return switch (type) {
            case TCP -> new TCPService(param1, param2);
            case SERIAL -> new SerialService(param1, param2);
            case MODBUS_TCP -> new ModbusTCPService(param1, param2);
            case MQTT -> new MQTTService(param1, "sensor-gui-client");
            case MODBUS_RTU -> throw new IllegalArgumentException("Use createModbusRtuService for MODBUS_RTU");
        };
    }

    public static ModbusRTUService createModbusRtuService(String portName, int baudRate) {
        return new ModbusRTUService(portName, baudRate);
    }

    public static MQTTService createMqttService(String brokerUrl, String clientId) {
        return new MQTTService(brokerUrl, clientId);
    }
}
