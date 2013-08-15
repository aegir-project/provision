core = 6.x
api = 2

projects[drupal][type] = "core"
projects[drupal][version] = "6.28"
projects[drupal][patch][] = "https://drupal.org/files/common.inc_6.28.patch"

projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][url] = "http://git.drupal.org/project/hostmaster.git"
projects[hostmaster][download][tag] = "6.x-2.x"
