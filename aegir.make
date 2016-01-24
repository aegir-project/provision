; Aegir Provision makefile
;

core = 7.x
api = 2

; BOA-3.0.0-dev

projects[drupal][type] = "core"
; projects[drupal][download][type] = "get"
; projects[drupal][download][url] = "http://files.aegir.cc/core/drupal-7.41.1.tar.gz"
projects[drupal][download][type] = "copy"
projects[drupal][download][url] = "/opt/tmp/make_local/drupal"

projects[hostmaster][type] = "profile"
; projects[hostmaster][download][type] = "git"
; projects[hostmaster][download][url] = "https://github.com/omega8cc/hostmaster.git"
; projects[hostmaster][download][branch] = "feature/3.0.x-profile"
projects[hostmaster][download][type] = "copy"
projects[hostmaster][download][url] = "/opt/tmp/make_local/hostmaster"

; projects[hostmaster][type] = "profile"
; projects[hostmaster][download][type] = "get"
; projects[hostmaster][download][url] = "http://files.aegir.cc/versions/stable/tar/hostmaster-BOA-3.0.0.tar.gz"
