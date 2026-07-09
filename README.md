Cool Edit File Filters
----------------------
Various file filters for the ancient, but still excellent, Cool Edit 2000 application.
They may also work with very early versions of Adobe Audition.

The latest release can be found at https://github.com/jfchapman/CoolEditFileFilters/releases/latest

The following file filters are included:
- flac.flt - Free Lossless Audio Codec (read/write)
- opus.flt - Opus Audio Codec (read/write)
- ffmpeg.flt - FFmpeg (read only)
- ffmpeg_aac.flt - Advanced Audio Coding via FFmpeg (read/write)
- ffmpeg_alac.flt - Apple Lossless via FFmpeg (read/write)
- openmpt.flt - Tracker formats (MOD, XM, IT, etc.) via libopenmpt (read only)

The FFmpeg filter should read just about any audio/video file format.
For video files, the default audio track will be read.


Credits
-------
FLAC is copyright (c) 2000-2009 Josh Coalson, 2011-2026 Xiph.Org
http://xiph.org/flac/

Opus is copyright (c) 2011-2026 Xiph.Org
https://opus-codec.org/

This software uses libraries from the FFmpeg project under the LGPLv2.1
https://ffmpeg.org/

This software uses the libopenmpt library under the BSD-3-Clause License
Copyright (c) 2004-2026, OpenMPT Project Developers and Contributors
Copyright (c) 1997-2003, Olivier Lapicque
https://lib.openmpt.org/libopenmpt/

JSON for Modern C++ is copyright (c) 2013-2026 Niels Lohmann
https://github.com/nlohmann/json


License
-------
Copyright (c) 2026 James Chapman

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
