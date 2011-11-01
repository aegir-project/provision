<?php print "<?php \n"; ?>
<?php foreach ($records as $key => $record) {
  print "\n\${$this->key}['{$key}'] = ". var_export($record, TRUE) .';';
}
?>
