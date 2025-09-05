#include "pers/utils/Mutex.h"

namespace pers {

// Static member definition for debug mode Mutex specialization
std::atomic<int> Mutex<true>::_globalLockId{0};

} // namespace pers