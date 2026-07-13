#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller {
public:
  virtual ~Controller() = default;
  virtual bool begin() = 0;
  virtual void loop() = 0;
};

#endif // CONTROLLER_H
