; Aegir Provision makefile
;

core = 7.x
api = 2

; this makefile fetches the latest release from Drupal.org
; it is maintained through the release.sh script
projects[hostmaster][type] = "core"
projects[hostmaster][version] = "7.x-3.0-dev"
