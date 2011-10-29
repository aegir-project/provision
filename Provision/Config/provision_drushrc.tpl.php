<?php print "<?php \n"; ?>

<?php foreach ($option_keys as $key) {
  print "\n\$options['$key'] = ". var_export(${$key}, TRUE) .';';
}
?>
