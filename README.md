# 🌊 Smart Rain & Flood Monitoring System (CPC357)

This project is a secure, end-to-end IoT solution designed to provide early warnings for rain and flood events. It utilizes an **ESP32** for edge sensing, **Google Cloud Platform (GCP)** for infrastructure, and **Streamlit** for a secured data dashboard.



---

## 🏗️ System Architecture
The system follows a 3-tier IoT architecture:
1.  **Edge Layer**: ESP32 equipped with a Raindrops Module (Analog) and Ultrasonic Sensor (HC-SR04) to monitor environmental conditions.
2.  **Communication Layer**: A **Mosquitto MQTT Broker** hosted on a GCP Compute Engine instance, secured via **TLS/SSL (Port 8883)**.
3.  **Cloud & Application Layer**: A Python-based **Firestore Bridge** to ingest data into a NoSQL database and a **Streamlit** web dashboard for real-time visualization.

---

## 🛠️ Installation & Backend Setup

### 1. Virtual Environment Setup
To isolate project dependencies, create and activate a Python virtual environment:
```bash
sudo apt update
sudo apt install python3-venv -y
python3 -m venv assignment_env
source assignment_env/bin/activate
pip install firebase-admin paho-mqtt streamlit pandas plotly
```

### 2. Security: Generating SSL/TLS Certificates
Data privacy is ensured using self-signed certificates for MQTTS (8883) and HTTPS (8080).
```bash
# Create directory for certificates
sudo mkdir -p /etc/mosquitto/certs
cd /etc/mosquitto/certs

# Generate Root CA
sudo openssl genrsa -out ca.key 2048
sudo openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj "/CN=MyLocalCA"

# Generate Server Certificate (Replace YOUR_VM_IP with your GCP External IP)
sudo openssl genrsa -out server.key 2048
sudo openssl req -new -key server.key -out server.csr -subj "/CN=YOUR_VM_IP"
sudo openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650

# Set ownership for the Mosquitto service
sudo chown -R mosquitto:mosquitto /etc/mosquitto/certs
```

### 3. Mosquitto Broker Configuration
Modify /etc/mosquitto/mosquitto.conf to enable the secure listener:
```bash
# --- Persistence & Logging ---
persistence true
persistence_location /var/lib/mosquitto/
log_dest file /var/log/mosquitto/mosquitto.log
log_type all

# --- Default Listener (Unencrypted) ---
# Useful for local testing as per Lab Sheet 8
listener 1883 0.0.0.0
allow_anonymous true

# --- Secure Listener (SSL/TLS) ---
# Requires certificates in /etc/mosquitto/certs/
listener 8883 0.0.0.0
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

# Set to false to allow clients to connect without their own certificate
require_certificate false

```
Restart the service:
```bash
sudo systemctl restart mosquitto
```

---

## 🖥️ Application Components
**Firestore Bridge** (firestore_bridge.py)
This script acts as a subscriber to the MQTT broker and pushes JSON payloads to Google Cloud Firestore.

- **Dependencies:** firebase-admin, paho-mqtt
- **Security:** Uses a GCP Service Account JSON key with limited IAM permissions.

**User Dashboard** (dashboard.py)
A secure Streamlit application providing real-time metrics and historical trends.

- **Authentication:** Login gate implemented with SHA-256 password hashing.
- **Access:** Accessible via https://YOUR_VM_IP:8080.
  
---

## 🔒 Security Strategies
- **Transport Encryption:** All sensor data is sent via TLS v1.2. The web dashboard is served over HTTPS.
- **Access Control:** Implemented Google Cloud IAM roles to ensure the bridge can only access the necessary Firestore collections.
- **Application Security:** Session state management and hashed credentials prevent unauthorized access to the monitoring logs.
  
---

## 📁 Project Structure
```bash
├── dashboard.py           # Streamlit Web App
├── firestore_bridge.py    # MQTT-to-Firestore Bridge
├── esp32_flood_node.ino   # ESP32 Firmware
├── ca.crt                 # Root CA Certificate (For ESP32)
├── server.crt             # Server Certificate
├── server.key             # Private Key (Keep secret!)
└── README.md              # Documentation
```
---

## 📜 Usage
1. Power on the ESP32; it will connect to WiFi and the secure MQTT broker.
2. Run the bridge: python3 firestore_bridge.py.
3. Launch the dashboard: streamlit run dashboard.py --server.port 8080 --server.address 0.0.0.0 --server.sslCertFile server.crt --server.sslKeyFile server.key.
4. Log in using the authorized credentials.

---
