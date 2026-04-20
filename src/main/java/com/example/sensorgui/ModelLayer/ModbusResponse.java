package com.example.sensorgui.ModelLayer;

public class ModbusResponse {
    private final byte[] data;
    private final String hexString;

    public ModbusResponse(byte[] data, String hexString) {
        this.data = data;
        this.hexString = hexString;
    }

    public byte[] getData() {
        return data;
    }

    public String getHexString() {
        return hexString;
    }
}
