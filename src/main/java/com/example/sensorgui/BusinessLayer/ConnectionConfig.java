package com.example.sensorgui.BusinessLayer;

public class ConnectionConfig {
    private final DeviceType deviceType;
    private final String hostOrPortName;
    private final int portOrBaudRate;

    public ConnectionConfig(DeviceType deviceType, String hostOrPortName, int portOrBaudRate) {
        this.deviceType = deviceType;
        this.hostOrPortName = hostOrPortName;
        this.portOrBaudRate = portOrBaudRate;
    }

    public DeviceType getDeviceType() {
        return deviceType;
    }

    public String getHostOrPortName() {
        return hostOrPortName;
    }

    public int getPortOrBaudRate() {
        return portOrBaudRate;
    }
}
