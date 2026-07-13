#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <functional>
#include <utility>

class SensorManager {
public:
  using WeightCallback = std::function<void(float)>;
  using WeightReader = std::function<float()>;
  SensorManager();
  bool begin();
  float readWeight();
  void setReader(WeightReader reader) { _reader = std::move(reader); }
  void setOnWeight(WeightCallback cb) { _cb = cb; }
private:
  WeightCallback _cb;
  WeightReader _reader;
};

#endif // SENSORMANAGER_H
