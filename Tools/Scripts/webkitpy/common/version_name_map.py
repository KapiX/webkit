# Copyright (C) 2017 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re

from webkitpy.common.memoized import memoized
from webkitpy.common.version import Version


class VersionNameMap(object):

    # Allows apple_additions to define a custom mapping
    @staticmethod
    @memoized
    def map(platform=None):
        return VersionNameMap(platform=platform)

    def __init__(self, platform=None):
        if platform is None:
            from webkitpy.common.system.systemhost import SystemHost
            platform = SystemHost().platform
        self.mapping = {}

        self.default_system_platform = platform.os_name
        self.mapping['public'] = {
            'mac': {
                'Leopard': Version(10, 5),
                'Snow Leopard': Version(10, 6),
                'Lion': Version(10, 7),
                'Mountain Lion': Version(10, 8),
                'Mavericks': Version(10, 9),
                'Yosemite': Version(10, 10),
                'El Capitan': Version(10, 11),
                'Sierra': Version(10, 12),
                'High Sierra': Version(10, 13),
                'Future': Version(10, 14),
            },
            'ios': self._automap_to_major_version('iOS', minimum=Version(10), maximum=Version(12)),
            'win': {
                'Win10': Version(10),
                '8.1': Version(6, 3),
                '8': Version(6, 2),
                '7sp0': Version(6, 1, 7600),
                'Vista': Version(6),
                'XP': Version(5, 1),
            },
            'haiku' : { '' : Version(1.4) }
        }

    @classmethod
    def _automap_to_major_version(cls, prefix, minimum=Version(1), maximum=Version(1)):
        result = {}
        assert minimum <= maximum
        for i in xrange((maximum.major + 1) - minimum.major):
            result['{} {}'.format(prefix, str(Version(minimum.major + i)))] = Version(minimum.major + i)
        return result

    def to_name(self, version, platform=None, table='public'):
        platform = self.default_system_platform if platform is None else platform
        assert table in self.mapping
        assert platform in self.mapping[table]
        closest_match = (None, None)
        for os_name, os_version in self.mapping[table][platform].iteritems():
            if version == os_version:
                return os_name
            elif version.contained_in(os_version):
                if closest_match[1] and closest_match[1].contained_in(os_version):
                    continue
                closest_match = (os_name, os_version)
        return closest_match[0]

    @staticmethod
    def strip_name_formatting(name):
        # <OS> major.minor.tiny should map to <OS> major
        if ' ' in name:
            try:
                name = '{}{}'.format(''.join(name.split(' ')[:-1]), Version.from_string(name.split(' ')[-1]).major)
            except ValueError:
                pass
        else:
            try:
                split = re.split(r'\d', name)
                name = '{}{}'.format(split[0], Version.from_string(name[(len(split) - 1):]).major)
            except ValueError:
                pass

        # Strip out any spaces, make everything lower-case
        result = name.replace(' ', '').lower()
        return result

    def from_name(self, name):
        # Exact match
        for _, map in self.mapping.iteritems():
            for os_name, os_map in map.iteritems():
                if name in os_map:
                    return (os_name, os_map[name])

        # It's not an exact match, let's try unifying formatting
        unformatted = self.strip_name_formatting(name)
        for _, map in self.mapping.iteritems():
            for os_name, os_map in map.iteritems():
                for version_name, version in os_map.iteritems():
                    if self.strip_name_formatting(version_name) == unformatted:
                        return (os_name, version)
        return (None, None)
