package com.example.sensorgui.CommunicationLayer;

import com.example.sensorgui.ModelLayer.SensorData;

public interface DataListener {
    void onDataReceived(SensorData data);
    void onError(Exception e);
    void onDisconnected();
}