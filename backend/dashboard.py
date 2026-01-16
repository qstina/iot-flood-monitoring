import streamlit as st
import pandas as pd
from firebase_admin import credentials, firestore, initialize_app, _apps
import plotly.express as px
import hashlib

# --- 1. User Authentication Setup ---
# In a production app, these would be stored as hashed values in a DB
USER_CREDENTIALS = {
    "admin": "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918", # hash for 'admin'
    "qistina": "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8" # hash for 'password'
}

def make_hashes(password):
    return hashlib.sha256(str.encode(password)).hexdigest()

def check_hashes(password, hashed_text):
    if make_hashes(password) == hashed_text:
        return True
    return False

# --- 2. Firestore Setup ---
if not _apps:
    cred = credentials.Certificate("serviceAccountKey.json")
    initialize_app(cred)
db = firestore.client()

# --- 3. Login Screen Function ---
def login_screen():
    st.title("🔐 IoT Flood Monitor Login")
    
    with st.form("login_form"):
        username = st.text_input("Username")
        password = st.text_input("Password", type='password')
        submit_button = st.form_submit_button("Login")
        
        if submit_button:
            if username in USER_CREDENTIALS and check_hashes(password, USER_CREDENTIALS[username]):
                st.session_state['logged_in'] = True
                st.session_state['user'] = username
                st.success(f"Welcome back, {username}!")
                st.rerun()
            else:
                st.error("Invalid Username or Password")

# --- 4. Main Dashboard Function ---
def main_dashboard():
    st.set_page_config(page_title="IoT Flood Monitor", layout="wide")
    
    # Sidebar Logout
    st.sidebar.title(f"User: {st.session_state['user']}")
    if st.sidebar.button("Logout"):
        st.session_state['logged_in'] = False
        st.rerun()

    st.title("🌊 Smart Rain & Flood Monitoring Dashboard")
    st.markdown(f"**Security Level:** HTTPS Secured | **User:** {st.session_state['user']}")

    # Fetch Data
    docs = db.collection("flood_monitor_logs").order_by("timestamp", direction=firestore.Query.DESCENDING).limit(50).stream()
    data = [d.to_dict() for d in docs]
    df = pd.DataFrame(data)

    if not df.empty:
        latest = df.iloc[0]
        col1, col2, col3 = st.columns(3)
        col1.metric("Rain Intensity", f"{latest.get('rain_intensity', 'N/A')}")
        col2.metric("Water Distance", f"{latest.get('water_level_cm', 'N/A')} cm")
        
        alert_val = latest.get('alert_level', 'STABLE')
        if alert_val == "CRITICAL": col3.error(f"STATUS: {alert_val}")
        elif alert_val == "WARNING_RAIN": col3.warning(f"STATUS: {alert_val}")
        else: col3.success(f"STATUS: {alert_val}")

        st.divider()
        chart_col1, chart_col2 = st.columns(2)

        with chart_col1:
            fig_water = px.line(df, x="timestamp", y="water_level_cm", title="Water Surface Trend")
            st.plotly_chart(fig_water, width='stretch')

        with chart_col2:
            fig_rain = px.area(df, x="timestamp", y="rain_intensity", title="Rain Intensity")
            fig_rain.update_yaxes(autorange="reversed")
            st.plotly_chart(fig_rain, width='stretch')

        st.subheader("Protected Logs")
        st.dataframe(df[["timestamp", "rain_intensity", "water_level_cm", "alert_level"]], width='stretch')
    else:
        st.info("No data found in Firestore.")

# --- 5. Logic Controller ---
if 'logged_in' not in st.session_state:
    st.session_state['logged_in'] = False

if not st.session_state['logged_in']:
    login_screen()
else:
    main_dashboard()
