<?php 
foreach ($records as $zone => $master) {
  if (is_array($master)) {
    $master = implode(";", $master);
  }
  printf('zone "%s" { type slave; file "%s/%s.zone"; masters { %s; }; allow-query { any; }; };' . "\n", $zone, $dns_zoned_path, $zone, $master);
}
?>
