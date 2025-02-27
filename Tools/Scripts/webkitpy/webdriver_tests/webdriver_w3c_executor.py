# Copyright (C) 2017 Igalia S.L.
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

import logging
import os
import sys

from webkitpy.common.system.filesystem import FileSystem
from webkitpy.common.webkit_finder import WebKitFinder
import webkitpy.thirdparty.autoinstalled.mozlog
import webkitpy.thirdparty.autoinstalled.mozprocess
import webkitpy.thirdparty.autoinstalled.pytest
import webkitpy.thirdparty.autoinstalled.pytest_timeout
from mozlog import structuredlog

w3c_tools_dir = WebKitFinder(FileSystem()).path_from_webkit_base('WebDriverTests', 'imported', 'w3c', 'tools')


def _ensure_directory_in_path(directory):
    if not directory in sys.path:
        sys.path.insert(0, directory)
_ensure_directory_in_path(os.path.join(w3c_tools_dir, 'webdriver'))
_ensure_directory_in_path(os.path.join(w3c_tools_dir, 'wptrunner'))

from wptrunner.executors.base import WdspecExecutor, WebDriverProtocol
from wptrunner.webdriver_server import WebDriverServer

_log = logging.getLogger(__name__)


class MessageLogger(object):

    def __init__(self, message_func):
        self.name = 'WebKit WebDriver WPT logger'
        self.send_message = message_func

    def _log_data(self, action, **kwargs):
        self.send_message('log', action, kwargs)

    def process_output(self, process, data, command):
        self._log_data('process_output', process=process, data=data, command=command)


class TestRunner(object):

    def __init__(self):
        self.logger = MessageLogger(self.send_message)
        structuredlog.set_default_logger(self.logger)

    def send_message(self, command, *args):
        if command == 'log':
            self._log(*args)

    def _log(self, level, details):
        if level == 'process_output':
            self._process_output(details['process'], details['command'], details['data'])
            return

        if not 'message' in details:
            return
        message = details['message']
        if level == 'info':
            _log.info(message)
        elif level == 'debug':
            _log.debug(message)
        elif level == 'error':
            _log.error(message)
        elif level == 'criticial':
            _log.critical(message)
        elif level == 'warning':
            _log.warning(message)

    def _process_output(self, pid, command, data):
        _log.debug('(%s:%d): %s' % (os.path.basename(command).split()[0], pid, data))


def _log_func(level_name):
    def log(self, message):
        self._log_data(level_name.lower(), message=message)
    log.__name__ = str(level_name).lower()
    return log

# Create all the methods on StructuredLog for debug levels.
for level_name in structuredlog.log_levels:
    setattr(MessageLogger, level_name.lower(), _log_func(level_name))


class WebKitDriverServer(WebDriverServer):
    default_base_path = '/'
    test_env = None

    def __init__(self, logger, binary=None, port=None, base_path='', args=None):
        WebDriverServer.__init__(self, logger, binary, port=port, base_path=base_path, args=args, env=self.test_env)

    def make_command(self):
        return [self.binary, '--port=%s' % str(self.port)] + self._args


class WebKitDriverProtocol(WebDriverProtocol):
    server_cls = WebKitDriverServer


class WebDriverW3CExecutor(WdspecExecutor):
    protocol_cls = WebKitDriverProtocol

    def __init__(self, driver, server, display_driver):
        WebKitDriverServer.test_env = display_driver._setup_environ_for_test()
        WebKitDriverServer.test_env.update(driver.browser_env())
        server_config = {'host': server.host(), 'ports': {'http': [str(server.port())]}}
        WdspecExecutor.__init__(self, driver.browser_name(), server_config, driver.binary_path(), None, capabilities=driver.capabilities())

    def setup(self):
        self.runner = TestRunner()
        self.protocol.setup(self.runner)

    def teardown(self):
        self.protocol.teardown()

    def run(self, path):
        # Timeout here doesn't really matter because it's ignored, so we pass 0.
        return self.do_wdspec(self.protocol.session_config, path, 0)
