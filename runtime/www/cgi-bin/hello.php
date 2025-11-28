#!/usr/bin/php-cgi
<?php
header('Content-Type: text/html');

echo "<html>\n";
echo "<body>\n";
echo "<h1>Hello from PHP CGI!</h1>\n";
echo "<p>Request Method: " . ($_SERVER['REQUEST_METHOD'] ?? 'N/A') . "</p>\n";
echo "<p>Query String: " . ($_SERVER['QUERY_STRING'] ?? 'N/A') . "</p>\n";

// Read POST body if present
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $body = file_get_contents('php://input');
    echo "<p>POST Body: " . htmlspecialchars($body) . "</p>\n";
}

echo "</body>\n";
echo "</html>\n";
?>
