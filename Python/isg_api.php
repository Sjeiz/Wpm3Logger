<!--
	Dit bestand moet op de ISG web in de /var/volatile/www (<-symlink) geplaatst worden
	Het is dan benaderbaar via http://servicewelt.iot.cheizoo.lan/isg_api.php
-->	
	
<?php
header('Content-Type: application/json');

$cmd = "cd /firmware/rel_b/bin && ./isg_tester -p";
$output = shell_exec($cmd);

if ($output === null) {
    echo json_encode(array("error" => "isg_tester kon niet worden uitgevoerd"));
    exit;
}

$data = array();
$lines = explode("\n", $output);

foreach ($lines as $line) {
    // Match: 60000; 390; 6; ...
    if (preg_match('/^\s*(\d+);\s*([-\d\.]+)/', $line, $m)) {
        $id = $m[1];
        $value = $m[2] + 0;
        $data[$id] = $value;
    }
}

echo json_encode($data);
