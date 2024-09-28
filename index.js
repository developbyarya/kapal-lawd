const mqtt = require("mqtt");
const WebSocket = require("ws");

// MQTT broker configuration
const mqttBroker = "mqtt:localhost"; // Replace with your MQTT broker address
const mqttTopic = "gps/location"; // The topic to subscribe to

// WebSocket server configuration
const wsPort = 8080; // Port for WebSocket server

// Create MQTT client and connect to broker
const mqttClient = mqtt.connect(mqttBroker);

// Create WebSocket server
const wss = new WebSocket.Server({ port: wsPort });

mqttClient.on("connect", () => {
  console.log("Connected to MQTT broker");
  mqttClient.subscribe(mqttTopic, (err) => {
    if (err) {
      console.error(`Failed to subscribe to topic ${mqttTopic}:`, err);
    } else {
      console.log(`Subscribed to topic: ${mqttTopic}`);
    }
  });
});

mqttClient.on("message", (topic, message) => {
  const gpsData = message.toString();
  console.log(`Received message: ${gpsData} on topic: ${topic}`);

  // Forward GPS data to all connected WebSocket clients
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(gpsData);
    }
  });
});

wss.on("connection", (ws) => {
  console.log("New client connected");

  ws.on("close", () => {
    console.log("Client disconnected");
  });
});

console.log(`WebSocket server is running on ws://localhost:${wsPort}`);
