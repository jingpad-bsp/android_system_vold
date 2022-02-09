/*
 * Created by Spreadst
 */

/*  write string to file  */

#ifdef UMS
#include <cutils/properties.h>
status_t ReadFile(const std::string& file, char* buf, int buf_size) {
    int fd;
    if ((fd = open(file.c_str(), O_RDONLY)) < 0) {
        PLOG(ERROR) << StringPrintf(" unable to open file (%s)", file.c_str());
        return -EIO;
    }
    int readRes = OK;
    readRes = read(fd, buf, buf_size);
    LOG(VERBOSE) << StringPrintf(" read file(%s) content (%s)", file.c_str(), buf);
    if(readRes < 0) {
        PLOG(ERROR) << StringPrintf(" unable to read file (%s)", file.c_str());
        close(fd);
        return -EIO;
    }
    close(fd);
    return OK;
}

bool getSupportUms() {
    return false;
    char support_ums[PROPERTY_VALUE_MAX];
    property_get("persist.sys.usb.support_ums", support_ums, "");
    if(!strcmp(support_ums, "true"))
        return true;
    return false;
}
#endif

status_t WriteToFile(const std::string& preMsg, const std::string& file, const std::string& str, const char byte) {
    int fd;

    if ((fd = open(file.c_str(), O_WRONLY)) < 0) {
        PLOG(ERROR) << preMsg << StringPrintf(" unable to open file (%s)", file.c_str());
        return -EIO;
    }

    int writeRes = OK;
    if (str.empty()) {
        LOG(VERBOSE) << StringPrintf(" write byte num %d to file \'%s\'", byte, file.c_str());
        writeRes = write(fd, &byte, 1);
    } else {
        LOG(VERBOSE) << StringPrintf(" write string \'%s\' to file \'%s\'", str.c_str(), file.c_str());
        writeRes = write(fd, str.c_str(), str.length());
    }
    if (writeRes < 0) {
        PLOG(ERROR) << preMsg << StringPrintf(" unable to write file (%s)", file.c_str());
        close(fd);
        return -EIO;
    }

    close(fd);
    return OK;
}
/* @} */

status_t ForceUnmount(const std::string& path) {
    const char* cpath = path.c_str();
    LOG(ERROR)<<"Begin umount2" << path;
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        LOG(ERROR)<<"umount2 ok";
        return OK;
    }
    /* SPRD: replace storage path to /storage @{ */
    LOG(WARNING) << "Failed to unmount " << path;

    std::string storage_path = path;
    std::string path_prefix = "/mnt/runtime/default";
    std::size_t pos = storage_path.find(path_prefix);
    LOG(WARNING) << "storage_path1= " + storage_path << ", pos=" << pos;
    if (pos != std::string::npos) {
        storage_path = storage_path.replace(pos, path_prefix.length(), "/storage");
    }
    path_prefix = "/mnt/runtime/write";
    pos = storage_path.find(path_prefix);
    LOG(WARNING) << "storage_path2= " + storage_path << ", pos=" << pos;
    if (pos != std::string::npos) {
        storage_path = storage_path.replace(pos, path_prefix.length(), "/storage");
    }

    const char* cpath2 = storage_path.c_str();
    /* @} */
    // Apps might still be handling eject request, so wait before
    // we start sending signals
    /* SPRD: replace storage path to /storage and add for unmount sdcard performance @{
     */
    if (sSleepOnUnmount) sleep(1);
    KillProcessesWithOpenFiles(cpath2, SIGINT);
    if (sSleepOnUnmount) sleep(1);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGINT. Failed to unmount " << path;

    KillProcessesWithOpenFiles(cpath2, SIGTERM);
    if (sSleepOnUnmount) sleep(1);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGTERM. Failed to unmount " << path;

    KillProcessesWithOpenFiles(cpath2, SIGKILL);
    if (sSleepOnUnmount) sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After SIGKILL. Failed to unmount " << path;

    KillProcessesWithOpenFiles(cpath2, SIGKILL);
    if (sSleepOnUnmount) sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    LOG(WARNING) << "After Second SIGKILL. Failed to unmount " << path;

    /* SPRD: Lazy umount @{ */
    if (sSleepOnUnmount) sleep(7);
    if (!umount2(cpath, UMOUNT_NOFOLLOW|MNT_DETACH) || errno == EINVAL || errno == ENOENT) {
        PLOG(WARNING) << "Success lazy unmount " << path;
        return OK;
    }
    LOG(ERROR) << "Failed to Lazy unmount " << path;
    /* @} */
    return -errno;
}

/* SPRD: add for storage @{ */
/*  create symlink  */
status_t CreateSymlink(const std::string& source, const std::string& target) {
    status_t res = 0;
    if(!remove(target.c_str())) {
        LOG(INFO) << "delete symlink " << target << "sucess";
    } else {
        LOG(INFO) << "delete symlink " << target << "fail:" << strerror(errno);
    }
    LOG(INFO) << "create symlink " << target << "->" << source;
    if (symlink(source.c_str(), target.c_str()) < 0) {
        PLOG(ERROR)<< "Failed to create symlink " << target << "->" << source;
        res = -errno;
    }
    return res;
}

/*  delete symlink  */
status_t DeleteSymlink(const std::string& path) {
    status_t res = 0;
    LOG(INFO) << "delete symlink " << path;
    if (remove(path.c_str()) < 0) {
        PLOG(ERROR)<< "Failed to delete symlink " << path;
        res = -errno;
    }
    return res;
}
