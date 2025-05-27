import paho.mqtt.client as mqtt
import time
import datetime
import csv
import os
import signal
import sys
import traceback

MQTT_BROKER = "10.251.255.73"
MQTT_PORT = 1883
MQTT_KEEPALIVE = 5
CHECK_INTERVAL = 5
LOG_FILE = "mqtt_availability_log.csv"
SUMMARY_FILE = "mqtt_availability_summary.txt"
SUMMARY_INTERVAL = 300

start_time = datetime.datetime.now()
connection_attempts = 0
successful_connections = 0
failed_connections = 0
current_status = False
last_status_change = start_time
downtime_periods = []
current_downtime_start = None
last_summary_time = start_time

def init_log_file():
    file_exists = os.path.isfile(LOG_FILE)
    with open(LOG_FILE, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(['Timestamp', 'Status', 'Response Time (ms)'])

def on_connect(client, userdata, flags, rc):
    global current_status, successful_connections, current_downtime_start, last_status_change
    
    if rc == 0:
        successful_connections += 1
        if not current_status:
            current_status = True
            if current_downtime_start:
                downtime_duration = datetime.datetime.now() - current_downtime_start
                downtime_periods.append((current_downtime_start, datetime.datetime.now(), downtime_duration))
                current_downtime_start = None
            last_status_change = datetime.datetime.now()
            print(f"[{datetime.datetime.now()}] Connected to MQTT Broker! Server is back ONLINE.")
    else:
        handle_connection_failure()

def on_disconnect(client, userdata, rc):
    if rc != 0:
        handle_connection_failure()

def handle_connection_failure():
    global current_status, failed_connections, current_downtime_start, last_status_change
    
    failed_connections += 1
    if current_status:
        current_status = False
        current_downtime_start = datetime.datetime.now()
        last_status_change = datetime.datetime.now()
        print(f"[{datetime.datetime.now()}] Failed to connect to MQTT Broker! Server is OFFLINE.")

def check_mqtt_availability():
    global connection_attempts
    
    connection_attempts += 1
    timestamp = datetime.datetime.now()
    response_time = None
    
    try:
        client = mqtt.Client()
        client.on_connect = on_connect
        client.on_disconnect = on_disconnect
        
        client.connect_async(MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE)
        
        start_time = time.time()
        client.loop_start()

        time.sleep(2)
        
        if current_status:
            response_time = round((time.time() - start_time) * 1000, 2)
        
        client.loop_stop()
        client.disconnect()
        
    except Exception as e:
        print(f"Error during connection attempt: {e}")
        handle_connection_failure()
    
    status_str = "UP" if current_status else "DOWN"
    response_str = f"{response_time} ms" if response_time else "N/A"
    print(f"[{timestamp}] Status: {status_str}, Response: {response_str}")
    
    with open(LOG_FILE, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([timestamp, status_str, response_time if response_time else ""])
    
    return current_status

def calculate_statistics():
    current_time = datetime.datetime.now()
    monitoring_duration = current_time - start_time
    
    total_downtime = datetime.timedelta()
    for period in downtime_periods:
        total_downtime += period[2]
    
    if not current_status and current_downtime_start:
        current_downtime = current_time - current_downtime_start
        total_downtime += current_downtime
    
    total_uptime = monitoring_duration - total_downtime
    
    uptime_percentage = (total_uptime.total_seconds() / monitoring_duration.total_seconds()) * 100
    downtime_percentage = (total_downtime.total_seconds() / monitoring_duration.total_seconds()) * 100
    
    def format_timedelta(td):
        total_seconds = int(td.total_seconds())
        hours = total_seconds // 3600
        minutes = (total_seconds % 3600) // 60
        seconds = total_seconds % 60
        return f"{hours}h {minutes}m {seconds}s"
    
    summary = f"""
    MQTT Server Availability Report
    ==============================
    Start time: {start_time}
    End time: {current_time}
    Monitoring duration: {format_timedelta(monitoring_duration)}

    Connection Statistics:
    - Total connection attempts: {connection_attempts}
    - Successful connections: {successful_connections}
    - Failed connections: {failed_connections}
    - Success rate: {successful_connections/connection_attempts*100:.2f}%

    Availability:
    - Current status: {"ONLINE" if current_status else "OFFLINE"}
    - Total uptime: {format_timedelta(total_uptime)} ({uptime_percentage:.2f}%)
    - Total downtime: {format_timedelta(total_downtime)} ({downtime_percentage:.2f}%)
    - Number of outages: {len(downtime_periods) + (1 if not current_status and current_downtime_start else 0)}

    Longest outage: 
    """

    longest_outage = datetime.timedelta()
    longest_outage_period = None
    
    for period in downtime_periods:
        if period[2] > longest_outage:
            longest_outage = period[2]
            longest_outage_period = period
    
    current_outage = None
    if not current_status and current_downtime_start:
        current_outage = current_time - current_downtime_start
        if current_outage > longest_outage:
            longest_outage = current_outage
            longest_outage_period = (current_downtime_start, "ongoing", current_outage)
    
    if longest_outage_period:
        if longest_outage_period[1] == "ongoing":
            summary += f"{format_timedelta(longest_outage)} (started at {longest_outage_period[0]} and still ongoing)"
        else:
            summary += f"{format_timedelta(longest_outage)} (from {longest_outage_period[0]} to {longest_outage_period[1]})"
    else:
        summary += "No outages recorded"
    
    summary += "\n\nLast 5 status changes:\n"
    
    status_changes = []
    try:
        with open(LOG_FILE, 'r') as csvfile:
            reader = csv.reader(csvfile)
            next(reader)
            
            prev_status = None
            for row in reader:
                if len(row) >= 2:
                    timestamp, status = row[0], row[1]
                    if prev_status is not None and status != prev_status:
                        status_changes.append((timestamp, status))
                    prev_status = status
    except Exception as e:
        summary += f"Error reading status changes: {str(e)}\n"
    
    for i, (timestamp, status) in enumerate(status_changes[-5:]):
        summary += f"- {timestamp}: Changed to {status}\n"
    
    return summary

def save_summary():
    try:
        summary = calculate_statistics()
        
        with open(SUMMARY_FILE, 'w') as f:
            f.write(summary)
        
        print(f"\n[{datetime.datetime.now()}] Summary report saved to {SUMMARY_FILE}")
    except Exception as e:
        print(f"\n[{datetime.datetime.now()}] Error saving summary: {e}")

def handle_exit(sig, frame):
    print("\nGenerating final report...")
    save_summary()
    print(f"Detailed log saved to {LOG_FILE}")
    print("\nExiting MQTT availability monitor. Goodbye!")
    sys.exit(0)

def main():
    global last_summary_time
    
    print(f"MQTT Server Availability Monitor")
    print(f"===============================")
    print(f"Target: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Check interval: {CHECK_INTERVAL} seconds")
    print(f"Summary save interval: {SUMMARY_INTERVAL} seconds")
    print(f"Started at: {start_time}")
    print(f"Press Ctrl+C to stop and exit")
    print(f"===============================")
    
    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)
    
    init_log_file()
    
    try:
        while True:
            check_mqtt_availability()
            
            current_time = datetime.datetime.now()
            time_since_last_summary = (current_time - last_summary_time).total_seconds()
            
            if time_since_last_summary >= SUMMARY_INTERVAL:
                save_summary()
                last_summary_time = current_time
            
            time.sleep(CHECK_INTERVAL)
            
    except Exception as e:
        print(f"Unhandled error in main loop: {e}")
        print(traceback.format_exc())
        save_summary()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Critical error: {e}")
        print(traceback.format_exc())
        try:
            save_summary()
            print("Emergency summary saved before exit")
        except:
            print("Could not save emergency summary")