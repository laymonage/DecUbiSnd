@echo off
echo %1
"../DecUbiSnd" --no-banner -S "%1" > "~Segments.seg.tmp"
"../DecUbiSnd" --no-banner "%1" -g "~Segments.seg.tmp" -l 1 -o "%1.wav"
del "~Segments.seg.tmp"
echo.
