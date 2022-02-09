/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Vold"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <cutils/properties.h>
#include <vector>
#include <string>
#include <sys/mount.h>
#include <cutils/log.h>
#include <selinux/selinux.h>
#include <logwrap/logwrap.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <linux/kdev_t.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "Ntfs.h"
#include "Utils.h"
#include "VoldUtil.h"

using android::base::StringPrintf;


namespace android {
namespace vold {
namespace ntfs {


static const char* kFsckPath = "/system/bin/ntfsfix";
static const char* kMountPath = "/system/bin/ntfs3g";
static const char* kMkfsPath = "/system/bin/mkntfs";


bool IsSupported() {
    bool ret = access(kFsckPath, X_OK) == 0
                && access(kMountPath, X_OK) == 0;
    LOG(ERROR) <<"ntfs support = " << ret;
    return ret;
}

status_t Check(const std::string& source) {
    std::vector<std::string> cmd;
    cmd.push_back(kFsckPath);
    cmd.push_back("-b");
    cmd.push_back(source);

    //Ntfs devices are currently always untrusted
    LOG(ERROR) <<"ntfs check begin";
    int rc = ForkExecvp(cmd);
    LOG(ERROR) <<"ntfs check end OK=" << rc;
    return rc;
}


status_t Mount(const std::string& source, const std::string& target, bool ro,
        bool remount, bool executable, int ownerUid, int ownerGid, int permMask,
        bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];

    const char* c_source = source.c_str();
    const char* c_target = target.c_str();

    flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC | MS_NOATIME;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);


    SLOGI("ntfs::Mount");
    snprintf(mountData, sizeof(mountData),
            "-o uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);
    std::vector<std::string> cmd;
    cmd.push_back(kMountPath);
    cmd.push_back(c_source);
    cmd.push_back(c_target);
    rc = ForkExecvp(cmd);

    if (rc == 0 && createLost) {
        char *lost_path;
        asprintf(&lost_path, "%s/LOST.DIR", c_target);
        if (access(lost_path, F_OK)) {
            /*
             * Create a LOST.DIR in the root so we have somewhere to put
             * lost cluster chains (fsck_msdos doesn't currently do this)
             */
            if (mkdir(lost_path, 0755)) {
                SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
            }
        }
        free(lost_path);
    }

    SLOGI("ntfs::Mount=%d", rc);

    return rc;
}

status_t Format(const std::string& source, bool wipe) {
    std::vector<std::string> cmd;
    cmd.push_back(kMkfsPath);
    if (wipe)
        cmd.push_back("-f");
        cmd.push_back(source);

     return  ForkExecvp(cmd);
}

}  // namespace ntfs
}  // namespace vold
}  // namespace android

