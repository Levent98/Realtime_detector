package com.example.sensorgui.CommunicationLayer;

import com.example.sensorgui.ModelLayer.SensorData;

import java.util.Optional;




public interface SensorCommunication {
        /**
         * Read data from a sensor
         * @return Optional<SensorData> if available
         */
        Optional<SensorData> readData();

        /**
         * Send command or data to sensor
         * @param command The command string
         */
        void sendCommand(String command);

        /**
         * Initialize the communication (open port / connect)
         */
        void connect() throws Exception;

        /**
         * Close communication (disconnect)
         */
        void disconnect() throws Exception;
}
