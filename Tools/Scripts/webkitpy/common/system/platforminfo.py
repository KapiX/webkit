# Copyright (c) 2011 Google Inc. All rights reserved.
# Copyright (c) 2015-2017 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re
import sys

from webkitpy.common.version import Version
from webkitpy.common.version_name_map import VersionNameMap
from webkitpy.common.system.executive import Executive


class PlatformInfo(object):
    """This class provides a consistent (and mockable) interpretation of
    system-specific values (like sys.platform and platform.mac_ver())
    to be used by the rest of the webkitpy code base.

    Public (static) properties:
    -- os_name
    -- os_version

    Note that 'future' is returned for os_version if the operating system is
    newer than one known to the code.
    """

    def __init__(self, sys_module, platform_module, executive):
        self._executive = executive
        self._platform_module = platform_module
        self.os_name = self._determine_os_name(sys_module.platform)
        self.os_version = None

        if self.os_name.startswith('mac'):
            version = Version.from_string(platform_module.mac_ver()[0])
        elif self.os_name.startswith('win'):
            version = self._win_version(sys_module)
        elif self.os_name == 'linux' or self.os_name == 'freebsd' or self.os_name == 'openbsd' or self.os_name == 'netbsd':
            version = None
        else:
            # Most other platforms (namely iOS) return conforming version strings.
            version = Version.from_string(platform_module.release())

        self._is_cygwin = sys_module.platform == 'cygwin'

        if version is None:
            return

        self.os_version = VersionNameMap.map(self).to_name(version, table='public')
        assert self.os_version is not None
        self.os_version = self.os_version.lower().replace(' ', '')

    def is_mac(self):
        return self.os_name == 'mac'

    def is_ios(self):
        return self.os_name == 'ios'

    def is_win(self):
        return self.os_name == 'win'

    def is_native_win(self):
        return self.is_win() and not self.is_cygwin()

    def is_cygwin(self):
        return self._is_cygwin

    def is_haiku(self):
        return self.os_name == 'haiku'

    def is_linux(self):
        return self.os_name == 'linux'

    def is_freebsd(self):
        return self.os_name == 'freebsd'

    def is_openbsd(self):
        return self.os_name == 'openbsd'

    def is_netbsd(self):
        return self.os_name == 'netbsd'

    def display_name(self):
        # platform.platform() returns Darwin information for Mac, which is just confusing.
        if self.is_mac():
            return "Mac OS X %s" % self._platform_module.mac_ver()[0]

        # Returns strings like:
        # Linux-2.6.18-194.3.1.el5-i686-with-redhat-5.5-Final
        # Windows-2008ServerR2-6.1.7600
        return self._platform_module.platform()

    def total_bytes_memory(self):
        if self.is_mac():
            return long(self._executive.run_command(["sysctl", "-n", "hw.memsize"]))
        return None

    def terminal_width(self):
        """Returns sys.maxint if the width cannot be determined."""
        try:
            if self.is_win():
                # From http://code.activestate.com/recipes/440694-determine-size-of-console-window-on-windows/
                from ctypes import windll, create_string_buffer
                handle = windll.kernel32.GetStdHandle(-12)  # -12 == stderr
                console_screen_buffer_info = create_string_buffer(22)  # 22 == sizeof(console_screen_buffer_info)
                if windll.kernel32.GetConsoleScreenBufferInfo(handle, console_screen_buffer_info):
                    import struct
                    _, _, _, _, _, left, _, right, _, _, _ = struct.unpack("hhhhHhhhhhh", console_screen_buffer_info.raw)
                    # Note that we return 1 less than the width since writing into the rightmost column
                    # automatically performs a line feed.
                    return right - left
                return sys.maxint
            else:
                import fcntl
                import struct
                import termios
                packed = fcntl.ioctl(sys.stderr.fileno(), termios.TIOCGWINSZ, '\0' * 8)
                _, columns, _, _ = struct.unpack('HHHH', packed)
                return columns
        except:
            return sys.maxint

    def xcode_sdk_version(self, sdk_name):
        if self.is_mac():
            # Assumes that xcrun does not write to standard output on failure (e.g. SDK does not exist).
            xcrun_output = self._executive.run_command(['xcrun', '--sdk', sdk_name, '--show-sdk-version'], return_stderr=False, error_handler=Executive.ignore_error).rstrip()
            if xcrun_output:
                return Version.from_string(xcrun_output)
        return None

    def xcode_simctl_list(self):
        if not self.is_mac():
            return ()
        output = self._executive.run_command(['xcrun', 'simctl', 'list'], return_stderr=False)
        return (line for line in output.splitlines())

    def xcode_version(self):
        if not self.is_mac():
            raise NotImplementedError
        return Version.from_string(self._executive.run_command(['xcodebuild', '-version']).split()[1])

    def _determine_os_name(self, sys_platform):
        if sys_platform == 'darwin':
            return 'mac'
        if sys_platform == 'ios':
            return 'ios'
        if sys_platform.startswith('linux'):
            return 'linux'
        if sys_platform.startswith('haiku'):
            return 'haiku'
        if sys_platform.startswith('win') or sys_platform == 'cygwin':
            return 'win'
        if sys_platform.startswith('freebsd'):
            return 'freebsd'
        if sys_platform.startswith('openbsd'):
            return 'openbsd'
        if sys_platform.startswith('haiku'):
            return 'haiku'
        raise AssertionError('unrecognized platform string "%s"' % sys_platform)

    def _win_version(self, sys_module):
        if hasattr(sys_module, 'getwindowsversion'):
            return Version.from_iterable(sys_module.getwindowsversion()[0:3])
        return Version.from_iterable(self._win_version_from_cmd())

    def _win_version_from_cmd(self):
        # Note that this should only ever be called on windows, so this should always work.
        ver_output = self._executive.run_command(['cmd', '/c', 'ver'], decode_output=False)
        match_object = re.search(r'(?P<major>\d+)\.(?P<minor>\d+)\.(?P<build>\d+)', ver_output)
        assert match_object, 'cmd returned an unexpected version string: ' + ver_output
        return match_object.groups()
