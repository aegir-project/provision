core = 6.x
api = 2

; BOA-2.3.x-dev

projects[pressflow][type] = "core"
projects[pressflow][download][type] = "get"
projects[pressflow][download][url] = "http://files.aegir.cc/core/pressflow-6.33.1.tar.gz"

projects[hostmaster][type] = "profile"
projects[hostmaster][download][type] = "git"
projects[hostmaster][download][url] = "https://github.com/omega8cc/hostmaster.git"
projects[hostmaster][download][branch] = "2.3.x-profile"
