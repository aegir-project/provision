core = 7.x
api = 2

; This makefile fetches the latest release of Drupal from Drupal.org.
projects[drupal][type] = "core"

; The release.sh script updates the version of hostmaster.
projects[hostmaster][version] = "3.9-beta1"
projects[hostmaster][type] = "profile"
projects[hostmaster][variant] = "projects"
