package com.example.sensorgui.CommunicationLayer;

import com.fazecast.jSerialComm.SerialPort;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class ModbusRTUComLayer {
    private final SerialPort serialPort;

    public ModbusRTUComLayer(SerialPort serialPort) throws Exception {
        this.serialPort = serialPort;
        if (!serialPort.openPort()) {
            throw new Exception("Failed to open serial port: " + serialPort.getSystemPortName());
        }
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, 1000, 500);
    }

    public byte[] readRegisters(int slaveId, int functionCode, int register, int count) throws IOException {
        InputStream in = serialPort.getInputStream();
        OutputStream out = serialPort.getOutputStream();

        byte[] request = new byte[8];
        request[0] = (byte) slaveId;
        request[1] = (byte) functionCode;
        request[2] = (byte) ((register >> 8) & 0xFF);
        request[3] = (byte) (register & 0xFF);
        request[4] = (byte) ((count >> 8) & 0xFF);
        request[5] = (byte) (count & 0xFF);

        int crc = computeCRC(request, 6);
        request[6] = (byte) (crc & 0xFF);
        request[7] = (byte) ((crc >> 8) & 0xFF);

        while (in.available() > 0) {
            in.read();
        }

        out.write(request);
        out.flush();

        try {
            Thread.sleep(5);
        } catch (InterruptedException ignored) {
            Thread.currentThread().interrupt();
        }

        byte[] header = new byte[3];
        readFully(in, header, 3);

        int function = header[1] & 0xFF;
        int byteCount = header[2] & 0xFF;

        if (function == (functionCode | 0x80)) {
            byte[] exceptionFrame = new byte[2];
            readFully(in, exceptionFrame, 2);
            int exceptionCode = exceptionFrame[0] & 0xFF;
            throw new IOException("Modbus exception: " + exceptionCode);
        }

        byte[] remaining = new byte[byteCount + 2];
        readFully(in, remaining, remaining.length);

        byte[] fullFrame = new byte[3 + remaining.length];
        System.arraycopy(header, 0, fullFrame, 0, 3);
        System.arraycopy(remaining, 0, fullFrame, 3, remaining.length);

        if (!verifyCRC(fullFrame, fullFrame.length)) {
            throw new IOException("CRC check failed");
        }

        byte[] data = new byte[byteCount];
        System.arraycopy(fullFrame, 3, data, 0, byteCount);
        return data;
    }

    private void readFully(InputStream in, byte[] buffer, int length) throws IOException {
        int offset = 0;
        long startTime = System.currentTimeMillis();
        int timeoutMs = 1000;

        while (offset < length) {
            if (System.currentTimeMillis() - startTime > timeoutMs) {
                throw new IOException("Read timeout");
            }

            int read = in.read(buffer, offset, length - offset);
            if (read > 0) {
                offset += read;
            }
        }
    }

    private boolean verifyCRC(byte[] data, int length) {
        int computedCRC = computeCRC(data, length - 2);
        int receivedCRC = (data[length - 1] & 0xFF) << 8 | (data[length - 2] & 0xFF);
        return computedCRC == receivedCRC;
    }

    private int computeCRC(byte[] data, int length) {
        int crc = 0xFFFF;
        for (int i = 0; i < length; i++) {
            crc ^= data[i] & 0xFF;
            for (int j = 0; j < 8; j++) {
                if ((crc & 0x0001) != 0) {
                    crc >>= 1;
                    crc ^= 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    public void close() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }
}
