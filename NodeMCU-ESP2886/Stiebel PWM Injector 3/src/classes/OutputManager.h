#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include <Arduino.h>
class OutputManager {
public:
    void begin();
    void setDefrost();
    void setPostRun(int pwmPercent);
    void setNormal();
};
#endif // OUTPUTMANAGER_H
