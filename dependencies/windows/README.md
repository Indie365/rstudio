Configure Windows for RStudio Development
=============================================================================

These instructions are intended for a clean Windows-10 machine and may not
produce a successful build environment if any dependencies are already 
installed.

Bootstrap
=============================================================================
- Open an Administrator PowerShell and enter these commands
- `Set-ExecutionPolicy Unrestricted -force`
- `iex ((New-Object System.Net.WebClient).DownloadString('https://github.com/rstudio/rstudio/blob/master/dependencies/windows/Install-RStudio-Prereqs.ps1'))` 
- wait for the script to complete, it runs unattended

Clone the Repo and Run Batch File
=============================================================================
- Open Command Prompt (non-administrator); do this **after** running the 
PowerShell bootstrapping script above to pick up environment changes
- `cd` to the location you want the repo
- clone the repro, e.g. `git clone https://github.com/rstudio/rstudio`
- `cd rstudio\dependencies\windows`
- `install-dependencies.cmd`
- wait for the script to complete, it runs unattended

Build Java/Gwt
=============================================================================
- `cd rstudio\src\gwt`
- `ant draft` or for iterative development of Java/Gwt code, `ant desktop`

Build C++
=============================================================================
- open Qt Creator
- Open Project and select rstudio\src\cpp\CMakelists.txt
- Select the kit(s) you wish to use, 32-bit and/or 64-bit
- click Configure, then build

Run RStudio
=============================================================================
- from command prompt, `cd` to the build location, and run `rstudio.bat`
- to run RStudio in Qt Creator, _TODO_

Debug RStudio
=============================================================================
- use Qt's debugger; it leverages the Microsoft CDB

Package Build
=============================================================================
This is not necessary for regular development work, but can be used to fully 
test your installation. This builds RStudio and bundles it up in a setup package.

In a non-administrator command prompt:
- `cd rstudio\package\win32`
- `make-package.bat`

When done, the setup is `rstudio\package\build\RStudio-99.9.9.exe`.

