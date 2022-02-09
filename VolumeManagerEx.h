/*
 * Created by Spreadst
 */

#ifndef VOLUME_MANAGER_EX_H
#define VOLUME_MANAGER_EX_H

#ifdef UMS
/* SPRD: add for UMS @{ */
int                    mUmsSharePrepareCount;
int                    mUmsSharedCount;
int                    mUmsShareIndex;

std::vector<std::string> mUMSFilePaths;
std::string            mSupportLunsFilePath;

int prepareShare(int count);
int shareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol);
int unshareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol);
int unshareOver();
/* @} */
std::string            mUsbFunction;
bool                   mUsbDataUnlocked;
std::string            mUsbState;
bool                   mPlugReset;
void handleUsbStateEvent(NetlinkEvent *evt);
std::list<std::shared_ptr<android::vold::VolumeBase>> findSdVolumes(android::vold::VolumeBase::State state);
void disableUms();
void enableUms();
void ifResetUms(bool needWait = false);
int waitForState(std::string usbState);
void getUsbProp();
#endif
#endif
