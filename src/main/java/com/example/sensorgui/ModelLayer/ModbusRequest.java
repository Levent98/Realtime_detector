package com.example.sensorgui.ModelLayer;

public class ModbusRequest {
    private final int slaveId;
    private final int functionCode;
    private final int registerAddress;
    private final int registerCount;

    public ModbusRequest(int slaveId, int functionCode, int registerAddress, int registerCount) {
        this.slaveId = slaveId;
        this.functionCode = functionCode;
        this.registerAddress = registerAddress;
        this.registerCount = registerCount;
    }

    public int getSlaveId() {
        return slaveId;
    }

    public int getFunctionCode() {
        return functionCode;
    }

    public int getRegisterAddress() {
        return registerAddress;
    }

    public int getRegisterCount() {
        return registerCount;
    }
}
