# Copyright (C) 2014-2017 Apple Inc. All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import itertools
import logging
import os
import plistlib
import re
import subprocess
import time

from webkitpy.common.timeout_context import Timeout
from webkitpy.common.host import Host
from webkitpy.common.version import Version

_log = logging.getLogger(__name__)

"""
Minimally wraps CoreSimulator functionality through simctl.

If possible, use real CoreSimulator.framework functionality by linking to the framework itself.
Do not use PyObjC to dlopen the framework.
"""


class DeviceType(object):
    """
    Represents a CoreSimulator device type.
    """
    def __init__(self, name, identifier):
        """
        :param name: The device type's human-readable name
        :type name: str
        :param identifier: The CoreSimulator identifier.
        :type identifier: str
        """
        self.name = name
        self.identifier = identifier

    @classmethod
    def from_name(cls, name):
        """
        :param name: The name for the desired device type.
        :type name: str
        :returns: A `DeviceType` object with the specified identifier or throws a TypeError if it doesn't exist.
        :rtype: DeviceType
        """
        identifier = None
        for device_type in Simulator().device_types:
            if device_type.name == name:
                identifier = device_type.identifier
                break

        if identifier is None:
            raise TypeError('A device type with name "{name}" does not exist.'.format(name=name))

        return DeviceType(name, identifier)

    @classmethod
    def from_identifier(cls, identifier):
        """
        :param identifier: The CoreSimulator identifier for the desired runtime.
        :type identifier: str
        :returns: A `Runtime` object witht the specified identifier or throws a TypeError if it doesn't exist.
        :rtype: DeviceType
        """
        name = None
        for device_type in Simulator().device_types:
            if device_type.identifier == identifier:
                name = device_type.name
                break

        if name is None:
            raise TypeError('A device type with identifier "{identifier}" does not exist.'.format(
                identifier=identifier))

        return DeviceType(name, identifier)

    def __eq__(self, other):
        return (self.name == other.name) and (self.identifier == other.identifier)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        return '<DeviceType "{name}": {identifier}>'.format(name=self.name, identifier=self.identifier)


class Runtime(object):
    """
    Represents a CoreSimulator runtime associated with an iOS SDK.
    """

    def __init__(self, version, identifier, available, devices=None, is_internal_runtime=False):
        """
        :param version: The iOS SDK version
        :type version: tuple
        :param identifier: The CoreSimulator runtime identifier
        :type identifier: str
        :param availability: Whether the runtime is available for use.
        :type availability: bool
        :param devices: A list of devices under this runtime
        :type devices: list or None
        :param is_internal_runtime: Whether the runtime is an Apple internal runtime
        :type is_internal_runtime: bool
        """
        self.version = version
        self.identifier = identifier
        self.available = available
        self.devices = devices or []
        self.is_internal_runtime = is_internal_runtime

    @classmethod
    def from_version(cls, version):
        return cls.from_identifier('com.apple.CoreSimulator.SimRuntime.iOS-' + '{}-{}'.format(version[0], version[1]))

    @classmethod
    def from_identifier(cls, identifier):
        """
        :param identifier: The identifier for the desired CoreSimulator runtime.
        :type identifier: str
        :returns: A `Runtime` object with the specified identifier or throws a TypeError if it doesn't exist.
        :rtype: Runtime
        """
        for runtime in Simulator().runtimes:
            if runtime.identifier == identifier:
                return runtime
        raise TypeError('A runtime with identifier "{identifier}" does not exist.'.format(identifier=identifier))

    def __eq__(self, other):
        return (self.version == other.version) and (self.identifier == other.identifier) and (self.is_internal_runtime == other.is_internal_runtime)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        version_suffix = ""
        if self.is_internal_runtime:
            version_suffix = " Internal"
        return '<Runtime {version}: {identifier}. Available: {available}, {num_devices} devices>'.format(
            version=str(self.version) + version_suffix,
            identifier=self.identifier,
            available=self.available,
            num_devices=len(self.devices))


# FIXME: This class is fragile because it parses the output of the simctl command line utility, which may change.
#        We should find a better way to query for simulator device state and capabilities. Maybe take a similiar
#        approach as in webkitdirs.pm and utilize the parsed output from the device.plist files in the sub-
#        directories of ~/Library/Developer/CoreSimulator/Devices?
#        Also, simctl has the option to output in JSON format (xcrun simctl list --json).
class Simulator(object):
    """
    Represents the iOS Simulator infrastructure under the currently select Xcode.app bundle.
    """
    device_type_re = re.compile('(?P<name>.+)\((?P<identifier>[^)]+)\)')
    runtime_re = re.compile('(?P<os>.+) \((?P<version>\d+\.\d+(\.\d+)?) - (?P<build_version>[^)]+)\) \((?P<identifier>[^)]+)\)( \((?P<availability>[^)]+)\))?')
    new_runtime_re = re.compile('(?P<os>.+) \((?P<version>\d+\.\d+(\.\d+)?) - (?P<build_version>[^)]+)\) - (?P<identifier>[^)]+)( \((?P<availability>[^)]+)\))?')
    unavailable_version_re = re.compile('-- (Unavailable: )?(?P<identifier>[^ ]+) --')
    version_re = re.compile('-- (i|watch|tv)OS (?P<version>\d+\.\d+)(?P<internal> Internal)? --')
    devices_re = re.compile(
        '\s*(?P<name>.+) \((?P<udid>[A-Z0-9\-]+)\) \((?P<state>[^)]+)\)( \((?P<availability>[^)]+)\))?')

    managed_devices = {}
    Device = None

    def __init__(self, host=None):
        # FIXME: This circular import should be resolved.
        if not Simulator.Device:
            from webkitpy.xcode.simulated_device import SimulatedDevice
            Simulator.Device = SimulatedDevice

        self._host = host or Host()
        self.runtimes = []
        self.device_types = []
        self.refresh()

    # Keep these constants synchronized with the SimDeviceState constants in CoreSimulator/SimDevice.h.
    class DeviceState:
        DOES_NOT_EXIST = -1
        CREATING = 0
        SHUTDOWN = 1
        BOOTING = 2
        BOOTED = 3
        SHUTTING_DOWN = 4

    NAME_FOR_STATE = [
        'CREATING',
        'SHUTDOWN',
        'BOOTING',
        'BOOTED',
        'SHUTTING_DOWN'
    ]

    @staticmethod
    def create_device(number, device_type, runtime):
        device = Simulator().lookup_or_create_device(device_type.name + ' WebKit Tester' + str(number), device_type, runtime)
        _log.debug('created device {} {}'.format(number, device))
        assert(len(Simulator.managed_devices) == number)
        Simulator.managed_devices[number] = device

    @staticmethod
    def remove_device(number):
        if not Simulator.managed_devices[number]:
            return
        device_udid = Simulator.managed_devices[number].udid
        _log.debug('removing device {} {}'.format(number, device_udid))
        del Simulator.managed_devices[number]
        Simulator.delete_device(device_udid)

    @staticmethod
    def device_number(number):
        return Simulator.managed_devices[number]

    @staticmethod
    def device_state_description(state):
        if (state == Simulator.DeviceState.DOES_NOT_EXIST):
            return 'DOES_NOT_EXIST'
        return Simulator.NAME_FOR_STATE[state]

    # FIXME: When <rdar://problem/31080009> is fixed, decrease timeout back to 5 minutes
    @staticmethod
    def wait_until_device_is_booted(udid, timeout_seconds=60 * 15):
        Simulator.wait_until_device_is_in_state(udid, Simulator.DeviceState.BOOTED, timeout_seconds)
        with Timeout(seconds=timeout_seconds):
            while True:
                try:
                    state = subprocess.check_output(['xcrun', 'simctl', 'spawn', udid, 'launchctl', 'print', 'system']).strip()
                    _log.debug('xcrun simctl spawn %s', udid)

                    if re.search("A[\s]+com.apple.springboard.services", state):
                        return
                except subprocess.CalledProcessError:
                    if Simulator.device_state(udid) != Simulator.DeviceState.BOOTED:
                        raise RuntimeError('Simuator device quit unexpectedly.')
                    _log.warn("Error in checking Simulator boot status. Will retry in 1 second.")
                time.sleep(1)

    # FIXME: When <rdar://problem/31080009> is fixed, decrease timeout back to 5 minutes
    @staticmethod
    def wait_until_device_is_in_state(udid, wait_until_state, timeout_seconds=60 * 15):
        _log.debug('waiting for device %s to enter state %s with timeout %s', udid, Simulator.device_state_description(wait_until_state), timeout_seconds)
        with Timeout(seconds=timeout_seconds):
            device_state = Simulator.device_state(udid)
            while (device_state != wait_until_state):
                device_state = Simulator.device_state(udid)
                _log.debug(' device state %s', Simulator.device_state_description(device_state))
                time.sleep(0.5)

        end_state = Simulator.device_state(udid)
        if (end_state != wait_until_state):
            raise RuntimeError('Timed out waiting for simulator device to enter state {0}; current state is {1}'.format(Simulator.device_state_description(wait_until_state), Simulator.device_state_description(end_state)))

    @staticmethod
    def device_state(udid):
        device_plist = os.path.join(Simulator.device_directory(udid), 'device.plist')
        if not os.path.isfile(device_plist):
            return Simulator.DeviceState.DOES_NOT_EXIST
        return plistlib.readPlist(device_plist)['state']

    @staticmethod
    def device_directory(udid):
        return os.path.realpath(os.path.expanduser(os.path.join('~/Library/Developer/CoreSimulator/Devices', udid)))

    @staticmethod
    def delete_device(udid):
        Simulator.Device.delete(udid)

    @staticmethod
    def reset_device(udid):
        Simulator.Device.reset(udid)

    def refresh(self):
        """
        Refresh runtime and device type information from ``simctl list``.
        """
        lines = self._host.platform.xcode_simctl_list()
        if not lines:
            return
        device_types_header = next(lines)
        if device_types_header != '== Device Types ==':
            raise RuntimeError('Expected == Device Types == header but got: "{}"'.format(device_types_header))
        self._parse_device_types(lines)

    def _parse_device_types(self, lines):
        """
        Parse device types from ``simctl list``.
        :param lines: A generator for the output lines from ``simctl list``.
        :type lines: genexpr
        :return: None
        """
        for line in lines:
            device_type_match = self.device_type_re.match(line)
            if not device_type_match:
                if line != '== Runtimes ==':
                    raise RuntimeError('Expected == Runtimes == header but got: "{}"'.format(line))
                break
            device_type = DeviceType(name=device_type_match.group('name').rstrip(),
                                     identifier=device_type_match.group('identifier'))
            self.device_types.append(device_type)

        self._parse_runtimes(lines)

    def _parse_runtimes(self, lines):
        """
        Continue to parse runtimes from ``simctl list``.
        :param lines: A generator for the output lines from ``simctl list``.
        :type lines: genexpr
        :return: None
        """
        for line in lines:
            runtime_match = self.runtime_re.match(line) or self.new_runtime_re.match(line)
            if not runtime_match:
                if line != '== Devices ==':
                    raise RuntimeError('Expected == Devices == header but got: "{}"'.format(line))
                break
            runtime = Runtime(version=Version.from_string(runtime_match.group('version')),
                              identifier=runtime_match.group('identifier'),
                              available=runtime_match.group('availability') is None,
                              is_internal_runtime=('Internal' in runtime_match.group('os')))
            self.runtimes.append(runtime)
        self._parse_devices(lines)

    def _parse_devices(self, lines):
        """
        Continue to parse devices from ``simctl list``.
        :param lines: A generator for the output lines from ``simctl list``.
        :type lines: genexpr
        :return: None
        """
        current_runtime = None
        for line in lines:
            version_match = self.version_re.match(line)
            if version_match:
                current_runtime = self.runtime(version=Version.from_string(version_match.group('version')), is_internal_runtime=bool(version_match.group('internal')))
                assert current_runtime
                continue

            unavailable_version_match = self.unavailable_version_re.match(line)
            if unavailable_version_match:
                current_runtime = None
                continue

            device_match = self.devices_re.match(line)
            if not device_match:
                if line != '== Device Pairs ==':
                    raise RuntimeError('Expected == Device Pairs == header but got: "{}"'.format(line))
                break
            if current_runtime:
                device = Simulator.Device(name=device_match.group('name').rstrip(),
                                udid=device_match.group('udid'),
                                available=device_match.group('availability') is None,
                                runtime=current_runtime,
                                host=self._host)
                current_runtime.devices.append(device)

    def device_type(self, name=None, identifier=None):
        """
        :param name: The short name of the device type.
        :type name: str
        :param identifier: The CoreSimulator identifier of the desired device type.
        :type identifier: str
        :return: A device type with the specified name and/or identifier, or None if one doesn't exist as such.
        :rtype: DeviceType
        """
        for device_type in self.device_types:
            if name and device_type.name != name:
                continue
            if identifier and device_type.identifier != identifier:
                continue
            return device_type
        return None

    def runtime(self, version=None, identifier=None, is_internal_runtime=None):
        """
        :param version: The iOS version of the desired runtime.
        :type version: tuple
        :param identifier: The CoreSimulator identifier of the desired runtime.
        :type identifier: str
        :return: A runtime with the specified version and/or identifier, or None if one doesn't exist as such.
        :rtype: Runtime or None
        """
        if version is None and identifier is None:
            raise TypeError('Must supply version and/or identifier.')

        for runtime in self.runtimes:
            if version and runtime.version != version:
                continue
            if is_internal_runtime and runtime.is_internal_runtime != is_internal_runtime:
                continue
            if identifier and runtime.identifier != identifier:
                continue
            return runtime
        return None

    def find_device_by_udid(self, udid):
        """
        :param udid: The UDID of the device to find.
        :type udid: str
        :return: The `Device` with the specified UDID.
        :rtype: Device
        """
        for device in self.devices:
            if device.udid == udid:
                return device
        return None

    def current_device(self):
        # FIXME: Find the simulator device that was booted by Simulator.app. For now, pick some booted simulator device, which
        # may have been booted using the simctl command line tool.
        for device in self.devices:
            if device.state == Simulator.DeviceState.BOOTED:
                return device
        return None

    # FIXME: We should find an existing device with respect to its name, device type and runtime.
    def device(self, name=None, runtime=None, should_ignore_unavailable_devices=False):
        """
        :param name: The name of the desired device.
        :type name: str
        :param runtime: The runtime of the desired device.
        :type runtime: Runtime
        :return: A device with the specified name and/or runtime, or None if one doesn't exist as such
        :rtype: Device or None
        """
        if name is None and runtime is None:
            raise TypeError('Must supply name and/or runtime.')

        for device in self.devices:
            if should_ignore_unavailable_devices and not device.available:
                continue
            if name and device.name != name:
                continue
            if runtime and device.runtime != runtime:
                continue
            return device
        return None

    @property
    def available_runtimes(self):
        """
        :return: An iterator of all available runtimes.
        :rtype: iter
        """
        return itertools.ifilter(lambda runtime: runtime.available, self.runtimes)

    @property
    def devices(self):
        """
        :return: An iterator of all devices from all runtimes.
        :rtype: iter
        """
        return itertools.chain(*[runtime.devices for runtime in self.runtimes])

    @property
    def latest_available_runtime(self):
        """
        :return: Returns a Runtime object with the highest version.
        :rtype: Runtime or None
        """
        if not self.runtimes:
            return None
        return sorted(self.available_runtimes, key=lambda runtime: runtime.version, reverse=True)[0]

    def lookup_or_create_device(self, name, device_type, runtime):
        """
        Returns an available iOS Simulator device for testing.

        This function will create a new simulator device with the specified name,
        device type and runtime if one does not already exist.

        :param name: The name of the simulator device to lookup or create.
        :type name: str
        :param device_type: The CoreSimulator device type.
        :type device_type: DeviceType
        :param runtime: The CoreSimulator runtime.
        :type runtime: Runtime
        :return: A dictionary describing the device.
        :rtype: Device
        """
        assert(runtime.available)
        testing_device = self.device(name=name, runtime=runtime, should_ignore_unavailable_devices=True)
        if testing_device:
            _log.debug('lookup_or_create_device %s %s %s found %s', name, device_type, runtime, testing_device.name)
            return testing_device
        testing_device = Simulator.Device.create(name, device_type, runtime)
        _log.debug('lookup_or_create_device %s %s %s created %s', name, device_type, runtime, testing_device.name)
        assert(testing_device.available)
        return testing_device

    def __repr__(self):
        return '<iOS Simulator: {num_runtimes} runtimes, {num_device_types} device types>'.format(
            num_runtimes=len(self.runtimes),
            num_device_types=len(self.device_types))

    def __str__(self):
        description = ['iOS Simulator:']
        description += map(str, self.runtimes)
        description += map(str, self.device_types)
        description += map(str, self.devices)
        return '\n'.join(description)
