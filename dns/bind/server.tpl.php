<?php 
foreach ($records as $key => $name) {
  printf('zone "%s" { type master; file "%s/%s.zone"; allow-query { any; }; };' . "\n", $name, $dns_zoned_path, $name);
}
?>
