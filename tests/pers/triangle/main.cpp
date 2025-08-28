#include "PersTriangleApp.h"

int main() {
    PersTriangleApp app;
    
    if (!app.initialize()) {
        return -1;
    }
    
    app.run();
    
    return 0;
}