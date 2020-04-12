v01.0
- This is the first release.

v01.1
- (Backward Incompatible) In v01.0, lines preceded by whitespace(s) were interpreted as empty lines and thus ignored. In v01.1, now lines' preceding and trailing whitespaces are removed before parsing, so that one can indent any lines.

v01.2
- Added the lock functionality for safety. Now only one session can be executed simultaneously.

<!-- vim: set spell: -->

