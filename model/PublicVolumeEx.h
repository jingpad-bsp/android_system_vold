/*
 * Created by Spreadst
 */

#ifndef PUBLIC_VOLUME_EX_H
#define PUBLIC_VOLUME_EX_H

/* SPRD: add for UMS @{ */
#ifdef UMS
std::string mMassStorageFilePath;
status_t doShare(const std::string& massStorageFilePath) override;
status_t doUnshare() override;
#endif

status_t doSetState(State state) override;

void createSymlink(const std::string & stableName);
void deleteSymlink(void);


#endif
