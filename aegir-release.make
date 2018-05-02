core = 7.x
api = 2

; This makefile fetches the latest release of Drupal from Drupal.org.
projects[drupal][type] = "core"
projects[drupal][version] = 7.59

; Function each() is deprecated since PHP 7.2; https://www.drupal.org/project/drupal/issues/2925449
projects[drupal][patch][2925449] = "https://www.drupal.org/files/issues/2018-04-08/deprecated_each2925449-106.patch"

; The release.sh script updates the version of hostmaster.
projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][tag] = "7.x-3.151"
projects[hostmaster][download][url] = "http://git.drupal.org/project/hostmaster.git"
