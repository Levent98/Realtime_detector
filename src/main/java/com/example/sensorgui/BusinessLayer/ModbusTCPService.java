package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.CommunicationLayer.ModbusTCPComLayer;
import com.example.sensorgui.ModelLayer.SensorData;

import java.util.Optional;

public class ModbusTCPService implements SensorService, ModbusCapableService {

    private final ModbusTCPComLayer modbus;

    public ModbusTCPService(String host, int port) {
        this.modbus = new ModbusTCPComLayer(host, port);
    }

    @Override
    public void connect() throws Exception {
        modbus.connect();
    }

    @Override
    public byte[] readRegisters(int slaveId, int functionCode, int register, int count) throws Exception {
        return modbus.readRegisters(slaveId, functionCode, register, count);
    }

    @Override
    public Optional<SensorData> getSensorData() {
        return Optional.empty();
    }

    @Override
    public void sendCommand(String command) {
        throw new UnsupportedOperationException("Use Modbus register operations instead of plain commands.");
    }

    @Override
    public void disconnect() throws Exception {
        modbus.disconnect();
    }
}
