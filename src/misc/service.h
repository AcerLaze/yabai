#ifndef SERVICE_H
#define SERVICE_H

#define _NAME_YABAI_PLIST   "com.koekeishiya.yabai"
#define _PATH_YABAI_PLIST	"/Users/%s/Library/LaunchAgents/"_NAME_YABAI_PLIST".plist"
#define _PATH_LAUNCHCTL     "/bin/launchctl"

#define _YABAI_PLIST \
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
    "<plist version=\"1.0\">\n" \
    "<dict>\n" \
    "    <key>Label</key>\n" \
    "    <string>"_NAME_YABAI_PLIST"</string>\n" \
    "    <key>ProgramArguments</key>\n" \
    "    <array>\n" \
    "        <string>%s</string>\n" \
    "    </array>\n" \
    "    <key>EnvironmentVariables</key>\n" \
    "    <dict>\n" \
    "        <key>PATH</key>\n" \
    "        <string>%s</string>\n" \
    "    </dict>\n" \
    "    <key>RunAtLoad</key>\n" \
    "    <true/>\n" \
    "    <key>KeepAlive</key>\n" \
    "    <true/>\n" \
    "    <key>StandardOutPath</key>\n" \
    "    <string>/tmp/yabai.out.log</string>\n" \
    "    <key>StandardErrorPath</key>\n" \
    "    <string>/tmp/yabai.err.log</string>\n" \
    "    <key>ThrottleInterval</key>\n" \
    "    <integer>30</integer>\n" \
    "    <key>ProcessType</key>\n" \
    "    <string>Interactive</string>\n" \
    "    <key>Nice</key>\n" \
    "    <integer>-20</integer>\n" \
    "</dict>\n" \
    "</plist>"

static int safe_exec(char *const argv[])
{
    pid_t pid;
    int status = posix_spawn(&pid, argv[0], NULL, NULL, argv, NULL);
    if (status) return 1;

    while ((waitpid(pid, &status, 0) == -1) && (errno == EINTR)) {
        usleep(1000);
    }

    if (WIFSIGNALED(status)) {
        return 1;
    } else if (WIFSTOPPED(status)) {
        return 1;
    } else {
        return WEXITSTATUS(status);
    }
}

static void populate_plist_path(char *yabai_plist_path, int size)
{
    char *user = getenv("USER");
    if (!user) {
        error("yabai: 'env USER' not set! abort..\n");
    }

    snprintf(yabai_plist_path, size, _PATH_YABAI_PLIST, user);
}

static void populate_plist(char *yabai_plist, int size)
{
    char *path_env = getenv("PATH");
    if (!path_env) {
        error("yabai: 'env PATH' not set! abort..\n");
    }

    char exe_path[4096];
    unsigned int exe_path_size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &exe_path_size) < 0) {
        error("yabai: unable to retrieve path of executable! abort..\n");
    }

    snprintf(yabai_plist, size, _YABAI_PLIST, exe_path, path_env);
}

static int service_install_internal(char *yabai_plist_path)
{
    char yabai_plist[4096];
    populate_plist(yabai_plist, sizeof(yabai_plist));

    FILE *handle = fopen(yabai_plist_path, "w");
    if (!handle) return 1;

    size_t bytes = fwrite(yabai_plist, strlen(yabai_plist), 1, handle);
    int result = bytes == 1 ? 0 : 1;
    fclose(handle);

    return result;
}

static int service_install(void)
{
    char yabai_plist_path[MAXLEN];
    populate_plist_path(yabai_plist_path, sizeof(yabai_plist_path));

    if (file_exists(yabai_plist_path)) {
        error("yabai: service file '%s' is already installed! abort..\n", yabai_plist_path);
    }

    return service_install_internal(yabai_plist_path);
}

static int service_uninstall(void)
{
    char yabai_plist_path[MAXLEN];
    populate_plist_path(yabai_plist_path, sizeof(yabai_plist_path));

    if (!file_exists(yabai_plist_path)) {
        error("yabai: service file '%s' is not installed! abort..\n", yabai_plist_path);
    }

    return unlink(yabai_plist_path) == 0 ? 0 : 1;
}

static int service_start(void)
{
    char yabai_plist_path[MAXLEN];
    populate_plist_path(yabai_plist_path, sizeof(yabai_plist_path));

    if (!file_exists(yabai_plist_path)) {
        warn("yabai: service file '%s' is not installed! attempting installation..\n", yabai_plist_path);

        int result = service_install_internal(yabai_plist_path);
        if (result) {
            error("yabai: service file '%s' could not be installed! abort..\n", yabai_plist_path);
        }
    }

    const char *const args[] = { _PATH_LAUNCHCTL, "load", "-w", yabai_plist_path, NULL };
    return safe_exec((char *const*)args);
}

static int service_restart(void)
{
    char yabai_plist_path[MAXLEN];
    populate_plist_path(yabai_plist_path, sizeof(yabai_plist_path));

    if (!file_exists(yabai_plist_path)) {
        error("yabai: service file '%s' is not installed! abort..\n", yabai_plist_path);
    }

    char yabai_service_id[MAXLEN];
    snprintf(yabai_service_id, sizeof(yabai_service_id), "gui/%d/%s", getuid(), _NAME_YABAI_PLIST);

    const char *const args[] = { _PATH_LAUNCHCTL, "kickstart", "-k", yabai_service_id, NULL };
    return safe_exec((char *const*)args);
}

static int service_stop(void)
{
    char yabai_plist_path[MAXLEN];
    populate_plist_path(yabai_plist_path, sizeof(yabai_plist_path));

    if (!file_exists(yabai_plist_path)) {
        error("yabai: service file '%s' is not installed! abort..\n", yabai_plist_path);
    }

    const char *const args[] = { _PATH_LAUNCHCTL, "unload", "-w", yabai_plist_path, NULL };
    return safe_exec((char *const*)args);
}

#endif
