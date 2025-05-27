from django.db import models
import os
from django.conf import settings

def location_image_path(instance, filename):
    filename = f"{instance.location_name}.jpg"

    full_path = os.path.join(settings.MEDIA_ROOT, 'images', filename)

    if os.path.exists(full_path):
        os.remove(full_path)

    return os.path.join('images', filename)



class Location(models.Model):
    location_name = models.CharField(max_length=255, unique=True)
    image = models.ImageField(upload_to=location_image_path)
        
    def __str__(self):
        return self.location_name
    
    def delete(self, *args, **kwargs):
        if self.image:
            image_path = self.image.path
            if os.path.isfile(image_path):
                os.remove(image_path)
        super().delete(*args, **kwargs)


class Station(models.Model):
    STATUS_CHOICES = (
        ('online', 'Online'),
        ('offline', 'Offline'),
    )
    
    # Primary Key
    station_id = models.AutoField(primary_key=True)
    
    # Fields
    status = models.CharField(max_length=10, choices=STATUS_CHOICES)
    last_sent_time = models.DateTimeField()
    station_name = models.CharField(max_length=50, unique =True)
    station_display_name = models.CharField(max_length=50, null=True, blank=True)
    # Foreign Key
    location = models.ForeignKey(Location, on_delete=models.CASCADE, null=True, blank=True)

    x_coord = models.FloatField(null=True, blank=True)
    y_coord = models.FloatField(null=True, blank=True) 
    
    def __str__(self):
        return f"Station {self.station_name}, ID {self.station_id} - {self.get_status_display()}"


class Sensor(models.Model):
    # Primary Key
    sensor_id = models.AutoField(primary_key=True)
    
    # Fields
    sensor_name = models.CharField(max_length=255, unique =True)
    sensor_display_name = models.CharField(max_length=50, null=True, blank=True)
    unit = models.CharField(max_length=50,null=True, blank=True)
    description = models.TextField(blank=True)
    description_display = models.CharField(max_length=255, null=True, blank=True)
    
    def __str__(self):
        return self.sensor_name

class SensorRange(models.Model):
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE, related_name="ranges")
    label = models.CharField(max_length=255, null=True, blank=True)
    min_value = models.FloatField(null=True, blank=True)
    max_value = models.FloatField(null=True, blank=True)
    color_hex = models.CharField(max_length=6, null=True, blank=True)

    def __str__(self):
        return f"{self.sensor.sensor_name} - {self.label} ({self.min_value}-{self.max_value})"

class StationSensor(models.Model):
    # Primary Key
    station_sensor_id = models.AutoField(primary_key=True)
    
    # Foreign Keys
    station = models.ForeignKey(Station, on_delete=models.CASCADE)
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE)
    
    def __str__(self):
        return f"Station {self.station.station_name}, ID {self.station.station_id} - Sensor {self.sensor.sensor_name}, ID {self.sensor.sensor_id}"


class DataEntry(models.Model):
    # Primary Key
    data_entry_id = models.AutoField(primary_key=True)
    
    # Foreign Key
    station_sensor = models.ForeignKey(StationSensor, on_delete=models.CASCADE)
    
    # Fields
    value = models.DecimalField(max_digits=10, decimal_places=2, null=True)
    timestamp = models.DateTimeField()
    
    def __str__(self):
        return f"{self.station_sensor} | Value: {self.value} at {self.timestamp}"

