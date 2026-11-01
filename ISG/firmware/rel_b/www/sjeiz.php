<?php
// debug tijdelijk aan tijdens testen
error_reporting(E_ALL);
ini_set('display_errors', 1);

// laad globale configuratie waar $json_template in staat
// pas het pad aan naar het bestand dat de variabelen definieert
$configFile = '/firmware/rel_b/www/config.php';
if (is_readable($configFile)) {
    include_once $configFile;
}

// fallback paden als $json_template niet gezet of niet leesbaar is
$possible = array(
  isset($json_template) ? $json_template : null,
  '/var/volatile/cache/config.json.template',
  '/firmware/json_template.json',
  '/firmware/rel_b/json_template.json',
  '/firmware/rel_b/www/json_template.json'
);

// verwijder null waarden
$possible = array_values(array_filter($possible));

$json_template_found = null;
foreach ($possible as $p) {
  if ($p && is_readable($p)) { $json_template_found = $p; break; }
}

if ($json_template_found === null) {
  error_log("json_template niet gevonden. Geprobeerde paden: ".implode(', ', $possible));
  echo "JSON bestand niet gevonden\n";
  exit(1);
}

$json = json_decode(file_get_contents($json_template_found), false);
if ($json === null) {
  echo "Ongeldige JSON in $json_template_found\n";
  exit(1);
}