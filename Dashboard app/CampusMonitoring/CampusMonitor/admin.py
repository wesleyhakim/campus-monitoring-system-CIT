from django.contrib import admin
from django.utils.html import mark_safe
from .models import Location, Station, Sensor, StationSensor, DataEntry, SensorRange
import csv
from django.http import HttpResponse

@admin.action(description="Export selected to CSV")
def export_as_csv(modeladmin, request, queryset):
    response = HttpResponse(content_type='text/csv')
    response['Content-Disposition'] = f'attachment; filename={modeladmin.model.__name__}.csv'
    
    writer = csv.writer(response)
    fields = [field.name for field in modeladmin.model._meta.fields]
    writer.writerow(fields)
    
    for obj in queryset:
        writer.writerow([getattr(obj, field) for field in fields])

    return response

@admin.register(Location)
class LocationAdmin(admin.ModelAdmin):
    list_display = ('id', 'location_name')
    search_fields = ('location_name',)

@admin.register(Station)
class StationAdmin(admin.ModelAdmin):
    list_display = ('station_id', 'station_name', 'station_display_name', 'status', 'last_sent_time', 'location', 'x_coord', 'y_coord')
    list_filter = ('status', 'location')
    search_fields = ('station_name',)
    readonly_fields = ('image_preview',)

    class Media:
        js = ('js/click_coords.js',)

    @admin.display(description='Location Image')
    def image_preview(self, obj):
        if obj.location and obj.location.image:
            return mark_safe(f'<img id="clickable-image" src="{obj.location.image.url}" style="max-width: 500px;" />')
        return "No image uploaded for this location"

class SensorRangeInline(admin.TabularInline):
    model = SensorRange
    extra = 1

@admin.register(Sensor)
class SensorAdmin(admin.ModelAdmin):
    list_display = ('sensor_id', 'sensor_name', 'sensor_display_name', 'unit', 'description', 'description_display')
    search_fields = ('sensor_name', 'unit', 'description')
    inlines = [SensorRangeInline]

@admin.register(SensorRange)
class SensorRangeAdmin(admin.ModelAdmin):
    list_display = ('sensor', 'label', 'min_value', 'max_value', 'color_hex')
    list_filter = ('sensor',)
    search_fields = ('label', 'sensor__sensor_name')


@admin.register(StationSensor)
class StationSensorAdmin(admin.ModelAdmin):
    list_display = ('station_sensor_id', 'station', 'sensor')
    list_filter = ('station', 'sensor')
    search_fields = (
        'station__station_name',
        'sensor__sensor_name',
    )


@admin.register(DataEntry)
class DataEntryAdmin(admin.ModelAdmin):
    list_display = ('data_entry_id', 'station_sensor', 'value', 'timestamp')
    list_filter = ('station_sensor__station', 'station_sensor__sensor', 'timestamp')
    search_fields = (
        'station_sensor__station__station_name',
        'station_sensor__sensor__sensor_name',
    )
    actions = [export_as_csv]

