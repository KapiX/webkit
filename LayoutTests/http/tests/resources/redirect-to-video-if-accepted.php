<?php
if (strpos($_SERVER['HTTP_ACCEPT'], 'video/*') !== false) {
    header('HTTP/1.1 301 Moved Permanently');
    header('Location: ' . $_GET['video']);
    return;
}

header('HTTP/1.1 400 Bad Request');
?>
