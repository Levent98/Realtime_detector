package com.example.sensorgui.ModelLayer;


public class Sensor {

    public enum ConnectionType {
        SERIAL, TCP
    }

    private String id;
    private String name; // Human-friendly name
    private String type; // Temperature, Humidity, etc.
    private ConnectionType connectionType;
    private String address; // COM port for SERIAL, IP:Port for TCP

    public Sensor(String id, String name, String type, ConnectionType connectionType, String address) {
        this.id = id;
        this.name = name;
        this.type = type;
        this.connectionType = connectionType;
        this.address = address;
    }

    // Getters and Setters
    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public ConnectionType getConnectionType() {
        return connectionType;
    }

    public void setConnectionType(ConnectionType connectionType) {
        this.connectionType = connectionType;
    }

    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        this.address = address;
    }

    @Override
    public String toString() {
        return "Sensor{" +
                "id='" + id + '\'' +
                ", name='" + name + '\'' +
                ", type='" + type + '\'' +
                ", connectionType=" + connectionType +
                ", address='" + address + '\'' +
                '}';
    }
}

