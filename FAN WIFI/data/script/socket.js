var connection;
var header = document.getElementById("speed");
var btns = header.getElementsByClassName("btn");
var fan = document.getElementById("spinner");
var power = false;
var number = 0;

// Establish WebSocket connection with auto-reconnect
function connectWebSocket() {
    connection = new WebSocket('ws://' + location.hostname + ':81/');

    connection.onopen = function () {
        console.log("✅ WebSocket connected.");
        // requestData(); // Request current status on connect
    };

    connection.onerror = function (error) {
        console.error("❌ WebSocket Error:", error);
    };

    connection.onclose = function () {
        console.warn("⚠️ WebSocket closed. Reconnecting...");
        setTimeout(connectWebSocket, 3000); // Reconnect after 3 seconds
    };

    connection.onmessage = function (event) {
        try {
            var data = JSON.parse(event.data);
            if (data.speed !== undefined) {
                power = data.power;
                number = data.speed;
                console.log(`📡 Received: Speed=${number}, Power=${power}`);
                updateActiveButton(number);
                updateFanAnimation(number);
            }
        } catch (e) {
            console.error("❌ Invalid JSON:", event.data);
        }
    };
}

// Request fan status from the server
function requestData() {
    if (connection.readyState === WebSocket.OPEN) {
        connection.send(JSON.stringify({ request: "status" }));
    }
}

// Handle button clicks and set speed
header.addEventListener("click", function (event) {
    if (event.target.classList.contains("btn")) {
        let speed = parseInt(event.target.dataset.speed);
        toggle(speed);
    }
});

// Function to update active speed button
function updateActiveButton(speed) {
    for (let btn of btns) {
        btn.classList.toggle("active", parseInt(btn.getAttribute("data-speed")) === speed);
    }
}

// Function to update fan animation
function updateFanAnimation(speed) {
    fan.classList.remove("spin1", "spin2", "spin3"); // Reset animation
    if (speed > 0) {
        fan.classList.add(`spin${speed}`);
    }
}

// Send speed and power data to ESP32
function sendData() {
    if (connection.readyState === WebSocket.OPEN) {
        var full_data = JSON.stringify({ SPEED: number, POWER: power });
        connection.send(full_data);
    } else {
        console.warn("⚠️ WebSocket not open. Data not sent.");
    }
}

// Toggle speed and power
function toggle(speed) {
    console.log(`🔄 Toggling: Speed=${speed}`);
    number = speed;
    power = speed > 0;
    sendData();
}
// Auto-run WebSocket connection on page load
window.onload = function () {
    connectWebSocket();  // Ensure connection is established
};

