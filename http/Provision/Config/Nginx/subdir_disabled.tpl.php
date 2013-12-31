location ^~ /<?php print $subdir; ?>/ {
  return       404;
  ### Dont't reveal Aegir front-end URL here.
}
