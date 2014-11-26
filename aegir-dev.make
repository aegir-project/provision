; Aegir Provision makefile
;

api = 2
core = 6.x

; BOA-2.3.7

; this makefile fetches the latest Aegir code from git from drupal.org
; it shouldn't really change at all apart from major upgrades, where
; the branch will change
projects[drupal][type] = "core"
; hardcode the version number so we survive core releases
projects[drupal][version] = "6.31"
; fix for issue #2060727, patch from https://drupal.org/node/1954296
projects[drupal][patch][] = "http://drupal.org/files/common.inc_6.28.patch"

; chain into hostmaster from git's 2.x branch
projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][url] = "http://git.drupal.org/project/hostmaster.git"
projects[hostmaster][download][branch] = "6.x-2.x"
