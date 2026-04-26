"""
PlatformIO Pre-Build Script
Generates version info and validates configuration.
"""

import os
from datetime import datetime

Import("env")

# Generate version header
version_h = f"""
#ifndef AUTO_VERSION_H
#define AUTO_VERSION_H
#define BUILD_TIMESTAMP "{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
#define BUILD_GIT_HASH "unknown"
#endif
"""

version_path = os.path.join(env.subst("$PROJECT_DIR"), "include", "auto_version.h")
with open(version_path, "w") as f:
    f.write(version_h)

print("[Pre-build] Generated auto_version.h")
