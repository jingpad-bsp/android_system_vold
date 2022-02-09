/*
 * Created by Spreadtrum
 */

#ifndef VOLUME_BASE_EX_H
#define VOLUME_BASE_EX_H

/* SPRD: link name for mount path */
std::string mLinkname;

#ifdef UMS
/* SPRD: add for UMS @{ */
status_t share(const std::string& massStorageFilePath);
status_t unshare();
virtual status_t doShare(const std::string& massStorageFilePath);
virtual status_t doUnshare();
bool mHasMounted = false;
State mBeforeShareState = State::kUnmountable;
/* @} */
#endif

std::string findState(State state);
virtual status_t doSetState(State state);

/* SPRD: get link name for mount path */
const std::string& getLinkName() { return mLinkname; }
/* SPRD: set link name for mount path */
status_t setLinkName(const std::string& linkName);

#endif
