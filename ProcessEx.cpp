
using android::base::ReadFileToString;


void getProcessName(pid_t pid, std::string& out_name) {
    if (!ReadFileToString(StringPrintf("/proc/%d/cmdline", pid), &out_name)) {
        out_name = "???";
    }
}

int KillProcessesWithOpenFiles(const std::string& prefix, int signal) {
    std::unordered_set<pid_t> pids;

    auto proc_d = std::unique_ptr<DIR, int (*)(DIR*)>(opendir("/proc"), closedir);
    if (!proc_d) {
        PLOG(ERROR) << "Failed to open proc";
        return -1;
    }

    struct dirent* proc_de;
    while ((proc_de = readdir(proc_d.get())) != nullptr) {
        // We only care about valid PIDs
        pid_t pid;
        if (proc_de->d_type != DT_DIR) continue;
        if (!android::base::ParseInt(proc_de->d_name, &pid)) continue;

        // Look for references to prefix
        bool found = false;
        auto path = StringPrintf("/proc/%d", pid);
        found |= checkMaps(path + "/maps", prefix);
        found |= checkSymlink(path + "/cwd", prefix);
        found |= checkSymlink(path + "/root", prefix);
        found |= checkSymlink(path + "/exe", prefix);

        auto fd_path = path + "/fd";
        auto fd_d = std::unique_ptr<DIR, int (*)(DIR*)>(opendir(fd_path.c_str()), closedir);
        if (!fd_d) {
            PLOG(WARNING) << "Failed to open " << fd_path;
        } else {
            struct dirent* fd_de;
            while ((fd_de = readdir(fd_d.get())) != nullptr) {
                if (fd_de->d_type != DT_LNK) continue;
                found |= checkSymlink(fd_path + "/" + fd_de->d_name, prefix);
            }
        }

        if (found) {
            std::string name;
            getProcessName(pid, name);
            LOG(WARNING)<< "Process " << name.c_str() << " (" << pid << ")  has maps/cwd/root/fd within " << prefix;
            pids.insert(pid);
        }
    }
    if (signal != 0) {
        for (const auto& pid : pids) {
            std::string name;
            getProcessName(pid, name);
            /* SPRD: support double sdcard kill ylog process force when unmount sd card for performance @{ */
            if(!strcmp(name.c_str(), "/system/bin/ylog") || !strcmp(name.c_str(), "/system/bin/slogmodem")){
               LOG(WARNING) << "Sending SIGKILL to " << name.c_str() << " (" << pid << ")";
               kill(pid, SIGKILL);
            } else if (!strcmp(name.c_str(), "system_server")) {
                 //Do nothing
            } else {
                LOG(WARNING) << "Sending " << strsignal(signal) << " to " << pid;
                kill(pid, signal);
            }
            /* @} */
        }
    }
    return pids.size();
}
