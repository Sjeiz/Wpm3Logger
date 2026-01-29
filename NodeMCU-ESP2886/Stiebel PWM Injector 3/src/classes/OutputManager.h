#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include <Arduino.h>
class OutputManager {
public:
    void begin();
    void setDefrost();
    void setPostRun(int pwmPercent);
    void setNormal();
    void loop(const char* stateName);
private:
    const char* lastStateName = nullptr;
};
#endif // OUTPUTMANAGER_H
