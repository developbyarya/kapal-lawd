const mqtt = require("mqtt");
const WebSocket = require("ws");

// MQTT Client
const mqttClient = mqtt.connect("mqtt://localhost"); // Replace with your MQTT broker address

// WebSocket Server
const wss = new WebSocket.Server({ port: 8080 });

// When WebSocket server is connected
wss.on("connection", (ws) => {
  console.log("WebSocket client connected");

  // Handle disconnection
  ws.on("close", () => {
    console.log("WebSocket client disconnected");
  });
});

// Subscribe to the desired MQTT topic
mqttClient.on("connect", () => {
  console.log("Connected to MQTT broker");
  mqttClient.subscribe("gps/data", (err) => {
    if (!err) {
      console.log("Subscribed to topic: gps/data");
    }
  });
});

// Parse and forward MQTT messages
mqttClient.on("message", (topic, message) => {
  console.log(`Received message from topic ${topic}: ${message.toString()}`);

  // Try to parse the incoming message as JSON
  try {
    const parsedMessage = JSON.parse(message.toString());

    // Ensure the message contains the expected fields
    if (
      parsedMessage.lat &&
      parsedMessage.lng &&
      parsedMessage.speed &&
      parsedMessage.datetime
    ) {
      // Broadcast parsed message to all WebSocket clients
      wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(JSON.stringify(parsedMessage)); // Forward the JSON data
        }
      });
    } else {
      console.error("Invalid data format in MQTT message");
    }
  } catch (err) {
    console.error("Failed to parse message as JSON:", err);
  }
});
