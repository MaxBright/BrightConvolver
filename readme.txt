BrightConvolver
Copyright (C) 2016 Max Bright. All Rights Reserved.
Distributed under the GNU General Public License, information can be found in license.txt

Audio plugin made with wdl-ol by Oli Larken: https://github.com/olilarkin/wdl-ol
Takes two incoming signals and outputs the convolution between the last user-defined amount of samples

Usage: Send input track 1 to channels 1&2, and input track 2 to channels 3&4. Convolution output will be sent to channels 1&2
If you just want the plugin, download .\BrightConvolver.dll

-------------------------------------------------------

Built with Visual Studio 2013
Based on the IPlugExamples that come with wdl-ol: https://github.com/olilarkin/wdl-ol/tree/master/IPlugExamples
wdl Licence can be found in readme-wdl.txt

Uses the ooura FFT package: http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
ooura FFT license can be found in readme-fft.txt

