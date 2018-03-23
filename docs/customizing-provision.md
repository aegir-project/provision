# Customizing Provision

## Docker Services

When using Provision's Docker services, a docker-compose.yml file is automatically generated, using pre-built images for Database & Web servers.

Each server has a "config path" where all server configuration is stored, such as apache config. Check `provision status` for the server config path. The server config folder is filled with files. Most files are generated automaticaly by provision verify. You can create the files labelled \(Optional\) below to customize the behavior of this server's stack.

```
~/.config/provision/$SERVER_NAME
   /docker-compose.yml  # Generated on provision verify
   /docker-compose-overrides.yml   # (Optional) Merged into docker-compose.yml on provision verify** 
   /mysql.cnf           # (Optional) MySQL configuration can be put into this file.*** 
   /php.ini             # (Optional) Custom PHP configuration.****
   /php-cli.ini         # (Optional) Custom PHP configuration for CLI only.
   /Dockerfile.http     # (Optional) Custom dockerfile for the http service.*****
   /Dockerfile.db       # (Optional) Custom dockerfile to use the db service.
   /apacheDocker.conf   # Generated on provision verify
   /apacheDocker
     /platform.d        # Generated Platform apache configs. 
     /pre.d             # Custom Apache configs can be put in here.
     /post.d            # Custom Apache configs can be put in here.
     /vhost.d           # Generated Site virtualhost configs.
     /platform.d
```

\*\* The docker-compose command supports automatic merging of docker-compose.yml files, by passing multiple `-f` options. Provision detects if this file is present and automatically adds this for you when you run `provision verify`

\*\*\* mysql.cnf file must follow the right format:

```
[mysqld]
max_allowed_packet    = 32M
```

\*\*\*\* The `php.ini` file (if present) is mapped to `/etc/php/7.0/apache2/conf.d/99-provision.ini` and the `php-cli.ini` file is mapped to `/etc/php/7.0/cli/conf.d/99-provision.ini`. Make sure it is also in the right format:

```ini
memory_limit=512M
```

\*\*\*\*\* The http container requires a few specific things to work. You should use `FROM provision4/http` or `FROM provision4/http:php7` as your base image.  See the [Dockerfile.user](dockerfiles/Dockerfile.user) file as an example Dockerfile. Copy to ~/.config/provision/$SERVER_NAME/Dockerfile.http, then run `provision verify $SERVER_NAME` to build. If you wish to provide an entirely different http dockerfile, look at [Dockerfile.http.php7](dockerfiles/Dockerfile.http.php7) to learn the requirements. As long as your image has the web server configuration links and the correct sudoers file, Provision shopuld be able to use it.

#### Remember...

After modifying any optional configuration files, run `provision verify` on your server to enable the changes.