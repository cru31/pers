#include "pers/utils/Mutex.h"

namespace pers {

std::atomic<int> Mutex::_globalLockId{0};

} // namespace pers