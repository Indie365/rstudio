## RStudio 2024.10.0 "Kousa Dogwood" Release Notes

### New
#### RStudio
- Use native file and message dialogs by default on Linux desktop (#14683; Linux Desktop)
- Added www-socket option to rserver.conf to enable server to listen on a Unix domain socket (#14938; Open-Source Server)

#### Posit Workbench
-

### Fixed
#### RStudio
- Fixed being unable to save file after cancelling the "Choose Encoding" window (#14896)
- Fixed problems creating new files and projects on a UNC path (#14963, #14964; Windows Desktop)
- Prevent attempting to start Copilot on a non-main thread (#14952)
- Reformat Code no longer inserts whitespace around '^' operator (#14973)
- Prompt for personal access token instead of password when using github via https (#14103)
- RStudio now forward the current 'repos' option for actions taken in the Build pane (#5793)

#### Posit Workbench
-

### Dependencies

- Updated Electron to version 31.3.0 (#14982; Desktop)
