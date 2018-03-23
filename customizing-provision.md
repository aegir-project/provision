# Customizing Provision

## Docker Services

When using Provision's Docker services, a docker-compose.yml file is automatically generated, using pre-built images for Database & Web servers.

You can override the docker stack in the following ways

### 1. Create docker-compose-overrides.yml

Each server has a "config path" where all server configuration is stored, such as apache config. Check `provision status` for the server config path. The server config path looks like this:

```
~/.config/provision/$SERVER_NAME
   /docker-compose.yml  # Generated on provision verify
   /docker-compose-overrides.yml   # (Optional) Merged into docker-compose.yml on provision verify** 
   /mysql.cnf           # (Optional) MySQL configuration can be put into this file.*** 
   /apacheDocker.conf   # Generated on provision verify
     /platform.d        # Generated Platform apache configs. 
     /pre.d             # Custom Apache configs can be put in here.
     /post.d            # Custom Apache configs can be put in here.
     /vhost.d           # Generated Site virtualhost configs.
     /platform.d
```

\*\* The docker-compose command supports automatic merging of docker-compose.yml files, by passing multiple `-f` options. Provision detects if this file is present and automatically adds this for you.

\*\*\* my.cnf file must follow the right format:

```
[mysqld]
max_allowed_packet	= 32M
```





