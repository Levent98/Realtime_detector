package com.example.sensorgui.CommunicationLayer;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;

public class ModbusTCPComLayer {

    private final String host;
    private final int port;
    private Socket socket;
    private InputStream in;
    private OutputStream out;
    private int transactionId = 1;

    public ModbusTCPComLayer(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public void connect() throws Exception {
        socket = new Socket();
        socket.connect(new InetSocketAddress(host, port), 3000);
        socket.setSoTimeout(3000);
        in = socket.getInputStream();
        out = socket.getOutputStream();
    }

    public byte[] readRegisters(int unitId, int functionCode, int register, int count) throws Exception {
        byte[] pdu = new byte[5];
        pdu[0] = (byte) functionCode;
        pdu[1] = (byte) ((register >> 8) & 0xFF);
        pdu[2] = (byte) (register & 0xFF);
        pdu[3] = (byte) ((count >> 8) & 0xFF);
        pdu[4] = (byte) (count & 0xFF);

        int length = 1 + pdu.length;

        byte[] mbap = new byte[7];
        mbap[0] = (byte) ((transactionId >> 8) & 0xFF);
        mbap[1] = (byte) (transactionId & 0xFF);
        mbap[2] = 0x00;
        mbap[3] = 0x00;
        mbap[4] = (byte) ((length >> 8) & 0xFF);
        mbap[5] = (byte) (length & 0xFF);
        mbap[6] = (byte) unitId;

        out.write(mbap);
        out.write(pdu);
        out.flush();

        byte[] header = readFully(7);
        int responseLength = ((header[4] & 0xFF) << 8) | (header[5] & 0xFF);
        byte[] body = readFully(responseLength - 1);
        transactionId++;

        int returnedFunction = body[0] & 0xFF;
        if (returnedFunction == (functionCode | 0x80)) {
            int exceptionCode = body[1] & 0xFF;
            throw new RuntimeException("Modbus TCP exception: " + exceptionCode);
        }

        int byteCount = body[1] & 0xFF;
        byte[] data = new byte[byteCount];
        System.arraycopy(body, 2, data, 0, byteCount);
        return data;
    }

    private byte[] readFully(int len) throws Exception {
        byte[] buffer = new byte[len];
        int total = 0;
        while (total < len) {
            int read = in.read(buffer, total, len - total);
            if (read < 0) {
                throw new RuntimeException("Connection closed");
            }
            total += read;
        }
        return buffer;
    }

    public void disconnect() throws Exception {
        if (in != null) in.close();
        if (out != null) out.close();
        if (socket != null && !socket.isClosed()) socket.close();
    }
}
