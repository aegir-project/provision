core = 7.x
api = 2

; This makefile fetches the latest release of Drupal from Drupal.org.
projects[drupal][type] = "core"
projects[drupal][version] = 7.60

; Sync manually with drupal-org-core.make in the hostmaster repo.

; Sync manually with drupal-org-core.make in the hostmaster repo.

; Function each() is deprecated since PHP 7.2; https://www.drupal.org/project/drupal/issues/2925449
projects[drupal][patch][2925449] = "https://www.drupal.org/files/issues/2018-04-08/deprecated_each2925449-106.patch"

; [PHP 7.2] Avoid count() calls on uncountable variables; https://www.drupal.org/project/drupal/issues/2885610
projects[drupal][patch][2885610] = "https://www.drupal.org/files/issues/2018-04-21/drupal-7-count-function-deprecation-fixes-2885610-19.patch"

; The release.sh script updates the version of hostmaster.
projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][tag] = "7.x-3.0-dev"
projects[hostmaster][download][url] = "http://git.drupal.org/project/hostmaster.git"
