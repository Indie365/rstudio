## RStudio 2022-06.0 "Spotted Wakerobin" Release Notes

### New
- Source marker `message` can contain ANSI SGR codes for setting style and color (#9010)
- Linux/MacOS: Executing a code selection that encounters an error will stop execution of remaining code (#3014)

#### R
- Added support for using the AGG renderer (as provided by the ragg package) as a graphics backend for inline plot execution; also added support for using the backend graphics device requested by the knitr `dev` chunk option (#9931)

### Fixed
- Fixed notebook execution handling of knitr `message=FALSE` chunk option to suppress messages if the option is set to FALSE (#9436)

### Breaking

### Deprecated / Removed
There is no deprecated or removed functionality in this release.
