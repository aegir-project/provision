<?php 
foreach ($records as $key => $name) {
  printf("conf-file=%s/%s.zone\n", $dns_zoned_path, $name);
}
?>
