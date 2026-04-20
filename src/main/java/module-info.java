module com.example.sensorgui {
    requires javafx.controls;
    requires javafx.fxml;
    requires org.controlsfx.controls;
    requires com.fazecast.jSerialComm;
    requires org.eclipse.paho.client.mqttv3;

    opens com.example.sensorgui to javafx.fxml;
    opens com.example.sensorgui.ControllerClasses to javafx.fxml;

    exports com.example.sensorgui;
    exports com.example.sensorgui.ControllerClasses;
}
