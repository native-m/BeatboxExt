# BeatboxExt
## What it does?
Extract sample from Caustic's Beatbox preset.

## How to build
Open the project on Visual Studio 2015 and build.

## How to use this?
```
Caustic BeatBox sample extractor v1.0
Usage: BeatboxExt [beatbox preset file] [options] [file/folder]
Options:
        -n <sample name>        Extract single sample by name. (If the name contain spaces,
                                you must use two double quote like this "My sample")
        -i <sample order>       Extract single sample by order.
Example:
        Extract all samples     BeatboxExt 606.beatbox /path/to/output/folder/ (output folder must be a valid folder)
        Extract sample by name  BeatboxExt 606.beatbox -n "Kick" /path/to/file.wav
        Extract sample by order BeatboxExt 606.beatbox -i 1 /path/to/file.wav
```

## FYI
BeatboxExt only support preset version v3.2.0 and v0.2.1.  
Other versions will be supported soon
