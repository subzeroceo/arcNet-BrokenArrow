```cpp
#include "FileSystem.h"

bool anFileSystem::RemoveExplicitDir(const char* OSPath, bool nonEmpty, bool recursive) {

  // Check if OSPath is empty
  if ( !OSPath[0]) {
    return false; 
  }

  // Build full path 
  anString fullPath = BuildOSPath(fs_savepath.GetString(), engineFolder, OSPath);

  // Check if recursive delete
  if ( !recursive) {

    // Try to remove directory (will fail if non-empty)
    if (remove(fullPath.c_str()) == 0) {
      return true;
    } else {
      return false;
    }

  } else {

    // Recursively delete directory
    return RemoveDir(fullPath.c_str());

  }

}
```

