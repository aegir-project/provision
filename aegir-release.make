; Aegir Provision makefile
;

api = 2
core = 6.x

; BOA-2.3.4

; this makefile fetches the latest release from Drupal.org
; it is maintained through the release.sh script
projects[hostmaster][type] = "core"
projects[hostmaster][patch][] = "http://drupal.org/files/common.inc_6.28.patch"
projects[hostmaster][version] = "6.x-2.0-dev"
