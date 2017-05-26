core = 7.x
api = 2

; This makefile fetches the latest release of Drupal from Drupal.org.
projects[drupal][type] = "core"

; The release.sh script updates the version of hostmaster.
projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][tag] = "7.x-3.0-dev"
projects[hostmaster][download][url] = "http://git.drupal.org/project/hostmaster.git"
