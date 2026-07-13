#include <services/SensorManager.h>
#include <utility>

SensorManager::SensorManager() {}

bool SensorManager::begin() {
  // The HX711 is owned by ScaleManager. A reader is injected so networking
  // never creates a second driver for the same physical sensor.
  return static_cast<bool>(_reader);
}

float SensorManager::readWeight() {
  if (!_reader) return 0.0f;
  const float weight = _reader();
  if (_cb) _cb(weight);
  return weight;
}
