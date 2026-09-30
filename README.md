# DecUbiSnd

DecUbiSnd decodes audio streams found in many Ubisoft games. Run `DecUbiSnd` without any arguments to see the available command-line switches. The `Examples` directory contains batch files demonstrating usage.

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