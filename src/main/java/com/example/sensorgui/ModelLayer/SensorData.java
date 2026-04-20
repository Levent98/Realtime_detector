package com.example.sensorgui.ModelLayer;

import java.time.LocalDateTime;
import java.util.Arrays;

public class SensorData {

    private String sensorId;
    private double value;
    private String unit;
    private LocalDateTime timestamp;
    private String sourceProtocol;
    private Integer registerAddress;
    private Integer functionCode;
    private byte[] rawPayload;
    private String quality;

    public SensorData(String id, double value, String unit) {
        this(id, value, unit, LocalDateTime.now());
    }

    public SensorData(String id, double value, String unit, LocalDateTime timestamp) {
        this.sensorId = id;
        this.value = value;
        this.unit = unit;
        this.timestamp = timestamp;
    }

    public String getSensorId() {
        return sensorId;
    }

    public void setSensorId(String sensorId) {
        this.sensorId = sensorId;
    }

    public double getValue() {
        return value;
    }

    public void setValue(double value) {
        this.value = value;
    }

    public String getUnit() {
        return unit;
    }

    public void setUnit(String unit) {
        this.unit = unit;
    }

    public LocalDateTime getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(LocalDateTime timestamp) {
        this.timestamp = timestamp;
    }

    public String getSourceProtocol() {
        return sourceProtocol;
    }

    public void setSourceProtocol(String sourceProtocol) {
        this.sourceProtocol = sourceProtocol;
    }

    public Integer getRegisterAddress() {
        return registerAddress;
    }

    public void setRegisterAddress(Integer registerAddress) {
        this.registerAddress = registerAddress;
    }

    public Integer getFunctionCode() {
        return functionCode;
    }

    public void setFunctionCode(Integer functionCode) {
        this.functionCode = functionCode;
    }

    public byte[] getRawPayload() {
        return rawPayload;
    }

    public void setRawPayload(byte[] rawPayload) {
        this.rawPayload = rawPayload;
    }

    public String getQuality() {
        return quality;
    }

    public void setQuality(String quality) {
        this.quality = quality;
    }

    @Override
    public String toString() {
        return "SensorData{" +
                "sensorId='" + sensorId + '\'' +
                ", value=" + value +
                ", unit='" + unit + '\'' +
                ", timestamp=" + timestamp +
                ", sourceProtocol='" + sourceProtocol + '\'' +
                ", registerAddress=" + registerAddress +
                ", functionCode=" + functionCode +
                ", rawPayload=" + Arrays.toString(rawPayload) +
                ", quality='" + quality + '\'' +
                '}';
    }
}
