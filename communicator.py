import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import time
import requests
from datetime import datetime


# Firebase configuration
cred = credentials.Certificate("puredrop-d420e-firebase-adminsdk-cst16-582fe6ee56.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://puredrop-d420e-default-rtdb.firebaseio.com/'
})

# Function to send data to Firebase
def send_to_firebase(tds, ec, turbidity, ph, temp):
    year = datetime.now().strftime("%Y")
    month = datetime.now().strftime("%m") 
    day = datetime.now().strftime("%d") 
    milliseconds = datetime.now().strftime("%f")[:3] 
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    _timestamp = datetime.now().strftime("%H:%M:%S")
    formatted_time = day + "_" + _timestamp + "_" + milliseconds
    data = {
        'Time': f"{_timestamp}",
        'TDS': f"{tds}",
        'EC': f"{ec}",
        'Turbidity': f"{turbidity}",
        'pH Level': f"{ph}",
        'Temperature': f"{temp}"
    }
    ref = db.reference('your_data_node')
    ref.child(year).child(month).child(formatted_time).set(data)
    print("Data sent to Firebase")

def get_button_state():
    ref = db.reference('switch_button') 
    button_state = ref.get() 
    return button_state

def send_button_state_to_esp8266(button_state):
    try:
        response = requests.post(f"http://{esp8266_ip_address}/get_switch_state", json=button_state)
        if response.status_code == 200:
            print("Button state sent to ESP8266")
        else:
            print("Failed to send button state, status code:", response.status_code)
    except Exception as e:
        print("Error sending button state:", e)


#esp8266_ip_address = '192.168.100.187:80'
esp8266_ip_address = '192.168.8.103:80'

# Main loop to read data from NodeMCU and send to Firebase
while True:
    try:
        send_button_state_to_esp8266(get_button_state())
        url = f'http://{esp8266_ip_address}/data'
        response = requests.get(url)
        if response.status_code == 200:
            data = response.text
            recieved_data = data.strip().split(",")
            print(f"TDS: {recieved_data[0]}")
            print(f"EC: {recieved_data[1]}")
            print(f"Turbidity: {recieved_data[2]}")
            print(f"pH: {recieved_data[3]}")
            print(f"Temperature: {recieved_data[4]}")

            f_tds = "{:.2f}".format(float(recieved_data[0]))
            f_ec = "{:.2f}".format(float(recieved_data[1]))
            f_turbidity = "{:.2f}".format(float(recieved_data[2]))
            f_ph = "{:.2f}".format(float(recieved_data[3]))
            f_temp = recieved_data[4]
            
            send_to_firebase(f_tds, f_ec, f_turbidity, f_ph, f_temp)
            
            time.sleep(2)  # Adjust the delay as needed
        else:
            print(response.status_code)
    except KeyboardInterrupt:
        print("Process interrupted.")
        break
    except Exception as e:
        #print(tds, turbidity, ph)
        print("An error occurred:", e)
