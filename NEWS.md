
## RStudio 2022-06.0 "Spotted Wakerobin" Release Notes


### New

- Source marker `message` can contain ANSI SGR codes for setting style and color (#9010)
- Linux/MacOS: Executing a code selection that encounters an error will stop execution of remaining code (#3014)
- Added support for hyperlinks in the console and build pane (#1941)
- "Clean and Rebuild" and "Install and Restart" have been merged into "Install Package", the "Project Options > Build" gains a "clean before install" option to toggle the --preclean flag. The Build toolbar changes to "Install | Test | Check | More v" (#4289)
- The source button uses `cpp11::source_cpp()` on C++ files that have `[[cpp11::register]]` decorations (#10387)
- New *Relative Line Numbers* preference for showing line numbers relative to the current line, rather than the first line (#1774)

#### Find in Files
- Fixed Find in Files whole-word replace option, so that when "Whole word" is checked, only file matches containing the whole word are replaced, as displayed in the preview (#9813)
- Adds support for POSIX extended regular expressions with backreferences in Find in Files find and replace modes, so that special regex characters such as `+` and `?`, `|`, `(`, etc do not need to be escaped, and other ERE escape sequences such as `\b`, `\w`, and `\d` are now supported. This matches the behavior of R's own `grep()` function, but note that backslashes do not need to be escaped (as they typically are in R strings) (#9344)
- The "Common R source files" option in Find in Files has been updated to "Common source files", with support for searching Markdown (of any type, including .Rmd), JS, and YAML files (#10526)
- Updated support for searching paths and filenames with Unicode characters on Windows, including Chinese and non-Latin characters (#9881)

#### R

- Added support for using the AGG renderer (as provided by the ragg package) as a graphics backend for inline plot execution; also added support for using the backend graphics device requested by the knitr `dev` chunk option (#9931)
- rstudioapi functions are now always evaluated in a clean environment, and will not be masked by objects in the global environment (#8031)

#### Python
- RStudio attempts to infer the appropriate version of Python when "Automatically activate project-local Python environments" is checked and the user has not requested a specific version of Python. This Python will be stored in the environment variable "RETICULATE_PYTHON_FALLBACK", available from the R console, the Python REPL, and the RStudio Terminal (#9990)

### Fixed

- Fixed notebook execution handling of knitr `message=FALSE` chunk option to suppress messages if the option is set to FALSE (#9436)
- Fixed plot export to PDF options (#9185)
- `.rs.formatDataColumnDispatch()` iterates through classes of `x` (#10073)
- `.rs.api.navigateToFile()` is now synchronous and returns document id (#8938)
- The `Session > Load Workspace` menu option now explicitly namespaces `base::load` if the `load` function has been masked in the global environment (#10089)
- The data viewer truncates large list cells to 50 characters by default, this can be changed with the command palette or `rstudioapi::writeRStudioPreference("data_viewer_max_cell_size", 10L)` (#5100)
- The R version and logo displayed in the top left of the console will update to the current R version whenever the R session is restarted (#10458)

### RStudio Workbench

- Add a -G option to `rsandbox` to allow configuring the effective group of the process (#3214)


### Deprecated / Removed

There is no deprecated or removed functionality in this release.

