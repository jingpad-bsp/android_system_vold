/*
 * Created by Spreadtrum
 */


#ifdef UMS
int VolumeManager::shareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol) {
#ifndef UMS_K44
    auto massStorageFilePath = MASS_STORAGE_FILE_PATH_SYSFS;
#else
    auto massStorageFilePath = MASS_STORAGE_FILE_PATH_CONFIGFS;
#endif
    LOG(INFO) << "massStorageFilePath = " << massStorageFilePath;
    int res = vol->share(massStorageFilePath);
    if (res == android::OK) {
        mUmsSharedCount ++;
    }
    return res;
}

int VolumeManager::unshareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol) {
    int res = vol->unshare();
    if (res == android::OK) {
        mUmsSharedCount --;
    }
    return res;
}
/* @} */

void VolumeManager::handleUsbStateEvent(NetlinkEvent *evt) {
    std::lock_guard<std::mutex> lock(mLock);
    std::string usbState(evt->findParam("USB_STATE")?evt->findParam("USB_STATE"):"");
    mUsbState = usbState;
    getUsbProp();
    SLOGD("handleUsbStateEvent usbstate[%s] plugReset[%d]", usbState.c_str(), mPlugReset);
        if(usbState == "DISCONNECTED") {
        if(std::string::npos == mUsbFunction.find(USB_FUNCTION_MASS_STORAGE)) {
            // unshareVolume public volume state shared when switch mode
            if(mPlugReset && USB_FUNCTION_NONE == mUsbFunction)  return;
            disableUms();
        } else if(!mPlugReset) {
            // user don't switch mode, but pc reconfig, need to disable ums
            disableUms();
        }
    } else if (usbState == "CONFIGURED" && std::string::npos != mUsbFunction.find(USB_FUNCTION_MASS_STORAGE)) {
        mPlugReset = false;
        enableUms();
    }
}

std::list<std::shared_ptr<android::vold::VolumeBase>> VolumeManager::findSdVolumes(android::vold::VolumeBase::State state) {
    std::list<std::shared_ptr<android::vold::VolumeBase>> sdVols;
    for (const auto& disk : mDisks) {
        if(disk->getFlags() & android::vold::Disk::Flags::kSd) {
            SLOGD("findSdVolumes disk::SharedCount %d", disk->mSharedCount);
            std::list<std::shared_ptr<android::vold::VolumeBase>> vols = disk->findVolumes(state);
            for (const auto& vol : vols) {
                if (vol != nullptr && (vol->getType() == android::vold::VolumeBase::Type::kPublic)
                    && disk->mVolumeHasMounted) {
                    sdVols.push_back(vol);
                }
            }
        }
    }
    return sdVols;
}

void VolumeManager::disableUms() {
    std::list<std::shared_ptr<android::vold::VolumeBase>> sharedVols
        = findSdVolumes(android::vold::VolumeBase::State::kShared);
    SLOGD("disableUms: sharedVols[%d]", (int)sharedVols.size());
    for (const auto& vol : sharedVols) {
        unshareVolume(vol);
        if(vol->mBeforeShareState == android::vold::VolumeBase::State::kMounted || !vol->mHasMounted) {
            vol->mount();
        }
        else
            vol->setState(android::vold::VolumeBase::State::kUnmounted);
    }
}

void VolumeManager::enableUms() {
    std::list<std::shared_ptr<android::vold::VolumeBase>> mountedVols = findSdVolumes(android::vold::VolumeBase::State::kMounted);
    std::list<std::shared_ptr<android::vold::VolumeBase>> unmountedVols = findSdVolumes(android::vold::VolumeBase::State::kUnmounted);
    SLOGD("enableUms: mountedVols[%d] unmountedVols[%d]", (int)mountedVols.size(), (int)unmountedVols.size());
    mountedVols.merge(unmountedVols);
    for (const auto& vol : mountedVols) {
        vol->mBeforeShareState = vol->getState();
        if(vol->mBeforeShareState == android::vold::VolumeBase::State::kMounted)
            vol->unmount();
        shareVolume(vol);
    }
}

void VolumeManager::ifResetUms(bool needWait) {
    getUsbProp();
    if(std::string::npos != mUsbFunction.find(USB_FUNCTION_MASS_STORAGE) && !mPlugReset) {
        SLOGD("reset ums function");
        property_set(USB_FUNCTION_CONFIG, USB_FUNCTION_NONE);
        if(needWait && waitForState(USB_FUNCTION_NONE)) {
            SLOGE("wait for sys.usb.state none timeout");
        }
        property_set(USB_FUNCTION_CONFIG, mUsbFunction.c_str());
        SLOGD("reset ums function end");
        mPlugReset = true;
    }
}

int VolumeManager::waitForState(std::string usbState) {
    int retry_times = 50;
    int retry = 0;
    char usb_prop_state[PROPERTY_VALUE_MAX];
    SLOGD("waitForState %s to be %s", USB_STATE, usbState.c_str());
    while(retry++ < retry_times) {
        property_get(USB_STATE, usb_prop_state, "");
        if(strcmp(usb_prop_state, usbState.c_str()))
            usleep(20000);
        else {
            return android::OK;
        }
    }
    return android::TIMED_OUT;
}

void VolumeManager::getUsbProp() {
    char usb_function[PROPERTY_VALUE_MAX];
    property_get(USB_FUNCTION_CONFIG, usb_function, "");
    mUsbFunction = usb_function;
    SLOGD("getUsbProp usb_function[%s]", mUsbFunction.c_str());
}
/* @} */
#endif