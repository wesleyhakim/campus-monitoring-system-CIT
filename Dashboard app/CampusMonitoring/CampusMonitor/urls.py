from django.urls import path
from . import views

urlpatterns = [
    path("lantai/<int:location_id>/", views.lantai_view, name="lantai_view"),
    path("system_overview/", views.system_overview, name="system_overview"),
    path("", views.home, name="home"),
    path('lantai/<int:location_id>/<int:station_id>/', views.station_detail, name='station_details'),
]