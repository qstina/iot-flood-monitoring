import paho.mqtt.client as mqtt
import firebase_admin
from firebase_admin import credentials, firestore
import json

# 1. Setup Firestore
cred = credentials.Certificate("/home/qistinaaanoruden03/serviceAccountKey.json")
firebase_admin.initialize_app(cred)
db = firestore.client()

def on_message(client, userdata, message):
    try:
        payload = json.loads(message.payload.decode("utf-8"))
        payload["timestamp"] = firestore.SERVER_TIMESTAMP
        
        # Storing in a logical collection name
        db.collection("flood_monitor_logs").add(payload)
        print(f"Logged to Firestore: {payload['alert_level']} - Rain: {payload['rain_intensity']}")
    except Exception as e:
        print(f"Error: {e}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.tls_set(ca_certs="/etc/mosquitto/certs/ca.crt")
client.tls_insecure_set(True) 
client.connect("localhost", 8883)
client.subscribe("projects/sublime-lodge-481702-c8/topics/assgn-2")

print("Flood Monitor Bridge Active...")
client.loop_forever()
