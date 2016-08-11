<?php
/**
 * @file
 * Template file for Provision_Config_Data_Store.
 */
print "<?php \n"; ?>
<?php foreach ($records as $key => $record) {
  print "\n\${$this->key}['{$key}'] = " . var_export($record, TRUE) . ';';
}
?>
