; Aegir Provision makefile
;

api = 2
core = 6.x

; BOA-2.4.0-dev

; this makefile fetches the latest release from Drupal.org
; it is maintained through the release.sh script
projects[hostmaster][type] = "core"
projects[hostmaster][version] = "6.x-2.0-dev"
