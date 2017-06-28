# Amalgamate

A cross platform command line tool for producing an amalgamation of
C or C++ sources.

## What is an amalgamation?

An amalgamation is an alternate way to distribute a library's source code using
only a few files (as low as one or two). This tool produces an amalgamation by
inlining included files to create one or more large source or header files.

## How is this useful?

For libraries which are mature (i.e. do not change often) the amalgamated
distribution format is often both easier to integrate, and update. The
amalgmation is added as an additional source file to an existing project
rather than needing to be built separately by build tools. Here are some
examples of source code libraries that have been amalgamated:

- **[FreeType][1]** http://github.com/vinniefalco/FreeTypeAmalgam
- **[JUCE][2]** http://github.com/vinniefalco/JUCEAmalgam
- **[TagLib][3]** http://github.com/vinniefalco/TagLibAmalgam

## License

Copyright (C) 2012 [Vinnie Falco][4]<br>
Amalgamate is provided under the terms of the [MIT license][5].<br>
Amalgamate uses portions of [JUCE][2], licensed separately.

[1]: http://www.freetype.org "The FreeType Project"
[2]: http://rawmaterialsoftware.com/juce.php "JUCE"
[3]: http://developer.kde.org/~wheeler/taglib.html "TagLib"
[4]: http://vinniefalco.com "Vinnie Falco's Home Page"
[5]: http://www.opensource.org/licenses/MIT "MIT License"
