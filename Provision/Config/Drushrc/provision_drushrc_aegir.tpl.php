<?php
/**
 * @file
 * Template file for an Aegir-wide drushrc file.
 */
print "<?php \n\n\$options['hosting_features'] = ". var_export($hosting_features, TRUE) . ";\n";

print "\$options['exclude'] = ". var_export($drush_exclude, TRUE) . ";\n";

?>
