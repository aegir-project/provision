; Aegir Provision makefile
;

core = 7.x
api = 2

projects[drupal][type] = "core"
projects[drupal][download][type] = "copy"
projects[drupal][download][url] = "/opt/tmp/make_local/drupal"

projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "copy"
projects[hostmaster][download][url] = "/opt/tmp/make_local/hostmaster"
