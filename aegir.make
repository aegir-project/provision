; Aegir Provision makefile
;

api = 2
core = 6.x

; BOA-2.4.2

projects[pressflow][type] = "core"
projects[pressflow][download][type] = "get"
projects[pressflow][download][url] = "http://files.aegir.cc/core/pressflow-6.35.1.tar.gz"

projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "get"
projects[hostmaster][download][url] = "http://files.aegir.cc/versions/stable/tar/hostmaster-BOA-2.4.2.tar.gz"
