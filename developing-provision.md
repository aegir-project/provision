# Developing Provision

One of our missions is to be _Easy to Develop._

This is not just for the core team, but also so others can come in and customize the system to their needs without much difficulty.

## Code

The source code for provision is available at [github.com/aegir-project/provision](https://github.com/aegir-project/provision). The main branch is `4.x`.

The important files and folders are:

* composer.json  -  Defines the project and the dependencies.
* bin/provision  -  Executable. Run this to use provision.
* src/  -  All the new classes.
* vendor/  -  The new vendor directory. All composer libraries get installed here when you run `composer install`.
* .travis.yml  -  Automated tests for our new branch.
* README.md  - The main documentation file.

NOTE: The entire codebase pre-4.x is still in the repository, so \_everything else \_is legacy code.

## Classes

### [class Provision](https://github.com/aegir-project/provision/blob/4.x/src/Provision.php)

Implements:

* ConfigAwareInterface: `$provision->getConfig()`to load the CLI config, optionally loaded from ~/.provision.yml
* ContainerAwareInterface: Uses [container.thephpleague.com](http://container.thephpleague.com) for dependency injection.
* LoggerAwareInterface: `$provision->getLogger()->info()` to `$provision->getLogger()->debug(),` easy PSR logging.
* BuilderAwareInterface: Access to the Robo Builder. 
* and more... API docs coming soon.

### [class Context](https://github.com/aegir-project/provision/blob/4.x/src/Context.php)

A Context represents an object we are tracking, either a Server, Platform, or Site.

Each context type has different properties, defined in the `option_documentation()` method.

`$context->save()` will save the context properties into a YML file.  Run provision status to reveal the path to a context's YML file.

`$context->verifyCommand() `is triggered when the `provision verify` command is run

### [class ServerContext](https://github.com/aegir-project/provision/blob/4.x/src/Context/ServerContext.php)

ServerContext "provide" services, while all others "subscribe" to them.

Use `ServerContext::shell_exec()` to easily run commands in the Server's config directory, while hiding output, throwing exception with error messages, and showing output when running with `-v`.

### 



