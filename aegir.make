; Aegir Provision makefile
;

api = 2
core = 6.x

; BOA-2.4.0-dev

projects[pressflow][type] = "core"
projects[pressflow][download][type] = "get"
projects[pressflow][download][url] = "http://files.aegir.cc/core/pressflow-6.34.1.tar.gz"

projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][url] = "https://github.com/omega8cc/hostmaster.git"
projects[hostmaster][download][branch] = "2.4.x-profile"
