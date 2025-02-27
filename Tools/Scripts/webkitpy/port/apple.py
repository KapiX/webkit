# Copyright (C) 2011 Google Inc. All rights reserved.
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
#     * Neither the Google name nor the names of its
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

import logging
import os

from webkitpy.common.system.executive import ScriptError
from webkitpy.port.base import Port
from webkitpy.layout_tests.models.test_configuration import TestConfiguration


_log = logging.getLogger(__name__)


class ApplePort(Port):
    """Shared logic between all of Apple's ports."""

    # This is used to represent the version of an operating system
    # corresponding to the "mac" or "win" base LayoutTests/platform
    # directory.  I'm not sure this concept is very useful,
    # but it gives us a way to refer to fallback paths *only* including
    # the base directory.
    # This is mostly done because TestConfiguration assumes that self.version()
    # will never return None. (None would be another way to represent this concept.)
    # Apple supposedly has explicit "future" results which are kept in an internal repository.
    # It's possible that Apple would want to fix this code to work better with those results.
    FUTURE_VERSION = 'future'  # FIXME: This whole 'future' thing feels like a hack.

    # overridden in subclasses
    VERSION_FALLBACK_ORDER = []
    ARCHITECTURES = []
    _crash_logs_to_skip_for_host = {}

    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        options = options or {}
        if port_name in (cls.port_name, cls.port_name + '-wk2'):

            # Since IOS simulator run on mac, they need a special check
            if host.platform.os_name == 'mac' and 'ios-simulator' in port_name:
                return port_name

            # If the port_name matches the (badly named) cls.port_name, that
            # means that they passed 'mac' or 'win' and didn't specify a version.
            # That convention means that we're supposed to use the version currently
            # being run, so this won't work if you're not on mac or win (respectively).
            # If you're not on the o/s in question, you must specify a full version or -future (cf. above).
            if port_name == cls.port_name and not getattr(options, 'webkit_test_runner', False):
                port_name = cls.port_name + '-' + host.platform.os_version
            else:
                port_name = cls.port_name + '-' + host.platform.os_version + '-wk2'
        elif getattr(options, 'webkit_test_runner', False) and  '-wk2' not in port_name:
            port_name += '-wk2'

        return port_name

    def _strip_port_name_prefix(self, port_name):
        # Callers treat this return value as the "version", which only works
        # because Apple ports use a simple name-version port_name scheme.
        # FIXME: This parsing wouldn't be needed if port_name handling was moved to factory.py
        # instead of the individual port constructors.
        return port_name[len(self.port_name + '-'):]

    def __init__(self, host, port_name, **kwargs):
        super(ApplePort, self).__init__(host, port_name, **kwargs)

        port_name = port_name.replace('-wk2', '')
        self._version = self._strip_port_name_prefix(port_name)

    def setup_test_run(self, device_class=None):
        self._crash_logs_to_skip_for_host[self.host] = self.host.filesystem.files_under(self.path_to_crash_logs())

    def default_timeout_ms(self):
        if self.get_option('guard_malloc'):
            return 350 * 1000
        return super(ApplePort, self).default_timeout_ms()

    def should_retry_crashes(self):
        return True

    def _skipped_file_search_paths(self):
        # We don't have a dedicated Skipped file for the most recent version of the port;
        # we just use the one in platform/{mac,win}
        most_recent_name = self.VERSION_FALLBACK_ORDER[-1]
        return set(filter(lambda name: name != most_recent_name, super(ApplePort, self)._skipped_file_search_paths()))

    # FIXME: A more sophisticated version of this function should move to WebKitPort and replace all calls to name().
    # This is also a misleading name, since 'mac-future' gets remapped to 'mac'.
    def _port_name_with_version(self):
        return self.name().replace('-future', '').replace('-wk2', '')

    def _allowed_port_names(self):
        return self.VERSION_FALLBACK_ORDER + [self._future_port_name()]

    def _future_port_name(self):
        return self.operating_system() + "-" + self.FUTURE_VERSION

    def _generate_all_test_configurations(self):
        configurations = []
        for port_name in self._allowed_port_names():
            for build_type in self.ALL_BUILD_TYPES:
                for architecture in self.ARCHITECTURES:
                    configurations.append(TestConfiguration(version=self._strip_port_name_prefix(port_name), architecture=architecture, build_type=build_type))
        return configurations

    def _path_to_helper(self):
        binary_name = 'LayoutTestHelper'
        return self._build_path(binary_name)
