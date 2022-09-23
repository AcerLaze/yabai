# fyabai

Personal patches for yabai. Cherry-pick them to your own fork if you want to use them:

Only works with disabled system window manager and Ventura (?) (e.g. in `yabairc`):
```bash
launchctl unload -F /System/Library/LaunchAgents/com.apple.WindowManager.plist > /dev/null 2>&1 &
```

- Upgrade IPC from unix sockets to mach messages
- Window borders are drawn half inside and half outside the window to avoid gaps
- Animation fade for smoother animations
- Allow only one child at a time to zoom to parent node
- Zoomed windows occlude windows below in the next-window-in-direction calculation
- Focus closest window on application termination
- Focus sibling window node on window destruction
