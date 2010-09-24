<?php 
foreach ($records as $key => $name) {
  printf('zone "%s" { type slave; file "%s/%s.zone"; masters { %s; }; allow-query { any; }; };' . "\n", $name, $dns_zoned_path, $name, $master_server);
}
?>
