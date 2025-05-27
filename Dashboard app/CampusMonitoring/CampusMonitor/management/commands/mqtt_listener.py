import json
import paho.mqtt.client as mqtt
from django.core.management.base import BaseCommand
from CampusMonitor.models import Station, Sensor, StationSensor, DataEntry, SensorRange
from django.utils import timezone
from django.db import IntegrityError, OperationalError
import threading
import time
from django.db.models import Max

BROKER = "10.251.255.73"

MAIN_TOPIC = "stations/+/data"
PONG_TOPIC = "stations/+/pong"

temp_subscribed_topics = {}

PONG_TIMEOUT = 10
PING_FREQUENCY = 5
pong_received = {}

def check_station_status(client):
    try:
        while True:
            now = timezone.now()

            try:
                stations = Station.objects.all()
            except OperationalError as e:
                if 'database is locked' in str(e):
                    print("Database is locked while fetching stations.")
                    continue
                else:
                    raise        
            try:
                for station in stations:
                    topic_ping = f"stations/{station.station_name}/ping"
                    client.publish(topic_ping, "ping")
                    print('-'*40 + f' ping {station.station_name} ' + '-'*40)

                    last_pong = pong_received.get(station.station_name)
                    if not last_pong or (now - last_pong).total_seconds() > PONG_TIMEOUT:
                        new_status = 'offline'
                    else:
                        new_status = 'online'

                    if new_status != station.status:
                        station.status = new_status
                        try:
                            station.save(update_fields=['status'])
                        except OperationalError as e:
                            if 'database is locked' in str(e):
                                print("Database is locked while saving station status.")
                                continue
                            else:
                                raise
            except OperationalError as e:
                if 'database is locked' in str(e):
                    print("Database is locked while saving station status.")
                    continue
                else:
                    raise
            
            time.sleep(PING_FREQUENCY)
    except Exception as e:
        print(f"Unexpected error incheck_station_status(): {e}")

def insert_dataentry(station_name, payload_json):

    try:
        station = Station.objects.get(station_name=station_name)
    except Station.DoesNotExist:
        print(f"Station '{station_name}' not found!")
        return

    for sensor_name, value in payload_json.items():
        
        try:
            sensor = Sensor.objects.get(sensor_name=sensor_name)
        
        except Sensor.DoesNotExist:
            print(f"Sensor '{sensor_name}' not found!")
            continue
        
        
        try:
            station_sensor = StationSensor.objects.get(station=station, sensor=sensor)
        
        except StationSensor.DoesNotExist:
            print(f"StationSensor mapping for '{station_name}' and '{sensor_name}' not found!")
            continue

        DataEntry.objects.create(
            station_sensor=station_sensor,
            value=value if isinstance(value, (int, float)) else None,
            timestamp=timezone.now()
        )

        station.last_sent_time = timezone.now()

        station.save()

        print(f"Added DataEntry for {station_name} - {sensor_name}: {value}")

def on_message(client, userdata, msg):
    try:
        global temp_subscribed_topics

        topic = msg.topic
        parts = topic.split("/")

        if len(parts) < 3:
            print(f"Received message from unexpected topic format: {topic}")
            return

        station_name = parts[1]
        if topic != f"stations/{station_name}/pong":
            try:
                payload_str = msg.payload.decode("utf-8")
                payload_json = json.loads(payload_str)
            
            except (UnicodeDecodeError, json.JSONDecodeError) as e:
                print(f"Error decoding JSON from {station_name}: {e}")
                return 

            print(f"From {station_name} topic {parts[2]}: {payload_json}")

            if topic == f"stations/{station_name}/data":

                if payload_str.strip() == "1":
                    temp_topic = f"stations/{station_name}/information"
                    
                    if temp_topic not in temp_subscribed_topics:
                        print(f"Temporarily subscribing to {temp_topic}")
                        client.subscribe(temp_topic)
                        temp_subscribed_topics[temp_topic] = True
                else:
                    insert_dataentry(station_name, payload_json)

            elif topic == f"stations/{station_name}/information":
                try:
                    station, created_station = Station.objects.get_or_create(
                        station_name=station_name,
                        defaults={
                            "status": "online",
                            "last_sent_time": timezone.now(),
                            "location": None
                        }
                    )
                except IntegrityError:
                    station = Station.objects.get(station_name=station_name)
                    created_station = False
                    print(f"Station with the name {station_name} already exists!")

                for sensor_name, (unit, description, ranges) in payload_json.items():
                    try:
                        sensor, created_sensor = Sensor.objects.get_or_create(
                            sensor_name=sensor_name,
                            defaults={
                                "unit": unit,
                                "description": description
                            }
                        )

                        if not created_sensor:
                            sensor.unit = unit
                            sensor.description = description
                            sensor.save()

                    except IntegrityError:
                        sensor = Sensor.objects.get(sensor_name=sensor_name)
                        created_sensor = False
                        print(f"Sensor with the name {sensor_name} already exists!")

                    station_sensor, created_station_sensor = StationSensor.objects.get_or_create(
                        station=station,
                        sensor=sensor
                    )
                    print(f"ini ranges {ranges[0][0]} {type(ranges[0][0])}")
                    if ranges[0][0] != "none":
                        existing_ranges = {r.label: r for r in sensor.ranges.all()}

                        updated_labels = set()

                        for label, min_val, max_val, color in ranges:
                            updated_labels.add(label)
                            if label in existing_ranges:
                                range_obj = existing_ranges[label]
                                range_obj.min_value = min_val
                                range_obj.max_value = max_val
                                range_obj.color_hex = color
                                range_obj.save()
                            else:
                                SensorRange.objects.create(
                                    sensor=sensor,
                                    label=label,
                                    min_value=min_val,
                                    max_value=max_val,
                                    color_hex=color
                                )

                        for label in existing_ranges:
                            if label not in updated_labels:
                                existing_ranges[label].delete()
                        print(f"Inserted {sensor_name} ranges for")
                    else:
                        SensorRange.objects.create(
                                    sensor=sensor,
                                    label=None,
                                    min_value=None,
                                    max_value=None,
                                    color_hex=None
                                )
                        
                print(f"Inserted station: {created_station}")

                if temp_subscribed_topics.get(topic, False):
                    print(f"Received one message from {topic}, unsubscribing...")
                    client.unsubscribe(topic)
                    del temp_subscribed_topics[topic]
                
        else:
            pong_received[station_name] = timezone.now()
            print(f"[PONG] Received pong from {station_name}")

    except Exception as e:
        print(f"Unexpected error in on_message(): {e}")

def on_disconnect(client, userdata, rc):
    print(f"Disconnected with result code {rc}")
    if rc != 0:
        print("Unexpected disconnection. Trying to reconnect...")
        while True:
            try:
                client.reconnect()
                client.subscribe(MAIN_TOPIC)
                client.subscribe(PONG_TOPIC)
                print("Reconnected to MQTT broker")
                break
            except Exception as e:
                print(f"Reconnect failed: {e}")
                time.sleep(5)


class Command(BaseCommand):
    help = "Runs the MQTT subscriber"

    def handle(self, *args, **kwargs):
        try:
            client = mqtt.Client()
            client.on_message = on_message
            client.on_disconnect = on_disconnect

            client.reconnect_delay_set(min_delay=1, max_delay=30)

            client.connect(BROKER, 1883, keepalive=300)
            client.subscribe(MAIN_TOPIC)
            client.subscribe(PONG_TOPIC)

            print("Listening for messages...")

            threading.Thread(target=check_station_status, args=(client,), daemon=True).start()


            client.loop_forever()

        except Exception as e:
            print(f"Fatal error in MQTT listener: {e}")