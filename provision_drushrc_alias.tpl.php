<?php print "<?php \n"; ?>
<?php foreach ($contexts as $name => $data) { ?>
$aliases['<?php print $name; ?>'] = <?php print var_export($data, TRUE); ?>;
<?php } ?>
