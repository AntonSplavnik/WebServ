#!/usr/bin/php-cgi
<?php
// Read query parameter
$mode = isset($_GET['mode']) ? $_GET['mode'] : 'normal';

header('Content-Type: text/html');

if ($mode === 'memory_bomb') {
    echo "<h1>Memory Bomb Test (PHP)</h1>\n";
    echo "<p>Attempting to allocate huge memory...</p>\n";
    flush();

    try {
        // Try to allocate 200MB array
        $huge_array = array();
        for ($i = 0; $i < 1000000; $i++) {
            $huge_array[] = str_repeat('X', 10000); // 10KB per iteration
        }
        echo "<p>ERROR: Should have been killed!</p>\n";
    } catch (Error $e) {
        echo "<p>Error caught: " . $e->getMessage() . "</p>\n";
    }

} elseif ($mode === 'large_output') {
    echo "<h1>Large Output Test (PHP)</h1>\n";
    echo "<p>Generating 20MB of output...</p>\n";
    flush();

    // Generate 20MB
    $chunk = str_repeat('X', 1024); // 1KB
    for ($i = 0; $i < 20 * 1024; $i++) {
        echo $chunk . "\n";
    }

} elseif ($mode === 'slow') {
    echo "<h1>Slow Script Test (PHP)</h1>\n";
    echo "<p>Sleeping for 25 seconds...</p>\n";
    flush();
    sleep(25);
    echo "<p>Finished sleeping</p>\n";

} elseif ($mode === 'post_echo') {
    echo "<h1>POST Echo Test (PHP)</h1>\n";
    echo "<h2>POST Data:</h2>\n";
    echo "<pre>\n";
    $post_data = file_get_contents('php://input');
    echo htmlspecialchars($post_data);
    echo "</pre>\n";
    echo "<h2>\$_POST array:</h2>\n";
    echo "<pre>\n";
    print_r($_POST);
    echo "</pre>\n";

} else {
    // Normal operation
    echo "<html>\n";
    echo "<head><title>CGI Test - PHP</title></head>\n";
    echo "<body>\n";
    echo "<h1>CGI Script Working! (PHP)</h1>\n";
    echo "<h2>Environment Variables:</h2>\n";
    echo "<ul>\n";
    echo "<li>REQUEST_METHOD: " . ($_SERVER['REQUEST_METHOD'] ?? 'N/A') . "</li>\n";
    echo "<li>QUERY_STRING: " . ($_SERVER['QUERY_STRING'] ?? 'N/A') . "</li>\n";
    echo "<li>CONTENT_LENGTH: " . ($_SERVER['CONTENT_LENGTH'] ?? 'N/A') . "</li>\n";
    echo "<li>SCRIPT_FILENAME: " . ($_SERVER['SCRIPT_FILENAME'] ?? 'N/A') . "</li>\n";
    echo "</ul>\n";

    echo "<h2>Test Links:</h2>\n";
    echo "<ul>\n";
    echo "<li><a href='?mode=normal'>Normal Output</a></li>\n";
    echo "<li><a href='?mode=memory_bomb'>Memory Bomb (test setrlimit)</a></li>\n";
    echo "<li><a href='?mode=large_output'>Large Output (test MAX_CGI_OUTPUT)</a></li>\n";
    echo "<li><a href='?mode=slow'>Slow Script (test timeout)</a></li>\n";
    echo "</ul>\n";

    echo "<h2>POST Test Form:</h2>\n";
    echo "<form method='POST' action='?mode=post_echo'>\n";
    echo "<textarea name='data' rows='4' cols='50'>Test POST data here</textarea><br>\n";
    echo "<input type='submit' value='Submit POST'>\n";
    echo "</form>\n";

    echo "</body>\n";
    echo "</html>\n";
}
?>
