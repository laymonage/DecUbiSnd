# DecUbiSnd

DecUbiSnd decodes audio streams found in many Ubisoft games. Run `DecUbiSnd` without any arguments to see the available command-line switches. The `Examples` directory contains batch files demonstrating usage.

## Building

On Windows, install Visual Studio with the C++ workload and make `vcpkg` available. From the repository root, install the dependencies and build:

```powershell
vcpkg install --triplet x86-windows-static-md --x-install-root=vcpkg_static --x-feature=gui
msbuild Package.proj /t:BuildAll /p:Configuration=Release /p:Platform=Win32
```

The projects use [vcpkg's MSBuild integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/msbuild-integration) for include paths and library linking; the explicit install step selects the optional GUI feature in the shared install tree.

The executables require the [latest supported Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) to run.

## Replacing audio segments

Audio replacement is an **experimental** feature and currently supports only mono 4-bit ADPCM segments in Splinter Cell 1 PC `.LS0` banks. Support for other Ubisoft games and audio formats may be added in the future.

For the current Splinter Cell 1 implementation, the replacement WAV must be mono signed 16-bit PCM at 36 kHz and contain exactly the target segment's sample count. The output can be the source bank itself to overwrite it; make a backup first. The bank is written through a temporary file and replaced only after encoding succeeds.

CLI example:

```powershell
DecUbiSnd.exe .\0_0_2.LS0 --replace --replacement-wav .\replacement.wav --offset 564212 --output .\0_0_2_replaced.LS0
```

Use offset `0` for the first segment.

In the GUI, scan or open an `.LS0` bank, select one mono 4-bit segment, and choose **Replace Segment...**. The selected bank and byte offset are used automatically; choose a replacement WAV and output bank. Select the loaded bank as the output to overwrite it; make a backup first. The current encoder does not resample or trim audio. A successful write confirms format and size checks, not in-game playback, so verify the output with DecUbiSnd's decoder and listen before use.

## Historical context

The project was originally published on the [XeNTaX forums](https://web.archive.org/web/20231015024409/https://forum.xentax.com/viewtopic.php?f=17&t=3156). However, the forum and the files stored in the bitbucket repository were lost to time. I managed to contact [@Zenchreal](https://github.com/Zenchreal), the original author, asking for permission to mirror the project here. Here's a portion of the email.


> > Would you be happy if I put the mirror up on GitHub?
>
> Absolutely. That would be really great actually. I won't be releasing any new versions. You are definitely welcome to polish it up or even add new features. And then use it for any of your projects.
>
> > What license would you use for the source code?
>
> I would go with the BSD 3-Clause

I reconstructed the provided releases into a git history and added the license file. I intend to use this repository as a fork of the original project to make it more usable on modern platforms.

This repository contains three branches:

- [`main`](https://github.com/laymonage/DecUbiSnd/tree/main): a combined codebase for `DecUbiSnd` + `DecUbiSndGui`, will be the main development branch moving forward.
- [`legacy/DecUbiSnd`](https://github.com/laymonage/DecUbiSnd/tree/legacy/DecUbiSnd): the original source code for DecUbiSnd, plus the missing `6BitAdpcm.cpp` file, with README and license.
- [`legacy/DecUbiSndGui`](https://github.com/laymonage/DecUbiSnd/tree/legacy/DecUbiSndGui): the original source code for DecUbiSndGui, plus the missing `6BitAdpcm.cpp` file, with README and license.

### Original credits

Coded by Zench of the XeNTaX forums ([forum.xentax.com](https://forum.xentax.com)).

Special thanks, in no particular order, to Kataah, OrangeC, Mirrodin, and Nikson of the XeNTaX forums.
