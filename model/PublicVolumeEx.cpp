/*
 * Created by Spreadtrum
 */

#ifdef UMS
/* SPRD: add for UMS @{ */
#define BUF_SIZE 96

status_t PublicVolume::doShare(const std::string& massStorageFilePath) {
    mMassStorageFilePath = massStorageFilePath;

    std::string shareDevPath;
    VolumeManager *vm = VolumeManager::Instance();
    auto disk = vm->findDisk(getDiskId());
    if(++disk->mSharedCount > 1)
        return OK;
    // external physical SD card, share whole disk
    shareDevPath = disk->getDevPath();
    int res = OK;
    char file_content[BUF_SIZE];
    res = android::vold::ReadFile(mMassStorageFilePath, file_content, BUF_SIZE);
    if(res == OK && !strncmp(file_content, shareDevPath.c_str(), shareDevPath.length())) return OK;
    res = android::vold::WriteToFile(getId(), mMassStorageFilePath, shareDevPath, 0);
    // can't share this volume, this volume don't be shared state, need to reduce the sharecount
    if(res != OK) disk->mSharedCount--;
    return res;
}

status_t PublicVolume::doUnshare() {
    VolumeManager *vm = VolumeManager::Instance();
    auto disk = vm->findDisk(getDiskId());
    disk->mSharedCount--;
    if(std::string::npos == vm->mUsbFunction.find("mass_storage")) {
        LOG(INFO) << " doUnshare not in ums mode, don't write byte 0 to file";
        return OK;
    }
    if (mMassStorageFilePath.empty()) {
        LOG(WARNING) << "mass storage file path is empty";
        return -1;
    }
#ifdef UMS_K44
    // only reset and write 0 to usb file when disk destroy
    if(!disk->mIsDestroying) return OK;
    // only reset ums function when disk destroy
    if(android::vold::sSleepOnUnmount && android::vold::getSupportUms() && disk->mSharedCount < 1 && disk->mIsDestroying) {
        vm->ifResetUms(true);
    } else if(disk->mSharedCount >=1) {
        return OK;
    }
#endif
    int res = android::vold::WriteToFile(getId(), mMassStorageFilePath, std::string(), 0);
    // can't unshare this volume, volume still be shared state;
    if(res != OK) disk->mSharedCount++;
    return res;
}
#endif

status_t PublicVolume::doSetState(State state) {
    if (getLinkName().empty()) {
        LOG(WARNING) << "LinkName is empty, this is not a physical storage.";
    } else {
        char shutdown[PROPERTY_VALUE_MAX];
        property_get("sys.powerctl", shutdown, "");
        if (!strcmp(shutdown, "")) {
            property_set(StringPrintf("vold.%s.state", getLinkName().c_str()).c_str(), findState(state).c_str());
        }
    }
    return OK;
}
/* @} */

void PublicVolume::createSymlink(const std::string & stableName) {
    /* SPRD: add for storage, create link for fuse path @{ */
    if (!getLinkName().empty()) {
        LOG(VERBOSE) << "create link for fuse path, linkName=" << getLinkName();
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/default/%s", getLinkName().c_str()));
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/read/%s", getLinkName().c_str()));
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/write/%s", getLinkName().c_str()));
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/full/%s", getLinkName().c_str()));
        char shutdown[PROPERTY_VALUE_MAX];
        property_get("sys.powerctl", shutdown, "");
        if (!strcmp(shutdown, "")) {
            property_set(StringPrintf("vold.%s.path", getLinkName().c_str()).c_str(),
                StringPrintf("/storage/%s", stableName.c_str()).c_str());
        }
    }
    /* @} */
}

void PublicVolume::deleteSymlink() {
    /* SPRD: add for storage, delete link for fuse path @{
     *  */
    if (!getLinkName().empty()) {
        LOG(VERBOSE) << "delete link for fuse path, linkName=" << getLinkName();
        DeleteSymlink(StringPrintf("/mnt/runtime/default/%s", getLinkName().c_str()));
        DeleteSymlink(StringPrintf("/mnt/runtime/read/%s", getLinkName().c_str()));
        DeleteSymlink(StringPrintf("/mnt/runtime/write/%s", getLinkName().c_str()));
        DeleteSymlink(StringPrintf("/mnt/runtime/full/%s", getLinkName().c_str()));
    }
    /* @* } */
}


