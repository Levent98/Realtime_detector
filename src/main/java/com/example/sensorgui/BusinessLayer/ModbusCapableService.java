package com.example.sensorgui.BusinessLayer;

public interface ModbusCapableService {
    byte[] readRegisters(int slaveId, int functionCode, int register, int count) throws Exception;
}
