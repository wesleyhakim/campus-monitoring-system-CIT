import time
import json
import signal
import sys
from datetime import datetime
from collections import defaultdict

import pandas as pd
import numpy as np
import paho.mqtt.client as mqtt

BROKER_HOST = "10.251.255.73"
TOPIC = "stations/+/data"
DURATION_S = 1800

records = []
seq_tracker = defaultdict(set)
stop_requested = False

def on_message(client, userdata, msg):
    topic_parts = msg.topic.split('/')
    station_id = topic_parts[1] if len(topic_parts) > 1 else "unknown"

    recv_time = time.time()
    payload = msg.payload.decode()

    try:
        data = json.loads(payload)
    except:
        data = {"raw": payload}

    seq = data.get("seq", None)

    if seq is not None:
        seq_tracker[station_id].add(seq)

    records.append({
        "timestamp": datetime.utcnow(),
        "station_id": station_id,
        "seq": seq,
        "raw": data
    })

def signal_handler(sig, frame):
    global stop_requested
    print("\n🛑 Interrupt received. Stopping data collection...")
    stop_requested = True

def analyze_data():
    """Analyze collected data and print results"""
    if not records:
        print("⚠️ No data received.")
        return

    df = pd.DataFrame(records)
    df["timestamp"] = pd.to_datetime(df["timestamp"])

    print(f"🔍 Total messages: {len(df)}")
    print(df.groupby("station_id").size())

    print("\n📊 Per-Station Analysis:")
    summaries = []

    for station, group in df.groupby("station_id"):
        group = group.sort_values("timestamp")
        deltas = group["timestamp"].diff().dropna().dt.total_seconds()

        seqs = group["seq"].dropna().astype(int)
        if not seqs.empty:
            expected = set(range(seqs.min(), seqs.max() + 1))
            received = set(seqs)
            loss_pct = 100 * (len(expected - received) / len(expected)) if expected else 0
        else:
            loss_pct = np.nan

        test_duration = (group["timestamp"].max() - group["timestamp"].min()).total_seconds()
        expected_count = int(test_duration // 2) + 1
        delivery_rate = len(group) / expected_count * 100 if expected_count > 0 else 0
        jitter = deltas.std() if not deltas.empty else np.nan
        bursts = (deltas < 1.0).sum()

        summaries.append({
            "station_id": station,
            "msg_count": len(group),
            "expected_msgs": expected_count,
            "delivery_rate_pct": delivery_rate,
            "mean_interval_s": deltas.mean() if not deltas.empty else np.nan,
            "jitter_s": jitter,
            "min_interval_s": deltas.min() if not deltas.empty else np.nan,
            "max_interval_s": deltas.max() if not deltas.empty else np.nan,
        })

    summary_df = pd.DataFrame(summaries)
    print(summary_df.to_string(index=False))

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    summary_filename = f"mqtt_analysis_{timestamp}.csv"
    summary_df.to_csv(summary_filename, index=False)
    print(f"\n💾 Summary saved to {summary_filename}")

def run_mqtt_analysis():
    global stop_requested
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    client = mqtt.Client()
    client.on_message = on_message
    
    try:
        client.connect(BROKER_HOST)
        client.subscribe(TOPIC)
    except Exception as e:
        print(f"⚠️ Failed to connect to MQTT broker: {e}")
        return
    
    print(f"📡 Listening to '{TOPIC}' for up to {DURATION_S} seconds…")
    print("🔄 Press Ctrl+C at any time to stop and analyze data")
    
    client.loop_start()

    start_time = time.time()
    last_report = start_time
    
    try:
        while time.time() - start_time < DURATION_S and not stop_requested:
            elapsed = int(time.time() - start_time)
            remaining = DURATION_S - elapsed

            print(f"⏳ Running… {elapsed}s elapsed, {remaining}s remaining, {len(records)} messages received", end="\r")

            if elapsed % 60 == 0 and elapsed != 0 and time.time() - last_report > 1:
                print(f"\n🕒 {elapsed // 60} minute(s) elapsed… {len(records)} messages so far.")
                last_report = time.time()

            time.sleep(1)
    except Exception as e:
        print(f"\n⚠️ Error during data collection: {e}")
    finally:
        client.loop_stop()
        client.disconnect()
        
        if stop_requested:
            print("\n⏹️ Analysis stopped early by user.")
        else:
            print("\n⏹️ Done collecting messages.")
        
        analyze_data()

if __name__ == "__main__":
    run_mqtt_analysis()