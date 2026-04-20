package com.example.sensorgui;

import javafx.application.Application;


public class Launcher {


    public static void main(String[] args) {
        // Start Spring Boot context

        // Launch JavaFX
        Application.launch(HelloApplication.class, args);
    }

    // Provide access to Spring context if needed

}