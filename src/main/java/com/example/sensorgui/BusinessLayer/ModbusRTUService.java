package com.example.sensorgui.BusinessLayer;

import com.example.sensorgui.CommunicationLayer.ModbusRTUComLayer;
import com.example.sensorgui.ModelLayer.SensorData;
import com.fazecast.jSerialComm.SerialPort;

import java.util.Optional;

public class ModbusRTUService implements SensorService, ModbusCapableService {

    private final String portName;
    private final int baudRate;
    private ModbusRTUComLayer modbus;

    public ModbusRTUService(String portName, int baudRate) {
        this.portName = portName;
        this.baudRate = baudRate;
    }

    @Override
    public void connect() throws Exception {
        SerialPort serialPort = SerialPort.getCommPort(portName);
        serialPort.setBaudRate(baudRate);
        serialPort.setNumDataBits(8);
        serialPort.setNumStopBits(SerialPort.ONE_STOP_BIT);
        serialPort.setParity(SerialPort.NO_PARITY);
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, 3000, 1000);
        modbus = new ModbusRTUComLayer(serialPort);
    }

    @Override
    public byte[] readRegisters(int slaveId, int functionCode, int register, int count) throws Exception {
        ensureConnected();
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
    public void disconnect() {
        if (modbus != null) {
            modbus.close();
            modbus = null;
        }
    }

    private void ensureConnected() {
        if (modbus == null) {
            throw new IllegalStateException("Modbus RTU is not connected.");
        }
    }
}
