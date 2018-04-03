# Debugging

## NGINX

### "Permission Denied"

If you have everything setup and your site root is not in the normal location (such as /home/user/Projects/site) and you see "File not found" in the browser, but "Permission Denied" in the nginx error log, you have a permission problem:

If `selinux` is enabled, you must either disable it or configure the folder using `chcon`


NGINX User needs "x" permissions in every parent directory. Make sure `/home/USER` has `chmod a+x` permissions. 

Check every parent path with: `namei -om /path/to/check`

See https://stackoverflow.com/questions/6795350/nginx-403-forbidden-for-all-files for details.


 