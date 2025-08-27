import express from "express";
import cors from "cors";
import bodyParser from "body-parser";
import { WebSocketServer } from "ws";

const PORT = 7681;

const app = express();
const server = app.listen(PORT, "localhost", () => {
    console.log(`Server running at http://localhost:${PORT}`);
});
const wss = new WebSocketServer({ server, path: "/ws" });

// -------------------
// Middleware
// -------------------
app.use(cors({
    origin: "*",
    methods: ["GET", "POST", "OPTIONS"],
    allowedHeaders: ["Content-Type"],
}));

app.use(bodyParser.json());

// -------------------
// Example data
// -------------------
let uart_settings_example = {
    baudrate: 115200,
    bits: 8,
    parity: null,
    stop: 1
};

let gpio_settings_example = [
    { gpio: 1, mode: 1, dval: 2000, state: 0, name: "button_blue", desc: "Fake button",     color: "blue" },
    { gpio: 2, mode: 0, dval: 0,    state: 0, name: "led_red",     desc: "Fake red led",    color: "red" },
    { gpio: 3, mode: 0, dval: 0,    state: 0, name: "led_yellow",  desc: "Fake yellow led", color: "yellow" }
];

// -------------------
// Endpoints GET
// -------------------
app.get("/token", (req, res) => {
    res.json({ token: "my_secret_token" });
});

app.get("/stty", (req, res) => {
    res.json(uart_settings_example);
});

app.get("/gpio", (req, res) => {
    res.json(gpio_settings_example);
});

app.get("/stats", (req, res) => {
    res.json({tx: 10, rx: 20, txRateBps: 30, rxRateBps: 40 });
});

app.get("/heap", (req, res) => {
    res.set('Content-Type', 'text/plain');
    res.send("51200")
});

app.get("/reset", (req, res) => {
    res.json({status: "OK" });
});

app.get("/whoami", (req, res) => {
    res.json({
        board: "Fake ESP8266 board",
        pretty_name: "Node.js test server",
        hostname: "Wi-Se",
        software: {
            implementation: "Wi-Se C++",
            version: "1.2.3"
        },
        soc: {
            type: "ESP8266",
            chipId: 12345678,
            sdk: "v4.4.3",
            mhz: 160
        },
        health: {
            vccVoltage: 3.3,
            heapFree: 51200,
            heapFrag: 10
        },
        net: {
            wifiMode: "sta",
            ssid: "MyWiFi",
            bssid: "AA:BB:CC:DD:EE:FF",
            macAddr: "11:22:33:44:55:66",
            ip: "192.168.1.100",
            netmask: "255.255.255.0",
            gateway: "192.168.1.1",
            rssi: -55
        }
    });
});

// -------------------
// Endpoints POST
// -------------------
app.post("/gpio", async (req, res) => {
    const data = req.body;

    if ("button_blue" in data) {
        gpio_settings_example[2].state = data.button_blue;
        setTimeout(() => {
            gpio_settings_example[2].state = 0;
        }, data.button_blue);
    }

    res.json({ received: data, status: "OK" });
});

app.post("/stty", (req, res) => {
    uart_settings_example = req.body;
    res.json({ received: uart_settings_example, status: "OK" });
});

// -------------------
// WebSocket
// -------------------
wss.on("connection", (ws, req) => {
    console.log("Client connected via WS!");

    if (req.headers["sec-websocket-protocol"] !== "tty") {
        ws.close(1002, "Protocol required: tty");
        return;
    }

    ws.send(Buffer.from("1Node.js fake esp!"));

    // -------------------
    // Receiver loop
    // -------------------
    ws.on("message", (msg) => {
        if (Buffer.isBuffer(msg)) {
            if (msg[0] === 48) { // '0'
                console.log("Echo (bytes):", msg, "->", "\"",msg.toString(),"\"");
                ws.send(msg);
            } else {
                console.log("Received (bytes):", msg[0]);
            }
        } else {
            console.log("Received (text):", msg.toString());
        }
    });

    ws.on("close", () => {
        // clearInterval(sender);
        console.log("Client disconnected!");
    });
});

const sender = setInterval(() => {    
    const msg = Buffer.concat([
        Buffer.from("G"),
        Buffer.from(gpio_settings_example.map(g => g.state ? "1" : "0").join(""))
    ]);

    wss.clients.forEach(ws => {
        if (ws.readyState === ws.OPEN) {
            ws.send(msg);
        }
    });

    // console.log("Sent:", msg.toString());
    gpio_settings_example[1].state ^= 1;
}, 1000);
