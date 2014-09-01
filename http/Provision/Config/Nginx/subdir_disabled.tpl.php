location ^~ /<?php print $subdir; ?>/ {
  return       404;
  ### Do not reveal Aegir front-end URL here.
}
