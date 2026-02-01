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
    int getCurrentPwmPercent() const { return currentPwmPercent; }
private:
    const char* lastStateName = nullptr;
    int currentPwmPercent = 0;
};
#endif // OUTPUTMANAGER_H
