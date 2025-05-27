from django.shortcuts import render, get_object_or_404
from .models import Location, Station, Sensor, StationSensor, DataEntry
from django.utils import timezone
import json
from django.utils.text import slugify
from django.http import HttpResponse
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt

def _collect_locations_data(request):
    locations = Location.objects.all()
    locs_data = []

    for location in locations:
        total_stations = Station.objects.filter(location=location).count()
        online_stations = Station.objects.filter(location=location, status="online").count()
        
        locs_data.append({
            "location_id": location.id,
            "name": location.location_name,
            "total_stations": total_stations,
            "online_stations": online_stations,
        })
    return locs_data    


def home(request):
    locations_data = _collect_locations_data(request)

    if request.headers.get('X-Requested-With') == 'XMLHttpRequest':
        return JsonResponse({
            'locations': locations_data,
        })

    return render(request, "home.html", {"locations": locations_data})

def _collect_system_overview(request):
    locations = Location.objects.all()
    
    context_data = []
    
    for location in locations:
        location_data = {
            'name': location.location_name,
            'stations': []
        }
        
        stations = Station.objects.filter(location=location)


        for station in stations:
            station_name = station.station_display_name if station.station_display_name else station.station_name
            station_data = {
                'id': station.station_id,
                'name': station_name,
                'last_sent': station.last_sent_time if station.last_sent_time else None,
                'status':station.status,
                'sensors': []
            }
            
            station_sensors = StationSensor.objects.filter(station=station)
            
            for station_sensor in station_sensors:
                sensor = station_sensor.sensor
                
                try:
                    latest_data = DataEntry.objects.filter(
                        station_sensor=station_sensor
                    ).latest('timestamp')
                    value = latest_data.value
                except DataEntry.DoesNotExist:
                    value = "No data"
                
                sensor_data = {
                    'sensor_id': sensor.sensor_id,
                    'name': sensor.description_display if sensor.description_display else sensor.description,
                    'value': value,
                    'unit': sensor.unit
                }
                station_data['sensors'].append(sensor_data)
            
            location_data['stations'].append(station_data)
        
        context_data.append(location_data)
    
    return context_data

def system_overview(request):
    
    context_data = _collect_system_overview(request)

    context = {
        'locations': context_data
    }

    if request.headers.get('X-Requested-With') == 'XMLHttpRequest':
        return JsonResponse(context)

    return render(request, 'floors/system_overview.html', context)

def _collect_station_data(request, loc_id):
    stations_data = Station.objects.filter(location_id=loc_id)
    
    stations = []
    
    for station in stations_data:
        station_sensors = StationSensor.objects.filter(station_id=station.station_id)
        
        readings = []
        unranged_readings = []
        
        for station_sensor in station_sensors:
            sensor = station_sensor.sensor
            
            try:
                latest_data = DataEntry.objects.filter(
                    station_sensor_id=station_sensor.station_sensor_id
                ).order_by('-timestamp').first()
                
                if latest_data:

                    timestamp = latest_data.timestamp

                    if latest_data.value is not None:
                        try:
                            value = float(latest_data.value)
                        except ValueError:
                            print("Invalid value in database")
                            value = None
                    else:
                        value = latest_data.value

                else:
                    print("Database empty")
                    value = None
                    timestamp = timezone.now()
            
            except Exception:
                print("Error in getting latest_data entry function")
                value = None
                timestamp = timezone.now()

            if value is not None:
                if sensor.ranges.exclude(label__isnull=True).exists():
                    matching_range = None
                    for r in sensor.ranges.all():
                        print(f"min value {r.min_value}, {value}, max value {r.max_value}")
                        if r.min_value <= value <= r.max_value:
                            matching_range = r
                            break
                    
                    color_hex = f"#{matching_range.color_hex}" if matching_range else "#FF0000"
                    status_label = matching_range.label if matching_range else "Out of range"

                    min_val = min([r.min_value for r in sensor.ranges.all()] or [0])
                    max_val = max([r.max_value for r in sensor.ranges.all()] or [100])
                    rotation = 180 * (value - min_val) / (max_val - min_val) if (max_val - min_val) != 0 else 0

                    readings.append({
                        'sensor_id': sensor.sensor_id,
                        'sensor_name': sensor.sensor_display_name if sensor.sensor_display_name else sensor.sensor_name,
                        'description': sensor.description_display if sensor.description_display else sensor.description,
                        'value': value,
                        'color_hex': color_hex,
                        'rotation': rotation,
                        'unit': sensor.unit,
                        'status_label': status_label,
                        'min_val': min_val,
                        'max_val': max_val,
                        'timestamp': timestamp
                    })
                else:
                    zero_value_entry = DataEntry.objects.filter(station_sensor_id=station_sensor.station_sensor_id,value=0).order_by('-timestamp').first()
                    start_timestamp = zero_value_entry.timestamp if zero_value_entry else timestamp
                    unranged_readings.append({
                        'sensor_id': sensor.sensor_id,
                        'sensor_name': sensor.sensor_display_name if sensor.sensor_display_name else sensor.sensor_name,
                        'description': sensor.description_display if sensor.description_display else sensor.description,
                        'value': value,
                        'unit': sensor.unit,
                        'newest_timestamp': timestamp,
                        'start_timestamp': start_timestamp
                    })
            else:
                if sensor.ranges.exclude(label__isnull=True).exists():
                    readings.append({
                        'sensor_id': sensor.sensor_id,
                        'sensor_name': sensor.sensor_display_name if sensor.sensor_display_name else sensor.sensor_name,
                        'description': sensor.description_display if sensor.description_display else sensor.description,
                        'value': value,
                        'color_hex': "#FF0000",
                        'rotation': 0,
                        'unit': sensor.unit,
                        'status_label': "Disconnected",
                        'min_val': 0,
                        'max_val': 100,
                        'timestamp': timestamp
                    })
                else:
                    unranged_readings.append({
                        'sensor_id': sensor.sensor_id,
                        'sensor_name': sensor.sensor_display_name if sensor.sensor_display_name else sensor.sensor_name,
                        'description': sensor.description_display if sensor.description_display else sensor.description,
                        'value': value,
                        'unit': sensor.unit,
                        'newest_timestamp': timestamp,
                        'start_timestamp': timestamp
                    })
        
        coordinates = get_station_coordinates(station.station_name)
        
        station_name = station.station_display_name if station.station_display_name else station.station_name
        
        stations.append({
            'id': station.station_id,
            'name': station_name,
            'status': station.status,
            'last_sent_time': station.last_sent_time,
            'readings': readings,
            'unranged_readings': unranged_readings,
            'coordinates': coordinates
        })

    return stations

def lantai_view(request, location_id=None):
    """
    Display the floor view with all stations and their sensor readings
    for a specific location (floor).
    """
    all_locations = Location.objects.all()
    
    if location_id is None and all_locations.exists():
        location_id = all_locations.first().location_id
    
    current_location = get_object_or_404(Location, pk=location_id)

    stations = _collect_station_data(request, location_id)

    print(f"this is current location name {current_location.location_name}")
    floor_plan_url = current_location.image.url
    
    if request.headers.get('X-Requested-With') == 'XMLHttpRequest':
        return JsonResponse({
            'stations': stations,
            'floor_plan_url': floor_plan_url,
        })

    context = {
        'all_locations': all_locations,
        'current_location': current_location,
        'stations': stations,
        'floor_plan_url': floor_plan_url
    }

    return render(request, 'floors/lantai_view.html', context)

def get_station_coordinates(station_name):
    """
    Get coordinates for a station from the database.
    Returns {"x": x_coord, "y": y_coord}, or defaults to {"x": 0, "y": 0} if not found.
    """
    try:
        station = Station.objects.get(station_name=station_name)
        return {
            "x": station.x_coord,
            "y": station.y_coord
        }
    except Station.DoesNotExist:
        return {"x": 0, "y": 0}


def _collect_station_detail_data(location_id, station_id):
    """
    Build a fully‑serialisable dictionary describing *one* station and its
    sensors, plus per‑sensor time‑series for charts.
    Returns: (station_dict, chart_data_json_str)
    """

    current_location = get_object_or_404(Location, pk=location_id)
    
    location_stations = Station.objects.filter(location_id=location_id)
    print(f"Number of stations for this location: {location_stations.count()}")

    if station_id is None and location_stations.exists():
        station_id = location_stations.first().station_id
        print(f"No station ID provided, using first station: {station_id}")

    current_station = get_object_or_404(Station, station_id=station_id, location_id=location_id)

    station_sensors = StationSensor.objects.filter(station=current_station)


    station_data = {
        'name': current_station.station_display_name if current_station.station_display_name else current_station.station_name,
        'sensors': []
    }
    
    for station_sensor in station_sensors:
        sensor = station_sensor.sensor
        
        try:
            latest_data = DataEntry.objects.filter(
                station_sensor=station_sensor
            ).latest('timestamp')
            value = latest_data.value
            timestamp = latest_data.timestamp
        except DataEntry.DoesNotExist:
            value = "No data"
            timestamp = None
        
        sensor_data = {
            'sensor_id': sensor.sensor_id,
            'name': sensor.description_display if sensor.description_display else sensor.description,
            'value': value,
            'unit': sensor.unit,
            'timestamp': timestamp
        }
        
        station_data['sensors'].append(sensor_data)

    chart_data = {}
    
    for station_sensor in station_sensors:
        sensor = station_sensor.sensor

        recent_entries = (
            DataEntry.objects
            .filter(station_sensor=station_sensor)
            .order_by('timestamp')
            .values('timestamp', 'value')
        )

        recent_entries = list(recent_entries)[-5000:]

        timestamps, values = [], []

        for e in recent_entries:
            timestamps.append(e['timestamp'].strftime('%H:%M:%S'))

            try:
                values.append(float(e['value']))
            except (TypeError, ValueError):
                values.append(None)

        chart_data[sensor.sensor_id] = {
            'timestamps': timestamps,
            'values':     values,
            'unit':       sensor.unit,
            'name':       sensor.description_display if sensor.description_display else sensor.description,
        }

    chart_data_json_str = json.dumps(chart_data, default=str)

    return current_location, current_station, station_data, chart_data_json_str

def station_detail(request, location_id, station_id):
    """
    Detail page (HTML) or JSON (AJAX) for a single station.
    """

    all_locations = Location.objects.all()
    location_stations = Station.objects.filter(location_id=location_id)

    (
        current_location,
        current_station,
        station_data,
        chart_data_json_str,
    ) = _collect_station_detail_data(location_id, station_id)

    if request.headers.get("X-Requested-With") == "XMLHttpRequest":
        return JsonResponse(
            {
                "station": station_data,
                "chart_data_json": json.loads(chart_data_json_str),
            }
        )

    context = {
        "all_locations": all_locations,
        "current_location": current_location,
        "location_stations": location_stations,
        "current_station": current_station,
        "station": station_data,
        "chart_data_json": chart_data_json_str,
    }
    return render(request, "floors/lantai_details.html", context)